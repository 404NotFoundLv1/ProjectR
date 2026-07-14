// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillTypes.h"
#include "Components/ActorComponent.h"

#include "PRPlayerSkillComponent.generated.h"

struct FPRDamageRequest;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FPRPlayerSkillPhaseExpiredNative,
	FGameplayTag,
	EPRPlayerSkillPhase);

/** Avatar-local player-skill phase and controlled-displacement state. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRPlayerSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRPlayerSkillComponent();

	bool BeginPhase(FGameplayTag AbilityTag, EPRPlayerSkillPhase Phase, float Duration);
	void CancelExecution();
	void ClearExecution();

	FGameplayTag GetCurrentAbilityTag() const;
	EPRPlayerSkillPhase GetCurrentPhase() const;
	FGuid GetActiveDisplacementRequestId() const;
	FPRPlayerSkillPhaseExpiredNative& OnPhaseExpired();

	bool RequestOwnedDisplacement(
		const FPRAbilityDisplacementRequest& Request,
		FGameplayTag& OutFailureTag);
	void CancelOwnedDisplacement();

	EPRCombatMitigationResult EvaluateIncomingDamage(
		const FPRDamageRequest& Request,
		FGameplayTagContainer& OutResponseTags) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandlePhaseExpired();
	void HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result);
	void EnsureSubsystemBinding();

	FGameplayTag CurrentAbilityTag;
	EPRPlayerSkillPhase CurrentPhase = EPRPlayerSkillPhase::Idle;
	FTimerHandle PhaseTimerHandle;
	FGuid ActiveDisplacementRequestId;
	FDelegateHandle DisplacementFinishedHandle;
	FPRPlayerSkillPhaseExpiredNative PhaseExpiredEvent;
};
