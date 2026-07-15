// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_ShadowThrust.generated.h"

/** Native ShadowThrust startup, controlled displacement, path sweep, and recovery. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_ShadowThrust : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_ShadowThrust();

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
	void HandlePhaseExpired(FGameplayTag ExpiredAbilityTag, EPRPlayerSkillPhase ExpiredPhase);
	void HandleStartupExpired();
	void HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result);
	void ScanCurrentPath();
	void ScanSegment(FVector SegmentStart, FVector SegmentEnd);
	void FinishActive(EPRAbilityDisplacementEndReason EndReason);
	void EnterRecovery();
	void PublishCompletedOutcome();
	void EndCurrentAbility(bool bWasCancelled);
	void ClearRuntimeBindings();

	FDelegateHandle PhaseExpiredHandle;
	FDelegateHandle DisplacementFinishedHandle;
	FTimerHandle ScanTimerHandle;
	FGuid DisplacementRequestId;
	FVector PreviousScanLocation = FVector::ZeroVector;
	TMap<TWeakObjectPtr<AActor>, FName> LockedTargets;
	TSet<FName> HitTargetIds;
	bool bResolvedPathObstructed = false;
	bool bFinishingActive = false;
	bool bEnding = false;
};
