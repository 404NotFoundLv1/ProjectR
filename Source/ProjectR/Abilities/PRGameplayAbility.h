// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "Abilities/PRAbilityTypes.h"

#include "PRGameplayAbility.generated.h"

/** ProjectR ability base that owns activation policy and verified commit semantics. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPRGameplayAbility();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual bool CommitAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) override;

	const FGameplayTag& GetProjectRAbilityTag() const;
	EPRAbilityActivationPolicy GetActivationPolicy() const;
	bool CanActivateWhileDead() const;
	bool ShouldCancelOnDeath() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	EPRAbilityActivationPolicy ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	bool bCanActivateWhileDead = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	bool bCancelOnDeath = true;
};
