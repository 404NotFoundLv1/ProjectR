// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_FireSlash.generated.h"

class UGameplayEffect;

/** Native FireSlash startup, deterministic cone hit, and Burning registration. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_FireSlash : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_FireSlash();

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
	void EnterRecovery();
	void EndCurrentAbility(bool bWasCancelled);
	void UnbindPhaseDelegate();
	TSubclassOf<UGameplayEffect> ResolveBurningEffectClass() const;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Skill|Burning")
	TSubclassOf<UGameplayEffect> BurningEffectClass;

	FDelegateHandle PhaseExpiredHandle;
	bool bEnding = false;
};
