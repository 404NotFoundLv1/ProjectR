// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/PRGameplayAbility.h"
#include "TimerManager.h"

#include "PRGA_BasicAttack.generated.h"

class UPRBasicAttackDataAsset;

/** Native, zero-resource standard attack routed by the existing Input.Attack semantic. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_BasicAttack : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_BasicAttack();

	virtual bool CanActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	const UPRBasicAttackDataAsset* GetBasicAttackData() const;
	void HandleStartupElapsed();
	void HandleCompletionElapsed();
	void EndCurrentAbility(bool bWasCancelled);
	void ClearRuntimeTimers();

	FTimerHandle StartupTimerHandle;
	FTimerHandle EndTimerHandle;
	FVector LockedAimDirection = FVector::ZeroVector;
	bool bEnding = false;
};
