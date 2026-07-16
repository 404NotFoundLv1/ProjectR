// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"
#include "GameplayEffectTypes.h"

#include "PRGA_CounterProofWall.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UPRPlayerSkillComponent;

/** Native held-guard ability that owns the temporary Guarding GE lifecycle. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_CounterProofWall : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_CounterProofWall();
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void HandlePhaseExpired(FGameplayTag Tag, EPRPlayerSkillPhase Phase);
	void EndCurrentAbility(bool bWasCancelled);
	void ClearRuntimeState();
	TSubclassOf<UGameplayEffect> ResolveGuardingEffectClass() const;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Skill|Status")
	TSubclassOf<UGameplayEffect> GuardingEffectClass;

	TWeakObjectPtr<UAbilitySystemComponent> GuardingASC;
	TWeakObjectPtr<UPRPlayerSkillComponent> GuardComponent;
	FActiveGameplayEffectHandle GuardingEffectHandle;
	FDelegateHandle PhaseExpiredHandle;
	bool bEnding = false;
};
