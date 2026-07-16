// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_VectorHook.generated.h"

/** Configuration-only native parent for VectorHook; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_VectorHook : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_VectorHook();

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
	bool ResolveHookPlan(
		const FGameplayAbilityActorInfo* ActorInfo,
		const class UPRPlayerSkillDataAsset& Data,
		FPRAbilityTargetQueryResult& OutTargets,
		FPRAbilityDisplacementRequest& OutDisplacement,
		FGameplayTag& OutFailureTag) const;
	void HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result);
	void HandlePhaseExpired(FGameplayTag ExpiredAbilityTag, EPRPlayerSkillPhase ExpiredPhase);
	void HandleValidationTick();
	void EndCurrentAbility(bool bWasCancelled);
	void ClearRuntimeBindings();
	void PublishCompletedOutcome();

	FDelegateHandle PhaseExpiredHandle;
	FDelegateHandle DisplacementFinishedHandle;
	FTimerHandle ValidationTimerHandle;
	FGuid DisplacementRequestId;
	TWeakObjectPtr<AActor> ActivationAvatar;
	TWeakObjectPtr<AActor> LockedTarget;
	FName LockedTargetId;
	bool bEnding = false;
	bool bCompletedOutcomePublished = false;
};
