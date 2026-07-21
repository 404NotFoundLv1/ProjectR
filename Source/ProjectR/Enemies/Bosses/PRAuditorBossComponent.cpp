// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/Bosses/PRAuditorBossComponent.h"

#include "Abilities/PRAttributeSet.h"
#include "Combat/PRCombatTypes.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/Bosses/PRAuditorBossDataAsset.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "GameplayEffect.h"
#include "ProjectR.h"
#include "TimerManager.h"

namespace PRAuditorBoss
{
const FGameplayTag DistanceCorrectionRule = FGameplayTag::RequestGameplayTag(TEXT("Rule.DistanceCorrection"));
const FGameplayTag RepetitionPenaltyRule = FGameplayTag::RequestGameplayTag(TEXT("Rule.RepetitionPenalty"));
const FGameplayTag CounterAttack = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.AuditorCounter"));
const FGameplayTag LockShotAttack = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.AuditorLockShot"));
constexpr int32 MinimumRepetitionCount = 3;
constexpr int32 MinimumDistanceSampleCount = 3;
constexpr float DistanceCorrectionMedianThreshold = 650.0f;

bool IsPlayerSkillTag(const FGameplayTag Tag)
{
	return Tag.IsValid() && Tag.ToString().StartsWith(TEXT("Skill."));
}

FGameplayTag SelectHighestFrequencySkill(const TArray<FPRAuditorBossSample>& Samples, int32& OutCount)
{
	TMap<FGameplayTag, int32> Counts;
	for (const FPRAuditorBossSample& Sample : Samples)
	{
		if (IsPlayerSkillTag(Sample.AbilityTag))
		{
			Counts.FindOrAdd(Sample.AbilityTag)++;
		}
	}

	FGameplayTag BestTag;
	OutCount = 0;
	for (const TPair<FGameplayTag, int32>& Pair : Counts)
	{
		if (Pair.Value > OutCount
			|| (Pair.Value == OutCount && (!BestTag.IsValid() || Pair.Key.ToString() < BestTag.ToString())))
		{
			BestTag = Pair.Key;
			OutCount = Pair.Value;
		}
	}
	return BestTag;
}
}

UPRAuditorBossComponent::UPRAuditorBossComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FGameplayTag UPRAuditorBossComponent::SelectRuleAuditRule(const TArray<FPRAuditorBossSample>& InSamples)
{
	int32 HighestFrequency = 0;
	PRAuditorBoss::SelectHighestFrequencySkill(InSamples, HighestFrequency);
	if (HighestFrequency >= PRAuditorBoss::MinimumRepetitionCount)
	{
		return PRAuditorBoss::RepetitionPenaltyRule;
	}

	if (InSamples.Num() >= PRAuditorBoss::MinimumDistanceSampleCount)
	{
		TArray<float> Distances;
		Distances.Reserve(InSamples.Num());
		for (const FPRAuditorBossSample& Sample : InSamples)
		{
			if (PRAuditorBoss::IsPlayerSkillTag(Sample.AbilityTag) && FMath::IsFinite(Sample.Distance))
			{
				Distances.Add(Sample.Distance);
			}
		}
		Distances.Sort();
		if (Distances.Num() >= PRAuditorBoss::MinimumDistanceSampleCount)
		{
			const int32 Middle = Distances.Num() / 2;
			const float Median = Distances.Num() % 2 == 0
				? (Distances[Middle - 1] + Distances[Middle]) * 0.5f
				: Distances[Middle];
			if (Median > PRAuditorBoss::DistanceCorrectionMedianThreshold)
			{
				return PRAuditorBoss::DistanceCorrectionRule;
			}
		}
	}

	return PRAuditorBoss::DistanceCorrectionRule;
}

FGameplayTag UPRAuditorBossComponent::SelectPredictedAbilityTag(const TArray<FPRAuditorBossSample>& InSamples)
{
	int32 UnusedCount = 0;
	return PRAuditorBoss::SelectHighestFrequencySkill(InSamples, UnusedCount);
}

bool UPRAuditorBossComponent::SelectEnemyAttack(AActor* Target, FGameplayTag& OutAttackTag) const
{
	OutAttackTag = FGameplayTag();
	if (!Target || Phase == EPRAuditorBossPhase::Defeated)
	{
		return false;
	}
	if (bCounterArmed)
	{
		OutAttackTag = PRAuditorBoss::CounterAttack;
		return true;
	}
	if (ActiveRuleId.MatchesTagExact(PRAuditorBoss::DistanceCorrectionRule))
	{
		OutAttackTag = PRAuditorBoss::LockShotAttack;
		return true;
	}
	return false;
}

bool UPRAuditorBossComponent::GetPreferredRangeOverride(AActor* Target, float& OutMinimumRange, float& OutMaximumRange) const
{
	if (!Target || !bCounterArmed)
	{
		return false;
	}
	OutMinimumRange = 0.0f;
	OutMaximumRange = 300.0f;
	return true;
}

void UPRAuditorBossComponent::NotifyEnemyAttackActivated(const FGameplayTag AttackTag)
{
	if (bCounterArmed && AttackTag.MatchesTagExact(PRAuditorBoss::CounterAttack))
	{
		bCounterArmed = false;
		PublishState();
	}
}

EPRAuditorBossPhase UPRAuditorBossComponent::GetPhase() const
{
	return Phase;
}

void UPRAuditorBossComponent::SetPhase(const EPRAuditorBossPhase InPhase)
{
	if (static_cast<uint8>(InPhase) > static_cast<uint8>(Phase))
	{
		Phase = InPhase;
	}
}

bool UPRAuditorBossComponent::IsPredictionBlocking(const FPRDamageRequest& Request, FGameplayTagContainer& OutResponseTags) const
{
	const APREnemyCharacter* Enemy = GetEnemyOwner();
	const UPRAttributeSet* Attributes = Enemy ? Enemy->GetAttributeSet() : nullptr;
	if (Phase != EPRAuditorBossPhase::PredictionShield || !PredictionShieldHandle.IsValid()
		|| !Attributes || Attributes->GetShield() <= 0.0f || !PredictedAbilityTag.IsValid()
		|| Request.AbilityTag != PredictedAbilityTag)
	{
		return false;
	}
	OutResponseTags.AddTag(UPRTagLibrary::GetCombatResponsePredictionBlockedTag());
	return true;
}

FGameplayTag UPRAuditorBossComponent::GetPredictedAbilityTag() const { return PredictedAbilityTag; }
FGameplayTag UPRAuditorBossComponent::GetActiveRuleId() const { return ActiveRuleId; }

void UPRAuditorBossComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(InitializationTimer, this, &UPRAuditorBossComponent::InitializeWhenReady, 0.05f, true);
	}
}

void UPRAuditorBossComponent::InitializeWhenReady()
{
	APREnemyCharacter* Enemy = GetEnemyOwner();
	UPRAbilitySystemComponent* ASC = Enemy ? Enemy->GetProjectRAbilitySystemComponent() : nullptr;
	if (!Enemy || !ASC || !GetWorld())
	{
		return;
	}
	if (!Enemy->IsEnemyInitialized())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(InitializationTimer);
	if (UPRCombatSubsystem* Combat = GetWorld()->GetSubsystem<UPRCombatSubsystem>())
	{
		CombatEventHandle = Combat->OnCombatEvent().AddUObject(this, &UPRAuditorBossComponent::HandleCombatEvent);
	}
	HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UPRAuditorBossComponent::HandleHealthChanged);
	SetPhase(EPRAuditorBossPhase::Sampling);
	RefreshPhaseAndState();
}

void UPRAuditorBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitializationTimer);
		if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>())
		{
			Combat->OnCombatEvent().Remove(CombatEventHandle);
		}
	}
	if (APREnemyCharacter* Enemy = GetEnemyOwner())
	{
		if (UPRAbilitySystemComponent* ASC = Enemy->GetProjectRAbilitySystemComponent())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
			RemovePredictionShield();
		}
	}
	Samples.Empty();
	Super::EndPlay(EndPlayReason);
}

void UPRAuditorBossComponent::HandleCombatEvent(const FPRCombatEvent& Event)
{
	APREnemyCharacter* Enemy = GetEnemyOwner();
	if (!Enemy || (Phase != EPRAuditorBossPhase::Sampling && Phase != EPRAuditorBossPhase::RuleAudit)
		|| !PRAuditorBoss::IsPlayerSkillTag(Event.AbilityTag) || !Event.Instigator.IsValid())
	{
		return;
	}
	if (Event.Instigator.Get() == Enemy)
	{
		return;
	}
	const IPRCombatantInterface* InstigatorCombatant = Cast<IPRCombatantInterface>(Event.Instigator.Get());
	if (!InstigatorCombatant || InstigatorCombatant->GetCombatantId() != TEXT("Player"))
	{
		return;
	}
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : Event.WorldTimeSeconds;
	PruneSamples(Now);
	FPRAuditorBossSample& Sample = Samples.AddDefaulted_GetRef();
	Sample.AbilityTag = Event.AbilityTag;
	Sample.Distance = Event.Instigator.IsValid()
		? FMath::Abs(Event.Instigator->GetActorLocation().X - Enemy->GetActorLocation().X) : 0.0f;
	Sample.bPerfectTiming = Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
	Sample.bDodgeOutcome = Event.AbilityTag == FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"));
	Sample.WorldTimeSeconds = Now;
	while (Samples.Num() > 16) Samples.RemoveAt(0);
	if (Phase == EPRAuditorBossPhase::RuleAudit && ActiveRuleId.MatchesTagExact(PRAuditorBoss::RepetitionPenaltyRule)
		&& Event.AbilityTag == RepeatedAbilityTag)
	{
		bCounterArmed = true;
	}
	RefreshPhaseAndState();
}

void UPRAuditorBossComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshPhaseAndState();
}

void UPRAuditorBossComponent::RefreshPhaseAndState()
{
	APREnemyCharacter* Enemy = GetEnemyOwner();
	const UPRAttributeSet* Attributes = Enemy ? Enemy->GetAttributeSet() : nullptr;
	if (!Enemy || !Enemy->IsEnemyInitialized() || !Attributes || !GetWorld()) return;
	if (Attributes->GetHealth() <= 0.0f)
	{
		HandleBossDefeated();
		return;
	}
	const float Ratio = Attributes->GetMaxHealth() > 0.0f ? Attributes->GetHealth() / Attributes->GetMaxHealth() : 0.0f;
	const UPRAuditorBossDataAsset* Data = GetAuditorData();
	if (!Data)
	{
		UE_LOG(LogProjectR, Error, TEXT("Auditor phase evaluation rejected: Boss prototype data was invalid."));
		return;
	}
	if (Phase == EPRAuditorBossPhase::Sampling && Ratio <= Data->RuleAuditHealthRatio)
	{
		SetPhase(EPRAuditorBossPhase::RuleAudit);
		EvaluateRuleAudit();
	}
	if (Phase == EPRAuditorBossPhase::RuleAudit && Ratio <= Data->PredictionShieldHealthRatio)
	{
		SetPhase(EPRAuditorBossPhase::PredictionShield);
		EnterPredictionShield();
	}
	if (Phase == EPRAuditorBossPhase::PredictionShield && Attributes->GetShield() <= 0.0f)
	{
		RemovePredictionShield();
		PredictedAbilityTag = FGameplayTag();
	}
	PublishState();
}

void UPRAuditorBossComponent::EvaluateRuleAudit()
{
	PruneSamples(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
	int32 RepeatedCount = 0;
	RepeatedAbilityTag = PRAuditorBoss::SelectHighestFrequencySkill(Samples, RepeatedCount);
	ActiveRuleId = SelectRuleAuditRule(Samples);
	if (ActiveRuleId != PRAuditorBoss::RepetitionPenaltyRule)
	{
		RepeatedAbilityTag = FGameplayTag();
	}
}

void UPRAuditorBossComponent::EnterPredictionShield()
{
	PredictedAbilityTag = SelectPredictedAbilityTag(Samples);
	APREnemyCharacter* Enemy = GetEnemyOwner();
	UPRAbilitySystemComponent* ASC = Enemy ? Enemy->GetProjectRAbilitySystemComponent() : nullptr;
	const UPRAuditorBossDataAsset* Data = GetAuditorData();
	if (PredictedAbilityTag.IsValid() && ASC && Data && Data->PredictionShieldEffect && !PredictionShieldHandle.IsValid())
	{
		PredictionShieldHandle = ASC->ApplyGameplayEffectToSelf(Data->PredictionShieldEffect->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
		if (PredictionShieldHandle.IsValid())
		{
			// The Infinite GE owns the temporary MaxShield cap. Shield itself must be
			// initialized once, not continuously overridden, so standard Combat damage
			// can deplete the 240-point prediction shield.
			ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), Data->PredictionShieldValue);
		}
	}
	if (!PredictionShieldHandle.IsValid())
	{
		PredictedAbilityTag = FGameplayTag();
	}
}

void UPRAuditorBossComponent::RemovePredictionShield()
{
	if (PredictionShieldHandle.IsValid())
	{
		if (APREnemyCharacter* Enemy = GetEnemyOwner())
		{
			if (UPRAbilitySystemComponent* ASC = Enemy->GetProjectRAbilitySystemComponent()) ASC->RemoveActiveGameplayEffect(PredictionShieldHandle);
		}
		PredictionShieldHandle.Invalidate();
	}
}

void UPRAuditorBossComponent::HandleBossDefeated()
{
	if (bDefeatedPublished) return;
	bDefeatedPublished = true;
	SetPhase(EPRAuditorBossPhase::Defeated);
	RemovePredictionShield();
	if (APREnemyCharacter* Enemy = GetEnemyOwner())
	{
		if (Enemy->GetEnemyBrain()) Enemy->GetEnemyBrain()->StopBrain();
	}
	PublishState();
	if (UPRBossSubsystem* Bosses = GetWorld() ? GetWorld()->GetSubsystem<UPRBossSubsystem>() : nullptr)
	{
		FPRPrototypeRunResult Result;
		Result.CompletionId = FGuid::NewGuid(); Result.BossId = TEXT("Auditor");
		Result.BossSpawnId = GetEnemyOwner() ? GetEnemyOwner()->GetSpawnId() : FGuid();
		Result.CounterproofFragmentsAwarded = GetAuditorData() ? GetAuditorData()->CounterproofFragmentsAwarded : 1;
		Result.WorldTimeSeconds = GetWorld()->GetTimeSeconds();
		Bosses->PublishPrototypeRunCompleted(Result);
	}
}

void UPRAuditorBossComponent::PublishState() const
{
	APREnemyCharacter* Enemy = GetEnemyOwner(); const UPRAttributeSet* A = Enemy ? Enemy->GetAttributeSet() : nullptr;
	if (!Enemy || !A || !GetWorld()) return;
	if (UPRBossSubsystem* Bosses = GetWorld()->GetSubsystem<UPRBossSubsystem>())
	{
		FPRBossRuntimeState State; State.BossId = TEXT("Auditor"); State.SpawnId = Enemy->GetSpawnId(); State.Phase = Phase;
		State.Health = A->GetHealth(); State.MaxHealth = A->GetMaxHealth(); State.Shield = A->GetShield(); State.MaxShield = A->GetMaxShield();
		State.ActiveRuleId = ActiveRuleId; State.PredictedAbilityTag = PredictedAbilityTag;
		if (const UPREnemyBrainComponent* Brain = Enemy->GetEnemyBrain()) { State.ActiveAttackTag = Brain->GetRuntimeState().ActiveAttackTag; State.AttackPhase = static_cast<uint8>(Brain->GetRuntimeState().AttackPhase); }
		State.PhaseSequence = static_cast<int32>(Phase); State.bDefeated = Phase == EPRAuditorBossPhase::Defeated; State.WorldTimeSeconds = GetWorld()->GetTimeSeconds(); Bosses->PublishBossState(State);
	}
}

void UPRAuditorBossComponent::PruneSamples(const double Now)
{
	Samples.RemoveAll([Now](const FPRAuditorBossSample& Sample) { return Now - Sample.WorldTimeSeconds > 12.0; });
}

const UPRAuditorBossDataAsset* UPRAuditorBossComponent::GetAuditorData() const
{
	return Cast<UPRAuditorBossDataAsset>(GetEnemyOwner() ? GetEnemyOwner()->GetEnemyPrototype() : nullptr);
}

APREnemyCharacter* UPRAuditorBossComponent::GetEnemyOwner() const { return Cast<APREnemyCharacter>(GetOwner()); }
