// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/PRGameplayAbility.h"

#include "PREnemyAttackGameplayAbility.generated.h"

class UPREnemyAttackDataAsset;

/** Native server-only bridge from a validated enemy attack spec to its brain timeline. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPREnemyAttackGameplayAbility : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPREnemyAttackGameplayAbility();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleTimelineFinished();
	TWeakObjectPtr<class UPREnemyBrainComponent> ActiveBrain;
};
