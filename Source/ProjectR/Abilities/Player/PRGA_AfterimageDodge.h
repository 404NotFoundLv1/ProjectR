// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_AfterimageDodge.generated.h"

class APRSkillDecoyActor;
class UGameplayEffect;

/** Configuration-only native parent for AfterimageDodge; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_AfterimageDodge : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_AfterimageDodge();
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void HandlePhaseExpired(FGameplayTag Tag, EPRPlayerSkillPhase Phase);
	void HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result);
	void FinishActive(EPRAbilityDisplacementEndReason EndReason);
	void EndCurrentAbility(bool bWasCancelled);
	void ClearRuntimeBindings();
	bool ResolveDodgePath(const FGameplayAbilityActorInfo* ActorInfo, const class UPRPlayerSkillDataAsset& Data, FVector& OutDirection, FVector& OutEnd, float& OutSafeDistance, FGameplayTag& OutFailureTag) const;
	void PublishOutcome(AActor* Target, FGameplayTag ResponseTag) const;
	TSubclassOf<UGameplayEffect> ResolveInvulnerableEffectClass() const;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Skill|Status")
	TSubclassOf<UGameplayEffect> InvulnerableEffectClass;

	TWeakObjectPtr<APRSkillDecoyActor> ActiveDecoy;
	FDelegateHandle PhaseExpiredHandle;
	FDelegateHandle DisplacementFinishedHandle;
	FGuid DisplacementRequestId;
	bool bResolvedPathObstructed = false;
	bool bEnding = false;
};
