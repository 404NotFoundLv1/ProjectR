// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Allocator/PRAllocatorBossComponent.h"

#include "AbilitySystemInterface.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemyCharacter.h"
#include "Roguelike/PRRewardApplication.h"
#include "Roguelike/PRRewardTypes.h"

UPRAllocatorBossComponent::UPRAllocatorBossComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRAllocatorBossComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>())
		{
			BoundCombat = Combat;
			CombatHandle = Combat->OnCombatEvent().AddUObject(this, &UPRAllocatorBossComponent::HandleCombatEvent);
		}
	}
}

void UPRAllocatorBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRuntimeEffects();
	if (UPRCombatSubsystem* Combat = BoundCombat.Get()) Combat->OnCombatEvent().Remove(CombatHandle);
	CombatHandle.Reset();
	BoundCombat.Reset();
	ResourceCounterSkills.Reset();
	DeprivationCounterSkills.Reset();
	RewardSnapshots.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRAllocatorBossComponent::ConfigureChapterState(const int32 AllocationPressure, const TArray<FPRAppliedRewardSnapshot>& AppliedRewards)
{
	RuntimeState.AllocationPressure = FMath::Clamp(AllocationPressure, 0, 4);
	RewardSnapshots = AppliedRewards;
	RewardSnapshots.Sort([](const FPRAppliedRewardSnapshot& A, const FPRAppliedRewardSnapshot& B)
	{
		return A.Tier != B.Tier ? A.Tier > B.Tier : A.RewardId.ToString() < B.RewardId.ToString();
	});
}

const FPRAllocatorBossRuntimeState& UPRAllocatorBossComponent::GetRuntimeState() const { return RuntimeState; }

APREnemyCharacter* UPRAllocatorBossComponent::GetBoss() const { return Cast<APREnemyCharacter>(GetOwner()); }

UAbilitySystemComponent* UPRAllocatorBossComponent::ResolvePlayerASC() const
{
	if (APawn* Pawn = GetWorld() ? GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr : nullptr)
	{
		if (IAbilitySystemInterface* AbilitySystem = Cast<IAbilitySystemInterface>(Pawn)) return AbilitySystem->GetAbilitySystemComponent();
	}
	return nullptr;
}

void UPRAllocatorBossComponent::HandleCombatEvent(const FPRCombatEvent& Event)
{
	APREnemyCharacter* Boss = GetBoss();
	if (!Boss || Event.Target.Get() != Boss || !Event.EventId.IsValid()) return;
	if (Event.AbilityTag.IsValid() && Event.AbilityTag.ToString().StartsWith(TEXT("Skill.")))
	{
		if (RuntimeState.Phase == EPRAllocatorBossPhase::ResourceLock && !RuntimeState.bResourceLockCountered)
		{
			ResourceCounterSkills.Add(Event.AbilityTag);
			if (ResourceCounterSkills.Num() >= 2)
			{
				RuntimeState.bResourceLockCountered = true;
				if (UAbilitySystemComponent* PlayerASC = ResolvePlayerASC()) FPRGASRewardApplication::Remove(*PlayerASC, ResourceLockHandle);
			}
		}
		if (RuntimeState.Phase == EPRAllocatorBossPhase::RewardDeprivation && !RuntimeState.bRewardDeprivationCountered)
		{
			if (!DeprivationCounterSkills.Contains(Event.AbilityTag)) DeprivationCounterSkills.Add(Event.AbilityTag);
			if (DeprivationCounterSkills.Num() >= 3)
			{
				RuntimeState.bRewardDeprivationCountered = true;
				if (UAbilitySystemComponent* PlayerASC = ResolvePlayerASC()) FPRGASRewardApplication::Remove(*PlayerASC, DeprivationHandle);
			}
		}
	}
	EvaluatePhases();
	if (Event.bFatal) PublishCompletion();
}

void UPRAllocatorBossComponent::EvaluatePhases()
{
	const APREnemyCharacter* Boss = GetBoss();
	const UPRAttributeSet* Attributes = Boss ? Boss->GetAttributeSet() : nullptr;
	if (!Attributes || Attributes->GetMaxHealth() <= UE_SMALL_NUMBER || RuntimeState.Phase == EPRAllocatorBossPhase::Defeated) return;
	const float HealthRatio = Attributes->GetHealth() / Attributes->GetMaxHealth();
	if (HealthRatio <= 0.25f && RuntimeState.Phase < EPRAllocatorBossPhase::PriceAudit) { EnterPriceAudit(); return; }
	if (HealthRatio <= 0.50f && RuntimeState.Phase < EPRAllocatorBossPhase::RewardDeprivation) { EnterRewardDeprivation(); return; }
	if (HealthRatio <= 0.75f && RuntimeState.Phase < EPRAllocatorBossPhase::ResourceLock) EnterResourceLock();
}

void UPRAllocatorBossComponent::EnterResourceLock()
{
	RuntimeState.Phase = EPRAllocatorBossPhase::ResourceLock;
	if (UAbilitySystemComponent* PlayerASC = ResolvePlayerASC())
	{
		const FPRRewardEffectSpec LockSpec{EPRRewardAttribute::MaxEnergy, EPRRewardEffectDuration::Session, -25.0f};
		FPRGASRewardApplication::Apply(*PlayerASC, LockSpec, ResourceLockHandle);
	}
}

void UPRAllocatorBossComponent::EnterRewardDeprivation()
{
	RuntimeState.Phase = EPRAllocatorBossPhase::RewardDeprivation;
	const FPRAppliedRewardSnapshot* Selected = RewardSnapshots.IsEmpty() ? nullptr : &RewardSnapshots[0];
	if (!Selected || Selected->EffectSpec.Duration != EPRRewardEffectDuration::Session)
	{
		RuntimeState.bRewardDeprivationCountered = true;
		return;
	}
	FPRRewardEffectSpec Inverse = Selected->EffectSpec;
	Inverse.Magnitude = -FMath::Clamp(Inverse.Magnitude, -1000.0f, 1000.0f);
	if (UAbilitySystemComponent* PlayerASC = ResolvePlayerASC())
	{
		if (FPRGASRewardApplication::Apply(*PlayerASC, Inverse, DeprivationHandle) != EPRRewardApplyResult::Applied) RuntimeState.bRewardDeprivationCountered = true;
	}
}

void UPRAllocatorBossComponent::EnterPriceAudit()
{
	RuntimeState.Phase = EPRAllocatorBossPhase::PriceAudit;
	int32 Tiers = RuntimeState.AllocationPressure;
	for (const FPRAppliedRewardSnapshot& Reward : RewardSnapshots) Tiers += FMath::Max(0, Reward.Tier);
	RuntimeState.AuditUnits = FMath::Clamp(Tiers, 0, 12);
	if (APREnemyCharacter* Boss = GetBoss())
	{
		if (UPRAbilitySystemComponent* ASC = Boss->GetProjectRAbilitySystemComponent())
		{
			const FPRRewardEffectSpec ShieldSpec{EPRRewardAttribute::Shield, EPRRewardEffectDuration::Instant, 100.0f + 20.0f * RuntimeState.AuditUnits};
			FActiveGameplayEffectHandle Ignored;
			FPRGASRewardApplication::Apply(*ASC, ShieldSpec, Ignored);
		}
	}
}

void UPRAllocatorBossComponent::PublishCompletion()
{
	if (bCompletionPublished) return;
	bCompletionPublished = true;
	RuntimeState.Phase = EPRAllocatorBossPhase::Defeated;
	FPRPrototypeRunResult Result;
	Result.CompletionId = FGuid::NewGuid();
	Result.BossId = UPRChapterContentRegistryDataAsset::GetAllocatorBossId();
	if (const APREnemyCharacter* Boss = GetBoss()) Result.BossSpawnId = Boss->GetSpawnId();
	Result.CounterproofFragmentsAwarded = 0;
	Result.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (UWorld* World = GetWorld()) if (UPRBossSubsystem* BossSubsystem = World->GetSubsystem<UPRBossSubsystem>()) BossSubsystem->PublishPrototypeRunCompleted(Result);
	ClearRuntimeEffects();
}

void UPRAllocatorBossComponent::ClearRuntimeEffects()
{
	if (UAbilitySystemComponent* PlayerASC = ResolvePlayerASC())
	{
		FPRGASRewardApplication::Remove(*PlayerASC, ResourceLockHandle);
		FPRGASRewardApplication::Remove(*PlayerASC, DeprivationHandle);
	}
	ResourceLockHandle = FActiveGameplayEffectHandle();
	DeprivationHandle = FActiveGameplayEffectHandle();
}
