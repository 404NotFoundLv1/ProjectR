// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/PRGameplayAbility.h"
#include "Abilities/Player/PRPlayerSkillTypes.h"

#include "PRPlayerSkillGameplayAbility.generated.h"

class UPRPlayerSkillComponent;
class UPRPlayerSkillDataAsset;

/** Common commit/snapshot/lifecycle base for formal P0 player skills. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRPlayerSkillGameplayAbility : public UPRGameplayAbility
{
	GENERATED_BODY()

public:
	UPRPlayerSkillGameplayAbility();

	virtual bool CommitAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	const UPRPlayerSkillDataAsset* GetSkillDataAsset() const;
	UPRPlayerSkillComponent* GetPlayerSkillComponent() const;

	bool PrepareExecutionSnapshot(
		const FPRAbilityTargetQueryResult& TargetResult,
		FVector Origin,
		FVector AimDirection,
		bool bCritical,
		FGameplayTag& OutFailureTag);
	const FPRPlayerSkillExecutionSnapshot* GetExecutionSnapshot() const;

private:
	FPRPlayerSkillExecutionSnapshot ExecutionSnapshot;
	bool bSnapshotPrepared = false;
	bool bSnapshotCommitted = false;
};
