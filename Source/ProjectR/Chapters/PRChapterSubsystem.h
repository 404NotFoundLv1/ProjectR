// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Chapters/PRChapterTypes.h"
#include "Save/PRChapterSaveTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRChapterSubsystem.generated.h"

UCLASS()
class PROJECTR_API UPRChapterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	bool GetSnapshot(FPRChapterSnapshot& OutSnapshot) const;
	bool GetLatestCompletion(FPRChapterCompletionResult& OutResult) const;
	EPRChapterOperationResult RetryPendingSettlement();
	FPRChapterStateChangedNative& OnStateChanged();
	FPRChapterCompletionNative& OnChapterCompleted();

#if WITH_DEV_AUTOMATION_TESTS
	/** Fixed editor-only fixture seam: persists only the Allocator proof into an already-isolated automation profile. */
	bool StageFixedAllocatorProofForAutomation();
#endif

private:
	struct FPendingSettlement
	{
		FGuid SaveRequestId;
		FPRChapterPersistenceData Expected;
		FPRChapterPersistenceData Target;
		FPRChapterCompletionResult Completion;
		bool bPending = false;
	};

	/** Closed runtime selection. It never accepts a caller supplied chapter, asset, or proof id. */
	struct FActiveChapterDefinition
	{
		FPrimaryAssetId ChapterId;
		FPrimaryAssetId RoomRegistryId;
		FPrimaryAssetId EnemyRegistryId;
		FPrimaryAssetId FinalRoomId;
		FName ContentId;
		FName BossId;
		FName ProofId;
		bool bIsWarden = false;
	};

	void HandleRunStateChanged(const struct FPRRunRuntimeState& State);
	void HandleRoomCompleted(const struct FPRRoomSequenceCompleted& Completion);
	void HandleRoomEventResolved(const struct FPRRoomEventResult& Result);
	void HandleBossCompleted(const struct FPRPrototypeRunResult& Completion);
	void HandleAccountDeleted(const struct FPRAccountDeletedEvent& Event);
	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void BindWorld(UWorld* World);
	void UnbindWorld(UWorld* World);
	void PublishState();
	void ResetTransientSession();
	bool ConfigureActiveContent();
	bool SelectActiveDefinition(FActiveChapterDefinition& OutDefinition) const;
	bool IsActiveSequence(const struct FPRRoomSequenceCompleted& Completion) const;
	bool BeginSettlement();
	bool SubmitPendingSettlement();
	void RefreshWardenStoryProjection();
	void ClearWardenPresentation();
	void EnsureWardenPresentation();
	bool IsExpectedBossCompletion(const struct FPRPrototypeRunResult& Completion) const;

	FPRChapterSnapshot Snapshot;
	FPRChapterCompletionResult LatestCompletion;
	bool bHasLatestCompletion = false;
	FPRChapterStateChangedNative StateChanged;
	FPRChapterCompletionNative ChapterCompleted;
	FGuid FrozenRunId;
	FGuid FrozenAccountId;
	int32 FrozenSeed = 0;
	bool bRoomSequenceVerified = false;
	bool bBossVerified = false;
	FGuid VerifiedBossCompletionId;
	FGuid VerifiedBossSpawnId;
	FActiveChapterDefinition ActiveDefinition;
	TWeakObjectPtr<class UPRWardenChapterWidget> WardenOverlay;
	FDelegateHandle RunStateHandle;
	FDelegateHandle RoomCompletedHandle;
	FDelegateHandle RoomEventHandle;
	FDelegateHandle AccountDeletedHandle;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle BossCompletedHandle;
	FDelegateHandle WorldInitHandle;
	FDelegateHandle WorldCleanupHandle;
	TWeakObjectPtr<UWorld> BoundWorld;
	FPendingSettlement PendingSettlement;
};
