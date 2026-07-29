// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Pacifier/PRPacifierBossComponent.h"

#include "Abilities/PRAttributeSet.h"
#include "Chapters/Pacifier/PRPacifierBoss.h"
#include "Chapters/Pacifier/PRPacifierIllusionProjection.h"
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemyCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UPRPacifierBossComponent::UPRPacifierBossComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRPacifierBossComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>())
		{
			BoundCombat = Combat;
			CombatHandle = Combat->OnCombatEvent().AddUObject(this, &UPRPacifierBossComponent::HandleCombatEvent);
		}
	}
}

void UPRPacifierBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTransientState();
	if (UPRCombatSubsystem* Combat = BoundCombat.Get()) Combat->OnCombatEvent().Remove(CombatHandle);
	CombatHandle.Reset();
	BoundCombat.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRPacifierBossComponent::ConfigureChapterState(const int32 ComfortPressure)
{
	RuntimeState.ComfortPressure = FMath::Clamp(ComfortPressure, 0, 4);
}

const FPRPacifierBossRuntimeState& UPRPacifierBossComponent::GetRuntimeState() const { return RuntimeState; }
APREnemyCharacter* UPRPacifierBossComponent::GetBoss() const { return Cast<APREnemyCharacter>(GetOwner()); }
APawn* UPRPacifierBossComponent::GetPlayerPawn() const { return UGameplayStatics::GetPlayerPawn(this, 0); }

bool UPRPacifierBossComponent::IsLegalP0Skill(const FGameplayTag AbilityTag) const
{
	static const TSet<FGameplayTag> P0Skills = {
		FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.CounterProofWall"), false)};
	return AbilityTag.IsValid() && P0Skills.Contains(AbilityTag);
}

void UPRPacifierBossComponent::HandleCombatEvent(const FPRCombatEvent& Event)
{
	APREnemyCharacter* Boss = GetBoss();
	const bool bPlayerSkill = Boss && IsLegalP0Skill(Event.AbilityTag) && Event.Instigator.Get() == GetPlayerPawn();
	if (!Boss || !Event.EventId.IsValid()) return;

	if (bPlayerSkill && Event.Target.Get() == Boss)
	{
		if (RuntimeState.Phase == EPRPacifierBossPhase::IllusionSplit)
		{
			IllusionCounterSkills.Add(Event.AbilityTag);
			if (IllusionCounterSkills.Num() >= 2)
			{
				RuntimeState.bIllusionSplitCountered = true;
				ShowMechanic(TEXT("ILLUSION SPLIT COUNTERED"), FColor::Green);
				if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(IllusionWindowTimer);
				DestroyIllusionProjections();
			}
		}
		else if (RuntimeState.Phase == EPRPacifierBossPhase::LowRiskRewardLure)
		{
			const APawn* Player = GetPlayerPawn();
			if (Player && FVector::Dist(Player->GetActorLocation(), Boss->GetActorLocation()) <= 350.0f)
			{
				RuntimeState.bLowRiskLureCountered = true;
				ShowMechanic(TEXT("LOW-RISK LURE COUNTERED"), FColor::Green);
				if (UWorld* World = GetWorld())
				{
					World->GetTimerManager().ClearTimer(LureWindowTimer);
					World->GetTimerManager().ClearTimer(LureMonitorTimer);
				}
			}
		}
		else if (RuntimeState.Phase == EPRPacifierBossPhase::AdventureYieldSuppression
			&& !SuppressionCounterSkills.Contains(Event.AbilityTag)
			&& RuntimeState.SuppressionLayers > 0)
		{
			SuppressionCounterSkills.Add(Event.AbilityTag);
			--RuntimeState.SuppressionLayers;
			ShowMechanic(
				*FString::Printf(TEXT("YIELD SUPPRESSION: %d LAYERS"), RuntimeState.SuppressionLayers),
				RuntimeState.SuppressionLayers == 0 ? FColor::Green : FColor::Yellow);
			if (RuntimeState.SuppressionLayers == 0)
			{
				RuntimeState.bYieldSuppressionCountered = true;
				if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(SuppressionTimer);
			}
		}
	}

	if (Event.Target.Get() == Boss) EvaluatePhases();
	if (Event.Target.Get() == Boss && Event.bFatal) PublishCompletion();
}

void UPRPacifierBossComponent::EvaluatePhases()
{
	const APREnemyCharacter* Boss = GetBoss();
	const UPRAttributeSet* Attributes = Boss ? Boss->GetAttributeSet() : nullptr;
	if (!Attributes || Attributes->GetMaxHealth() <= UE_SMALL_NUMBER || RuntimeState.Phase == EPRPacifierBossPhase::Defeated) return;
	const float Ratio = Attributes->GetHealth() / Attributes->GetMaxHealth();
	if (Ratio <= 0.25f && RuntimeState.Phase < EPRPacifierBossPhase::AdventureYieldSuppression)
	{
		EnterAdventureYieldSuppression();
		return;
	}
	if (Ratio <= 0.50f && RuntimeState.Phase < EPRPacifierBossPhase::LowRiskRewardLure)
	{
		EnterLowRiskRewardLure();
		return;
	}
	if (Ratio <= 0.75f && RuntimeState.Phase < EPRPacifierBossPhase::IllusionSplit) EnterIllusionSplit();
}

void UPRPacifierBossComponent::EnterIllusionSplit()
{
	RuntimeState.Phase = EPRPacifierBossPhase::IllusionSplit;
	IllusionCounterSkills.Reset();
	ShowMechanic(TEXT("ILLUSION SPLIT: 1.0s WARNING"), FColor::Yellow);
	if (!GetWorld() || !GetBoss())
	{
		MarkDegradedNoOp();
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(IllusionWarningTimer, this, &UPRPacifierBossComponent::SpawnIllusionProjections, 1.0f, false);
}

void UPRPacifierBossComponent::SpawnIllusionProjections()
{
	UWorld* World = GetWorld();
	const APREnemyCharacter* Boss = GetBoss();
	if (!World || !Boss)
	{
		MarkDegradedNoOp();
		return;
	}
	const FVector Offsets[] = {FVector(-300.0f, 0.0f, 0.0f), FVector(300.0f, 0.0f, 0.0f)};
	RuntimeState.Projections.Reset();
	for (const FVector& Offset : Offsets)
	{
		FActorSpawnParameters Parameters;
		Parameters.ObjectFlags |= RF_Transient;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (APRPacifierIllusionProjection* Projection = World->SpawnActor<APRPacifierIllusionProjection>(
			APRPacifierIllusionProjection::StaticClass(), Boss->GetActorTransform(), Parameters))
		{
			Projection->SetActorLocation(Boss->GetActorLocation() + Offset);
			IllusionProjections.Add(Projection);
			FPRPacifierIllusionProjectionState& ProjectionState = RuntimeState.Projections.AddDefaulted_GetRef();
			ProjectionState.RelativeOffset = Offset;
			ProjectionState.bActive = true;
		}
	}
	RuntimeState.ActiveProjectionCount = IllusionProjections.Num();
	if (RuntimeState.ActiveProjectionCount != 2)
	{
		DestroyIllusionProjections();
		MarkDegradedNoOp();
		return;
	}
	ShowMechanic(TEXT("ILLUSION SPLIT: USE 2 DIFFERENT SKILLS"), FColor(80, 220, 255));
	World->GetTimerManager().SetTimer(IllusionWindowTimer, this, &UPRPacifierBossComponent::ResolveIllusionSplit, 4.0f, false);
}

void UPRPacifierBossComponent::ResolveIllusionSplit()
{
	DestroyIllusionProjections();
	if (!RuntimeState.bIllusionSplitCountered)
	{
		ShowMechanic(TEXT("ILLUSION SPLIT FAILED"), FColor::Red);
		ApplyPlayerDamage(10.0f, TEXT("Pacifier.IllusionSplit"));
	}
}

void UPRPacifierBossComponent::EnterLowRiskRewardLure()
{
	RuntimeState.Phase = EPRPacifierBossPhase::LowRiskRewardLure;
	bStayedOutsideLureRange = true;
	ShowMechanic(TEXT("LOW-RISK LURE: CLOSE IN + HIT"), FColor::Yellow);
	if (!GetWorld() || !GetBoss() || !GetPlayerPawn())
	{
		MarkDegradedNoOp();
		return;
	}
	MonitorLowRiskRewardLure();
	GetWorld()->GetTimerManager().SetTimer(LureMonitorTimer, this, &UPRPacifierBossComponent::MonitorLowRiskRewardLure, 0.1f, true);
	GetWorld()->GetTimerManager().SetTimer(LureWindowTimer, this, &UPRPacifierBossComponent::ResolveLowRiskRewardLure, 5.0f, false);
}

void UPRPacifierBossComponent::MonitorLowRiskRewardLure()
{
	const APREnemyCharacter* Boss = GetBoss();
	const APawn* Player = GetPlayerPawn();
	if (!Boss || !Player)
	{
		MarkDegradedNoOp();
		return;
	}
	if (FVector::Dist(Player->GetActorLocation(), Boss->GetActorLocation()) < 600.0f) bStayedOutsideLureRange = false;
}

void UPRPacifierBossComponent::ResolveLowRiskRewardLure()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(LureMonitorTimer);
	if (!RuntimeState.bLowRiskLureCountered && bStayedOutsideLureRange)
	{
		ShowMechanic(TEXT("LOW-RISK LURE FAILED"), FColor::Red);
		ApplyPlayerDamage(10.0f, TEXT("Pacifier.LowRiskRewardLure"));
	}
}

void UPRPacifierBossComponent::EnterAdventureYieldSuppression()
{
	RuntimeState.Phase = EPRPacifierBossPhase::AdventureYieldSuppression;
	RuntimeState.SuppressionLayers = FMath::Clamp(1 + RuntimeState.ComfortPressure, 1, 4);
	SuppressionCounterSkills.Reset();
	ShowMechanic(*FString::Printf(TEXT("YIELD SUPPRESSION: %d LAYERS"), RuntimeState.SuppressionLayers), FColor::Yellow);
	if (!GetWorld())
	{
		MarkDegradedNoOp();
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(SuppressionTimer, this, &UPRPacifierBossComponent::ResolveAdventureYieldSuppression, 4.0f, false);
}

void UPRPacifierBossComponent::ResolveAdventureYieldSuppression()
{
	RuntimeState.bYieldSuppressionCountered = RuntimeState.SuppressionLayers == 0;
	if (RuntimeState.SuppressionLayers > 0)
	{
		ShowMechanic(TEXT("YIELD SUPPRESSION FAILED"), FColor::Red);
		ApplyPlayerDamage(5.0f * RuntimeState.SuppressionLayers, TEXT("Pacifier.AdventureYieldSuppression"));
	}
}

void UPRPacifierBossComponent::DestroyIllusionProjections()
{
	for (TWeakObjectPtr<APRPacifierIllusionProjection>& Projection : IllusionProjections)
	{
		if (Projection.IsValid()) Projection->Destroy();
	}
	IllusionProjections.Reset();
	RuntimeState.ActiveProjectionCount = 0;
	RuntimeState.Projections.Reset();
}

void UPRPacifierBossComponent::ClearTransientState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IllusionWarningTimer);
		World->GetTimerManager().ClearTimer(IllusionWindowTimer);
		World->GetTimerManager().ClearTimer(LureWindowTimer);
		World->GetTimerManager().ClearTimer(LureMonitorTimer);
		World->GetTimerManager().ClearTimer(SuppressionTimer);
	}
	DestroyIllusionProjections();
	IllusionCounterSkills.Reset();
	SuppressionCounterSkills.Reset();
}

void UPRPacifierBossComponent::ApplyPlayerDamage(const float Damage, const FName SourceId)
{
	UPRCombatSubsystem* Combat = BoundCombat.Get();
	APawn* Player = GetPlayerPawn();
	APREnemyCharacter* Boss = GetBoss();
	if (!Combat || !Player || !Boss)
	{
		MarkDegradedNoOp();
		return;
	}
	FPRDamageRequest Request;
	Request.SourceId = SourceId;
	Request.DamageSource = this;
	Request.Instigator = Boss;
	Request.Target = Player;
	Request.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.AuditorCounter"), false);
	Request.RawDamage = Damage;
	Request.ImpactOrigin = Boss->GetActorLocation();
	Request.IncomingDirection = (Player->GetActorLocation() - Boss->GetActorLocation()).GetSafeNormal();
	Combat->ApplyDamage(Request);
}

void UPRPacifierBossComponent::ShowMechanic(const TCHAR* Text, const FColor& Color)
{
	if (APRPacifierBoss* Boss = Cast<APRPacifierBoss>(GetOwner()))
	{
		Boss->SetMechanicPresentation(FText::FromString(Text), Color, true);
	}
}

void UPRPacifierBossComponent::MarkDegradedNoOp()
{
	RuntimeState.bDegradedNoOp = true;
	ShowMechanic(TEXT("PACIFIER MECHANIC DEGRADED (NO-OP)"), FColor::Silver);
}

void UPRPacifierBossComponent::PublishCompletion()
{
	if (bCompletionPublished) return;
	bCompletionPublished = true;
	RuntimeState.Phase = EPRPacifierBossPhase::Defeated;
	ClearTransientState();
	if (APRPacifierBoss* Boss = Cast<APRPacifierBoss>(GetOwner()))
	{
		Boss->SetMechanicPresentation(FText::GetEmpty(), FColor::White, false);
	}
	FPRPrototypeRunResult Result;
	Result.CompletionId = FGuid::NewGuid();
	Result.BossId = UPRChapterContentRegistryDataAsset::GetPacifierBossId();
	if (const APREnemyCharacter* Boss = GetBoss()) Result.BossSpawnId = Boss->GetSpawnId();
	Result.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (UWorld* World = GetWorld())
	{
		if (UPRBossSubsystem* BossSubsystem = World->GetSubsystem<UPRBossSubsystem>())
		{
			BossSubsystem->PublishPrototypeRunCompleted(Result);
		}
	}
}
