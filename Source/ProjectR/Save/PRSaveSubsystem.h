// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRSaveGame.h"
#include "Save/PRSaveTypes.h"
#include "Save/PRSaveMigration.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/StrongObjectPtr.h"

#include "PRSaveSubsystem.generated.h"

class FPRSaveStorage;

/** Owns the single ProjectR profile and its serialized A/B save queue. */
UCLASS()
class PROJECTR_API UPRSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	EPRSaveResult LoadDefaultProfile(FGuid& OutRequestId);
	EPRSaveResult CreateNewDefaultProfile(FGuid& OutRequestId);
	EPRSaveRequestStatus RequestSaveCurrentProfile(FGuid& OutRequestId);
	FPRSaveRuntimeState GetSaveRuntimeState() const;
	FPRSaveOperationEventNative& OnSaveOperation();
	bool GetLoadedProfileSnapshot(FPRProfileSaveData& OutProfile) const;
	bool StageCompanionRelationships(const TArray<FPRCompanionRelationshipRecord>& Records);

private:
	struct FObservedGeneration
	{
		EPRSaveResult Result = EPRSaveResult::ReadFailed;
		FGuid ProfileId;
		int32 SchemaVersion = 0;
		int64 SaveRevision = 0;
		TArray<uint8> RawEnvelope;
	};

	struct FSaveRequest
	{
		explicit FSaveRequest(UPRSaveGame* InSnapshot)
			: Snapshot(InSnapshot)
		{
		}

		FGuid RequestId;
		TStrongObjectPtr<UPRSaveGame> Snapshot;
		EPRSaveGeneration TargetGeneration = EPRSaveGeneration::None;
		TArray<uint8> Envelope;
		FObservedGeneration PrewriteA;
		FObservedGeneration PrewriteB;
	};

	struct FLoadSelection
	{
		EPRSaveResult Result = EPRSaveResult::ReadFailed;
		UPRSaveGame* SaveGame = nullptr;
		EPRSaveGeneration Generation = EPRSaveGeneration::None;
		int32 SourceSchemaVersion = 0;
		bool bUsedAlternateGeneration = false;
		bool bNeedsResave = false;
	};

	static FObservedGeneration ObserveGeneration(const struct FPRSaveGenerationRead& Read);
	static FLoadSelection SelectLoadedGeneration(
		const struct FPRSaveGenerationRead& ReadA,
		const struct FPRSaveGenerationRead& ReadB);
	static bool ObservationsMatch(
		const FObservedGeneration& Expected,
		const FObservedGeneration& Current);

	void PublishOperation(
		const FGuid& RequestId,
		EPRSaveOperationType Operation,
		EPRSaveResult Result,
		int32 SourceSchemaVersion,
		EPRSaveGeneration Generation,
		bool bUsedAlternateGeneration,
		bool bEventNeedsResave);
	void StartActiveSave();
	void HandleActiveWriteComplete(bool bSuccess);
	void HandleActiveVerificationComplete(bool bSuccess, TArray<uint8> Data);
	void CompleteActiveSave(EPRSaveResult Result);
	UPRSaveGame* CaptureCurrentSnapshot() const;
	bool IsCallableOnGameThread(FGuid& OutRequestId) const;

	UPROPERTY(Transient)
	TObjectPtr<UPRSaveGame> LoadedSave = nullptr;

	EPRSaveSubsystemState State = EPRSaveSubsystemState::Uninitialized;
	EPRSaveGeneration LoadedGeneration = EPRSaveGeneration::None;
	EPRSaveResult LastResult = EPRSaveResult::InvalidRequest;
	bool bNeedsResave = false;
	bool bPublishingCompletion = false;
	TSharedPtr<FPRSaveStorage> Storage;
	FObservedGeneration ObservedA;
	FObservedGeneration ObservedB;
	TUniquePtr<FSaveRequest> ActiveSave;
	TUniquePtr<FSaveRequest> TrailingSave;
	FPRSaveMigrationRegistry MigrationRegistry;
	FPRSaveOperationEventNative SaveOperationEvent;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FPRSaveRuntimeTest;
	friend class FPRSaveFilesystemIsolationTest;
#endif
};
