// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "PRGA_ThunderDrop.generated.h"

class UGameplayEffect;

/** Configuration-only native parent for ThunderDrop; business logic is deferred. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_ThunderDrop : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGA_ThunderDrop();
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void HandlePhaseExpired(FGameplayTag Tag, EPRPlayerSkillPhase Phase);
	void EndCurrentAbility(bool bWasCancelled);
	void UnbindPhaseDelegate();
	TSubclassOf<UGameplayEffect> ResolveStunnedEffectClass() const;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Skill|Status")
	TSubclassOf<UGameplayEffect> StunnedEffectClass;

	FDelegateHandle PhaseExpiredHandle;
	bool bEnding = false;
};
