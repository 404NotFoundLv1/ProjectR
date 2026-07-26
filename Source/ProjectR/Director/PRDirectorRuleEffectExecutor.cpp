// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRDirectorRuleEffectExecutor.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Director/PRDirectorRuleEffectTypes.h"
#include "Enemies/PREnemyCharacter.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

namespace PRDirectorRuleExecutor
{
const FGameplayTag DirectorMagnitudeTag = FGameplayTag::RequestGameplayTag(TEXT("Rule.SurvivalProtocol"), false);
constexpr float PlayerAttackPowerMinimum = 0.50f;
constexpr float PlayerAttackPowerMaximum = 1.75f;
constexpr float PlayerMaxMinimum = 0.50f;
constexpr float PlayerMaxMaximum = 1.00f;
constexpr float EnemyAttackPowerMinimum = 1.00f;
constexpr float EnemyAttackPowerMaximum = 1.75f;
constexpr float EnemyMoveSpeedMinimum = 1.00f;
constexpr float EnemyMoveSpeedMaximum = 1.50f;
constexpr float EnemyArmorMinimum = 0.0f;
constexpr float EnemyArmorMaximum = 180.0f;

float LevelValue(const int32 Level, const float One, const float Two, const float Three)
{
	return Level == 1 ? One : Level == 2 ? Two : Three;
}
}

void FPRDirectorRuleEffectExecutor::BindWorld(UWorld* InWorld)
{
	if (World.Get() == InWorld) return;
	RemoveAggregateEffects();
	World = InWorld;
	RefreshAggregateEffects();
}

bool FPRDirectorRuleEffectExecutor::Apply(
	const FPRAppliedDirectorRuleHandle& Handle,
	const FText& VisibleReason,
	const FText& CounterDescription,
	const double WorldTimeSeconds)
{
	if (!Handle.HandleId.IsValid() || !FPRDirectorRuleEffectContract::IsRequiredRuleId(Handle.RuleId)
		|| Handle.Level < 1 || Handle.Level > 3 || !FMath::IsFinite(WorldTimeSeconds))
	{
		return false;
	}

	FPRDirectorRuleRuntimeState& State = States.FindOrAdd(Handle.RuleId);
	State.HandleId = Handle.HandleId;
	State.RuleId = Handle.RuleId;
	State.Level = Handle.Level;
	const FString RuleName = Handle.RuleId.ToString();
	State.Status = RuleName == TEXT("Rule.CompanionIsolation") || RuleName == TEXT("Rule.RiskReward") || RuleName == TEXT("Rule.ObedienceTest")
		? EPRDirectorRuleRuntimeStatus::Degraded : EPRDirectorRuleRuntimeStatus::Suspended;
	State.VisibleReason = VisibleReason;
	State.EffectDescription = FPRDirectorRuleEffectContract::GetEffectDescription(Handle.RuleId, Handle.Level);
	State.CounterDescription = CounterDescription;
	State.CounterProgress = 0;
	State.CounterTarget = FPRDirectorRuleEffectContract::GetCounterTarget(Handle.RuleId);
	State.bUsingDegradedEffect = RuleName == TEXT("Rule.DeleteEcho") || RuleName == TEXT("Rule.OptimalPath")
		|| RuleName == TEXT("Rule.ResourceBalance") || RuleName == TEXT("Rule.RiskReward") || RuleName == TEXT("Rule.ObedienceTest");
	State.RuntimeSequence = ++RuntimeSequence;
	State.WorldTimeSeconds = WorldTimeSeconds;
	RefreshAggregateEffects();
	return true;
}

bool FPRDirectorRuleEffectExecutor::GetState(const FGameplayTag RuleId, FPRDirectorRuleRuntimeState& OutState) const
{
	OutState = FPRDirectorRuleRuntimeState();
	const FPRDirectorRuleRuntimeState* Found = States.Find(RuleId);
	if (!Found) return false;
	OutState = *Found;
	return true;
}

bool FPRDirectorRuleEffectExecutor::Remove(const FPRAppliedDirectorRuleHandle& Handle, const double WorldTimeSeconds)
{
	FPRDirectorRuleRuntimeState* Found = States.Find(Handle.RuleId);
	if (!Found || Found->HandleId != Handle.HandleId || !FMath::IsFinite(WorldTimeSeconds)) return false;
	if (UWorld* CurrentWorld = World.Get())
	{
		if (UPRCompanionRuntimeSubsystem* Runtime = CurrentWorld->GetSubsystem<UPRCompanionRuntimeSubsystem>())
		{
			Runtime->ClearSupportPolicy(FName(*FString::Printf(TEXT("Director.%s"), *Handle.RuleId.ToString())));
		}
	}
	States.Remove(Handle.RuleId);
	++RuntimeSequence;
	RefreshAggregateEffects();
	return true;
}

void FPRDirectorRuleEffectExecutor::Reset()
{
	RemoveAggregateEffects();
	States.Empty();
	++RuntimeSequence;
}

void FPRDirectorRuleEffectExecutor::RebindWorldEffects()
{
	RefreshAggregateEffects();
}

bool FPRDirectorRuleEffectExecutor::AdvanceCounter(const FGameplayTag RuleId, const int32 Amount, const double WorldTimeSeconds)
{
	FPRDirectorRuleRuntimeState* State = States.Find(RuleId);
	if (!State || Amount <= 0 || !FMath::IsFinite(WorldTimeSeconds)) return false;
	State->CounterProgress = FMath::Clamp(State->CounterProgress + Amount, 0, State->CounterTarget);
	if (State->CounterProgress >= State->CounterTarget)
	{
		UpdateState(*State, EPRDirectorRuleRuntimeStatus::Countered, WorldTimeSeconds);
	}
	else
	{
		UpdateState(*State, EPRDirectorRuleRuntimeStatus::Active, WorldTimeSeconds);
	}
	RefreshAggregateEffects();
	return true;
}

bool FPRDirectorRuleEffectExecutor::SetRuleStatus(const FGameplayTag RuleId, const EPRDirectorRuleRuntimeStatus Status, const double WorldTimeSeconds)
{
	FPRDirectorRuleRuntimeState* State = States.Find(RuleId);
	if (!State || !FMath::IsFinite(WorldTimeSeconds)) return false;
	if (State->Status == Status) return true;
	UpdateState(*State, Status, WorldTimeSeconds);
	RefreshAggregateEffects();
	return true;
}

float FPRDirectorRuleEffectExecutor::GetRuleValue(const FGameplayTag RuleId, const int32 Level, const uint8 RawEffectTarget) const
{
	using EEffectTarget = FPRDirectorRuleEffectContract::EEffectTarget;
	const EEffectTarget EffectTarget = static_cast<EEffectTarget>(RawEffectTarget);
	const FString Name = RuleId.ToString();
	if (EffectTarget == EEffectTarget::PlayerAttackPower)
	{
		if (Name == TEXT("Rule.RepetitionPenalty")) return PRDirectorRuleExecutor::LevelValue(Level, 0.90f, 0.80f, 0.70f);
		if (Name == TEXT("Rule.SurvivalProtocol")) return PRDirectorRuleExecutor::LevelValue(Level, 1.10f, 1.20f, 1.30f);
		if (Name == TEXT("Rule.DistanceCorrection")) return PRDirectorRuleExecutor::LevelValue(Level, 0.92f, 0.84f, 0.76f);
		if (Name == TEXT("Rule.PredictionLock")) return PRDirectorRuleExecutor::LevelValue(Level, 0.75f, 0.60f, 0.45f);
		if (Name == TEXT("Rule.RiskReward")) return PRDirectorRuleExecutor::LevelValue(Level, 1.10f, 1.20f, 1.30f);
		if (Name == TEXT("Rule.ObedienceTest")) return PRDirectorRuleExecutor::LevelValue(Level, 1.05f, 1.10f, 1.15f);
	}
	if (EffectTarget == EEffectTarget::PlayerMaxHealth && Name == TEXT("Rule.RiskReward")) return PRDirectorRuleExecutor::LevelValue(Level, 0.90f, 0.80f, 0.70f);
	if (EffectTarget == EEffectTarget::PlayerMaxEnergy)
	{
		if (Name == TEXT("Rule.ResourceBalance")) return PRDirectorRuleExecutor::LevelValue(Level, 0.90f, 0.80f, 0.70f);
		if (Name == TEXT("Rule.ObedienceTest")) return PRDirectorRuleExecutor::LevelValue(Level, 0.95f, 0.90f, 0.85f);
	}
	if (EffectTarget == EEffectTarget::EnemyAttackPower && Name == TEXT("Rule.DeleteEcho")) return PRDirectorRuleExecutor::LevelValue(Level, 1.10f, 1.20f, 1.30f);
	if (EffectTarget == EEffectTarget::EnemyMoveSpeed && Name == TEXT("Rule.OptimalPath")) return PRDirectorRuleExecutor::LevelValue(Level, 1.05f, 1.10f, 1.15f);
	if (EffectTarget == EEffectTarget::EnemyArmor && Name == TEXT("Rule.CooperationAudit")) return PRDirectorRuleExecutor::LevelValue(Level, 20.0f, 40.0f, 60.0f);
	return 1.0f;
}

void FPRDirectorRuleEffectExecutor::RefreshAggregateEffects()
{
	if (bRefreshingEffects)
	{
		bRefreshRequested = true;
		return;
	}
	bRefreshingEffects = true;
	do
	{
		bRefreshRequested = false;
		RemoveAggregateEffects();
		if (!World.IsValid()) break;
	using EEffectTarget = FPRDirectorRuleEffectContract::EEffectTarget;
	for (uint8 RawTarget = static_cast<uint8>(EEffectTarget::PlayerAttackPower); RawTarget <= static_cast<uint8>(EEffectTarget::EnemyArmor); ++RawTarget)
	{
		float Aggregate = RawTarget == static_cast<uint8>(EEffectTarget::EnemyArmor) ? 0.0f : 1.0f;
		bool bHasRule = false;
		for (const TPair<FGameplayTag, FPRDirectorRuleRuntimeState>& Pair : States)
		{
			const FPRDirectorRuleRuntimeState& State = Pair.Value;
			if (State.Status != EPRDirectorRuleRuntimeStatus::Active && State.Status != EPRDirectorRuleRuntimeStatus::Degraded) continue;
			if (!FPRDirectorRuleEffectContract::GetEffectTargets(State.RuleId).Contains(static_cast<EEffectTarget>(RawTarget))) continue;
			const float Value = GetRuleValue(State.RuleId, State.Level, RawTarget);
			Aggregate = RawTarget == static_cast<uint8>(EEffectTarget::EnemyArmor) ? FMath::Max(Aggregate, Value) : Aggregate * Value;
			bHasRule = true;
		}
		if (!bHasRule) continue;
		switch (static_cast<EEffectTarget>(RawTarget))
		{
		case EEffectTarget::PlayerAttackPower: Aggregate = FMath::Clamp(Aggregate, PRDirectorRuleExecutor::PlayerAttackPowerMinimum, PRDirectorRuleExecutor::PlayerAttackPowerMaximum); break;
		case EEffectTarget::PlayerMaxHealth:
		case EEffectTarget::PlayerMaxEnergy: Aggregate = FMath::Clamp(Aggregate, PRDirectorRuleExecutor::PlayerMaxMinimum, PRDirectorRuleExecutor::PlayerMaxMaximum); break;
		case EEffectTarget::EnemyAttackPower: Aggregate = FMath::Clamp(Aggregate, PRDirectorRuleExecutor::EnemyAttackPowerMinimum, PRDirectorRuleExecutor::EnemyAttackPowerMaximum); break;
		case EEffectTarget::EnemyMoveSpeed: Aggregate = FMath::Clamp(Aggregate, PRDirectorRuleExecutor::EnemyMoveSpeedMinimum, PRDirectorRuleExecutor::EnemyMoveSpeedMaximum); break;
		case EEffectTarget::EnemyArmor: Aggregate = FMath::Clamp(Aggregate, PRDirectorRuleExecutor::EnemyArmorMinimum, PRDirectorRuleExecutor::EnemyArmorMaximum); break;
		default: break;
		}
		ApplyAggregateEffect(RawTarget, Aggregate);
	}
	ApplyCompanionPolicy();
	}
	while (bRefreshRequested);
	bRefreshingEffects = false;
}

void FPRDirectorRuleEffectExecutor::RemoveAggregateEffects()
{
	TMap<uint8, TArray<FAppliedEffect>> EffectsToRemove = MoveTemp(AppliedEffects);
	for (TPair<uint8, TArray<FAppliedEffect>>& Pair : EffectsToRemove)
	{
		for (FAppliedEffect& Applied : Pair.Value)
		{
			if (UAbilitySystemComponent* ASC = Applied.AbilitySystem.Get(); ASC && Applied.Handle.IsValid()) ASC->RemoveActiveGameplayEffect(Applied.Handle);
		}
	}
	if (UWorld* CurrentWorld = World.Get())
	{
		if (UPRCompanionRuntimeSubsystem* Runtime = CurrentWorld->GetSubsystem<UPRCompanionRuntimeSubsystem>())
		{
			for (const TPair<FGameplayTag, FPRDirectorRuleRuntimeState>& Pair : States) Runtime->ClearSupportPolicy(FName(*FString::Printf(TEXT("Director.%s"), *Pair.Key.ToString())));
		}
	}
}

void FPRDirectorRuleEffectExecutor::ApplyAggregateEffect(const uint8 RawEffectTarget, const float Magnitude)
{
	using EEffectTarget = FPRDirectorRuleEffectContract::EEffectTarget;
	const EEffectTarget Target = static_cast<EEffectTarget>(RawEffectTarget);
	const TCHAR* AssetPath = FPRDirectorRuleEffectContract::GetEffectAssetPath(Target);
	if (!AssetPath || !*AssetPath || !World.IsValid()) return;
	UClass* EffectClass = LoadClass<UGameplayEffect>(nullptr, AssetPath);
	if (!EffectClass) return;
	auto ApplyToASC = [this, RawEffectTarget, EffectClass, Magnitude](UAbilitySystemComponent* ASC)
	{
		if (!ASC) return;
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ASC->MakeEffectContext());
		if (!Spec.IsValid() || !Spec.Data.IsValid()) return;
		Spec.Data->SetSetByCallerMagnitude(PRDirectorRuleExecutor::DirectorMagnitudeTag, Magnitude);
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (Handle.IsValid()) AppliedEffects.FindOrAdd(RawEffectTarget).Add({ ASC, Handle });
	};
	if (Target == EEffectTarget::PlayerAttackPower || Target == EEffectTarget::PlayerMaxHealth || Target == EEffectTarget::PlayerMaxEnergy)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World.Get(), 0);
		IAbilitySystemInterface* Interface = PlayerPawn ? Cast<IAbilitySystemInterface>(PlayerPawn) : nullptr;
		ApplyToASC(Interface ? Interface->GetAbilitySystemComponent() : nullptr);
		return;
	}
	for (TActorIterator<APREnemyCharacter> It(World.Get()); It; ++It)
	{
		APREnemyCharacter* Enemy = *It;
		if (Enemy && Enemy->IsEnemyInitialized() && !Enemy->IsEnemyDead()) ApplyToASC(Enemy->GetProjectRAbilitySystemComponent());
	}
}

void FPRDirectorRuleEffectExecutor::ApplyCompanionPolicy()
{
	UWorld* CurrentWorld = World.Get();
	UPRCompanionRuntimeSubsystem* Runtime = CurrentWorld ? CurrentWorld->GetSubsystem<UPRCompanionRuntimeSubsystem>() : nullptr;
	if (!Runtime) return;
	for (const TPair<FGameplayTag, FPRDirectorRuleRuntimeState>& Pair : States)
	{
		const FPRDirectorRuleRuntimeState& State = Pair.Value;
		if (State.Status != EPRDirectorRuleRuntimeStatus::Active && State.Status != EPRDirectorRuleRuntimeStatus::Degraded) continue;
		const FString Name = State.RuleId.ToString();
		if (Name == TEXT("Rule.EmotionalInterference")) Runtime->SetSupportPolicy(FName(*FString::Printf(TEXT("Director.%s"), *Name)), PRDirectorRuleExecutor::LevelValue(State.Level, 1.25f, 1.50f, 1.75f), 1);
		if (Name == TEXT("Rule.CompanionIsolation")) Runtime->SetSupportPolicy(FName(*FString::Printf(TEXT("Director.%s"), *Name)), 1.0f, State.Level == 1 ? 3 : State.Level == 2 ? 2 : 1);
	}
}

void FPRDirectorRuleEffectExecutor::UpdateState(FPRDirectorRuleRuntimeState& State, const EPRDirectorRuleRuntimeStatus Status, const double WorldTimeSeconds)
{
	State.Status = Status;
	State.RuntimeSequence = ++RuntimeSequence;
	State.WorldTimeSeconds = WorldTimeSeconds;
}

void FPRDirectorRuleEffectExecutor::GetStates(TArray<FPRDirectorRuleRuntimeState>& OutStates) const
{
	OutStates.Reset();
	States.GenerateValueArray(OutStates);
	OutStates.Sort([](const FPRDirectorRuleRuntimeState& Left, const FPRDirectorRuleRuntimeState& Right)
	{
		return Left.RuleId.ToString() < Right.RuleId.ToString();
	});
}
