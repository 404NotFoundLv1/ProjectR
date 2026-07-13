// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "Abilities/PRAbilityTypes.h"

#include "PRAbilitySystemComponent.generated.h"

/** Stable ProjectR Ability System Component extension point. */
UCLASS()
class PROJECTR_API UPRAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UPRAbilitySystemComponent();

	EPRAbilitySetOperationStatus GrantAbilitySet(
		const UPRAbilitySetDataAsset* AbilitySet,
		EPRAbilitySetGrantMode GrantMode,
		FPRAbilitySetGrantHandle& OutHandle);

	EPRAbilitySetOperationStatus RemoveAbilitySet(const FPRAbilitySetGrantHandle& Handle);

	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);

	bool GetAbilityRuntimeStateByAbilityTag(
		FGameplayTag AbilityTag,
		FPRAbilityRuntimeState& OutState) const;

	bool GetAbilityRuntimeStateByInputTag(
		FGameplayTag InputTag,
		FPRAbilityRuntimeState& OutState) const;

	FPRAbilityLifecycleEventNative& OnAbilityLifecycleEvent();

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	virtual void ClearActorInfo() override;
	virtual void NotifyAbilityCommit(UGameplayAbility* Ability) override;
	virtual void NotifyAbilityActivated(
		FGameplayAbilitySpecHandle Handle,
		UGameplayAbility* Ability) override;
	virtual void NotifyAbilityFailed(
		FGameplayAbilitySpecHandle Handle,
		UGameplayAbility* Ability,
		const FGameplayTagContainer& FailureReason) override;

private:
	struct FGrantRecord
	{
		TWeakObjectPtr<UPRAbilitySetDataAsset> AbilitySet;
		EPRAbilitySetGrantMode GrantMode = EPRAbilitySetGrantMode::InitializationOnly;
		TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
		TSet<int32> GrantedEntryIndices;
	};

	bool ValidateAbilitySetForGrant(const UPRAbilitySetDataAsset* AbilitySet) const;
	bool DoesSpecConflict(const FPRAbilitySetEntry& Entry) const;
	void RollBackGrantedSpecs(const TArray<FGameplayAbilitySpecHandle>& Handles);
	void ActivatePassiveAbilities();
	void ReleaseHeldAbilities();
	void CancelProjectRAbilities(bool bOnlyCancelOnDeath);
	void HandleProjectRLifeStateChanged(FGameplayTag Tag, int32 NewCount);
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void NotifyProjectRCommitFailed(
		UPRGameplayAbility* Ability,
		const FGameplayTagContainer& FailureTags);
	bool BuildRuntimeState(
		const FGameplayAbilitySpec& Spec,
		FPRAbilityRuntimeState& OutState) const;
	void BroadcastLifecycle(
		EPRAbilityLifecycleEventType EventType,
		FGameplayAbilitySpecHandle Handle,
		const FGameplayTagContainer& FailureTags = FGameplayTagContainer());
	TArray<FGameplayAbilitySpecHandle> FindSpecsByExactDynamicTag(FGameplayTag Tag) const;

	TMap<FGuid, FGrantRecord> GrantRecords;
	TSet<FGameplayAbilitySpecHandle> HeldAbilityHandles;
	FPRAbilityLifecycleEventNative AbilityLifecycleEvent;
	FDelegateHandle DeadTagEventHandle;
	FDelegateHandle AbilityEndedHandle;

	friend class UPRGameplayAbility;
	friend class FPRAbilityDeathReviveReplacementTest;
};
