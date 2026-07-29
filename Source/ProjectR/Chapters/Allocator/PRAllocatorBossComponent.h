// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/PRChapterTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Roguelike/PRRewardTypes.h"

#include "PRAllocatorBossComponent.generated.h"

class APREnemyCharacter;
class UPRCombatSubsystem;

/** Owns all Allocator phase effects, counters and one-shot completion. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRAllocatorBossComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRAllocatorBossComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ConfigureChapterState(int32 AllocationPressure, const TArray<FPRAppliedRewardSnapshot>& AppliedRewards);
	const FPRAllocatorBossRuntimeState& GetRuntimeState() const;

private:
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void EvaluatePhases();
	void EnterResourceLock();
	void EnterRewardDeprivation();
	void EnterPriceAudit();
	void PublishCompletion();
	void ClearRuntimeEffects();
	class UAbilitySystemComponent* ResolvePlayerASC() const;
	APREnemyCharacter* GetBoss() const;

	FPRAllocatorBossRuntimeState RuntimeState;
	TArray<FPRAppliedRewardSnapshot> RewardSnapshots;
	TSet<FGameplayTag> ResourceCounterSkills;
	TArray<FGameplayTag> DeprivationCounterSkills;
	FActiveGameplayEffectHandle ResourceLockHandle;
	FActiveGameplayEffectHandle DeprivationHandle;
	FDelegateHandle CombatHandle;
	TWeakObjectPtr<UPRCombatSubsystem> BoundCombat;
	bool bCompletionPublished = false;
};
