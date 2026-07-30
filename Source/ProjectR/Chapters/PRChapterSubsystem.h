// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Chapters/PRChapterTypes.h"
#include "Containers/Ticker.h"
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
	/** Fixed v0.6.2 fixture: persists only the Allocator/Warden prerequisite chain in isolated automation storage. */
	bool StageFixedPacifierPrerequisitesForAutomation();
	/** Fixed v0.7.0 fixture: persists only the Allocator/Warden/Pacifier prerequisite chain in isolated automation storage. */
	bool StageFixedAuditorPrerequisitesForAutomation();
	/** Re-selects only the fixed Auditor closure after its isolated prerequisite transaction commits and before the fixture starts a run. */
	bool RefreshFixedAuditorSelectionForAutomation();
	/** Fixed v0.6.2 persistence fixture: marks only the already-active Pacifier run's bounded completion facts. */
	bool StageFixedPacifierCompletionFactsForAutomation();
	/** Fixed v0.7.0 persistence fixture: marks only the already-active Auditor run's bounded completion facts. */
	bool StageFixedAuditorCompletionFactsForAutomation();
	/** Read-only fixed diagnostics for the Pacifier settlement acceptance runner. */
	void GetFixedPacifierSettlementDiagnosticsForAutomation(
		bool& bOutRoomVerified,
		bool& bOutBossVerified,
		bool& bOutAccountDeletedVerified,
		bool& bOutSettlementRequested,
		bool& bOutSettlementPending) const;
	/** Read-only fixed diagnostics for the Auditor settlement acceptance runner. */
	void GetFixedAuditorSettlementDiagnosticsForAutomation(
		bool& bOutRoomVerified,
		bool& bOutBossVerified,
		bool& bOutAccountDeletedVerified,
		bool& bOutSettlementRequested,
		bool& bOutSettlementPending) const;
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
		enum class EFixedContent : uint8
		{
			Allocator,
			Warden,
			Pacifier,
			Auditor
		};

		FPrimaryAssetId ChapterId;
		FPrimaryAssetId RoomRegistryId;
		FPrimaryAssetId EnemyRegistryId;
		FPrimaryAssetId FinalRoomId;
		FName ContentId;
		FName BossId;
		FName ProofId;
		EFixedContent FixedContent = EFixedContent::Allocator;
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
	bool SelectActiveDefinition(FActiveChapterDefinition& OutDefinition, FName& OutFailureReason) const;
	bool IsActiveSequence(const struct FPRRoomSequenceCompleted& Completion) const;
	void TryBeginSettlement();
	bool BeginSettlement();
	bool SubmitPendingSettlement();
	void ScheduleDeferredSettlementSubmit();
	bool TickDeferredSettlementSubmit(float DeltaSeconds);
	static bool IsFixedProofChainValid(const FPRChapterPersistenceData& Persistence);
	void RefreshWardenStoryProjection();
	void RefreshPacifierStoryProjection();
	void RefreshAuditorStoryProjection();
	void ClearWardenPresentation();
	void ClearPacifierPresentation();
	void ClearAuditorPresentation();
	void EnsureWardenPresentation();
	void EnsurePacifierPresentation();
	void EnsureAuditorPresentation();
	bool IsExpectedBossCompletion(const struct FPRPrototypeRunResult& Completion) const;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FPRPacifierFoundationFixedContractTest;
#endif

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
	bool bAccountDeletedVerified = false;
	FGuid VerifiedBossCompletionId;
	FGuid VerifiedBossSpawnId;
	FActiveChapterDefinition ActiveDefinition;
	TWeakObjectPtr<class UPRWardenChapterWidget> WardenOverlay;
	TWeakObjectPtr<class UPRPacifierChapterWidget> PacifierOverlay;
	TWeakObjectPtr<class UPRAuditorChapterWidget> AuditorOverlay;
	FDelegateHandle RunStateHandle;
	FDelegateHandle RoomCompletedHandle;
	FDelegateHandle RoomEventHandle;
	FDelegateHandle AccountDeletedHandle;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle BossCompletedHandle;
	FDelegateHandle WorldInitHandle;
	FDelegateHandle WorldCleanupHandle;
	FTSTicker::FDelegateHandle SettlementRetryTickerHandle;
	TWeakObjectPtr<UWorld> BoundWorld;
	FPendingSettlement PendingSettlement;
	/** Frozen AccountDeleted intent waiting for an upstream bounded save consumer to release SaveSubsystem. */
	bool bSettlementRequested = false;
};
