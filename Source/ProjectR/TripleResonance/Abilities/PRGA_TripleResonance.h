// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Abilities/PRGameplayAbility.h"

#include "PRGA_TripleResonance.generated.h"

class UGameplayEffect;

/** One-shot, transient v0.7.2 ability. The subsystem owns eligibility, target identity and result persistence. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGA_TripleResonance : public UPRGameplayAbility
{
	GENERATED_BODY()
public:
	UPRGA_TripleResonance();
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual UGameplayEffect* GetCostGameplayEffect() const override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

private:
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|TripleResonance") TSubclassOf<UGameplayEffect> ResonanceChargeEffect;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|TripleResonance") TSubclassOf<UGameplayEffect> ResonanceCostEffect;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|TripleResonance") TSubclassOf<UGameplayEffect> ResonanceCooldownEffect;
};
