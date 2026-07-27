// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"
#include "Roguelike/Progression/PRProgressionTypes.h"
#include "Save/PRSaveTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"

#include "PRProgressionSubsystem.generated.h"

UCLASS()
class PROJECTR_API UPRProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	EPRProgressionOperationResult RequestUnlockNode(FPrimaryAssetId NodeId, FGuid& OutRequestId);
	EPRProgressionOperationResult RetryPendingUnlock(FGuid& OutRequestId);
	bool GetProgressionSnapshot(FPRProgressionSnapshot& OutSnapshot) const;
	bool GetRunSnapshot(FPRProgressionRunSnapshot& OutSnapshot) const;
	bool IsNodeUnlocked(FPrimaryAssetId NodeId) const;
	FPRProgressionChangedNative& OnProgressionChanged();
	FPRProgressionUnlockCompletedNative& OnUnlockCompleted();
	FPRProgressionRunSnapshotChangedNative& OnRunSnapshotChanged();

private:
	struct FPendingUnlock
	{
		FGuid RequestId;
		FGuid SaveRequestId;
		FPrimaryAssetId NodeId;
		struct FPRAccountPersistenceData AccountPersistence;
		struct FPRProgressionPersistenceData ProgressionPersistence;
	};

	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	void HandleAccountOperation(const struct FPRAccountOperationEvent& Event);
	void HandleRunStateChanged(const struct FPRRunRuntimeState& State);
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	bool RefreshSnapshotFromProfile();
	const class UPRProgressionRegistryDataAsset* LoadRegistry() const;
	EPRProgressionOperationResult ValidateUnlock(
		const class UPRProgressionNodeDataAsset& Node,
		const struct FPRProfileSaveData& Profile) const;
	bool DoesRelationshipRequirementPass(const FPRProgressionRelationshipRequirement& Requirement) const;
	void FreezeRunSnapshot();
	void ClearRunSnapshot();
	void RebindRuntimeEffects();
	void ClearRuntimeEffects();
	class UAbilitySystemComponent* ResolvePlayerAbilitySystem(UWorld* World) const;
	void PublishChanged(const FGuid& RequestId);
	void PublishUnlockCompleted(EPRProgressionOperationResult Result);
	void PublishRunSnapshotChanged();

	TWeakObjectPtr<class UPRSaveSubsystem> SaveSubsystem;
	TWeakObjectPtr<class UPRRunStateSubsystem> RunStateSubsystem;
	TWeakObjectPtr<UWorld> BoundWorld;
	TWeakObjectPtr<class UAbilitySystemComponent> AppliedAbilitySystem;
	struct FActiveGameplayEffectHandle HealthEffectHandle;
	struct FActiveGameplayEffectHandle EnergyEffectHandle;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle AccountOperationHandle;
	FDelegateHandle RunStateHandle;
	FDelegateHandle PostWorldInitializationHandle;
	FDelegateHandle WorldCleanupHandle;
	FDelegateHandle PostLoadMapHandle;
	TOptional<FPendingUnlock> PendingUnlock;
	bool bHasLoadedProfile = false;
	FPRProgressionSnapshot Snapshot;
	FPRProgressionRunSnapshot RunSnapshot;
	bool bHasRunSnapshot = false;
	FPRProgressionChangedNative ProgressionChanged;
	FPRProgressionUnlockCompletedNative UnlockCompleted;
	FPRProgressionRunSnapshotChangedNative RunSnapshotChanged;
};
