// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Companions/PRCompanionTypes.h"
#include "Save/PRSaveTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRCompanionSubsystem.generated.h"

class UPRCompanionDataAsset;
class UPRCompanionRegistryDataAsset;
class UPRSaveSubsystem;

/** Owns run-local primary selection and the controlled long-term relationship projection. */
UCLASS()
class PROJECTR_API UPRCompanionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	EPRPrimaryCompanionSelectionResult SelectPrimaryCompanion(FGameplayTag CompanionId);
	FPRCompanionSyncState GetSyncState() const;
	bool GetRelationshipSnapshot(FGameplayTag CompanionId, FPRCompanionRelationshipRecord& OutRecord) const;
	void GetAllRelationshipSnapshots(TArray<FPRCompanionRelationshipRecord>& OutRecords) const;
	EPRRelationshipApplyResult ApplyRelationshipDelta(const FPRRelationshipDelta& Delta);
	FPRRelationshipChangedNative& OnRelationshipChanged();
	FPRPrimaryCompanionSyncChangedNative& OnPrimarySyncChanged();
	bool IsRegistryReady() const;

private:
	void LoadFixedRegistry();
	void FinishRegistryLoad();
	void HandleSaveOperation(const FPRSaveOperationEvent& Event);
	void LoadRelationshipProjection();
	void ClearSyncState();
	bool IsKnownCompanion(FGameplayTag CompanionId) const;
	double GetWorldTimeSeconds() const;

	TSoftObjectPtr<UPRCompanionRegistryDataAsset> RegistryAsset;
	TMap<FGameplayTag, TObjectPtr<UPRCompanionDataAsset>> LoadedCompanions;
	TArray<FPRCompanionRelationshipRecord> RelationshipRecords;
	FPRCompanionSyncState SyncState;
	TWeakObjectPtr<UPRSaveSubsystem> SaveSubsystem;
	FDelegateHandle SaveOperationHandle;
	bool bRegistryReady = false;
	FPRRelationshipChangedNative RelationshipChanged;
	FPRPrimaryCompanionSyncChangedNative PrimarySyncChanged;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FPRCompanionRuntimeTest;
#endif
};
