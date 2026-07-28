// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Memory/PRMemoryTypes.h"
#include "Memory/PRMemorySummaryBuilder.h"
#include "Memory/PRPostRunDialogueProvider.h"
#include "Save/PRSaveTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"

#include "PRMemorySubsystem.generated.h"

class UPRSaveSubsystem;
class UPRProgressionSubsystem;
class UPRRunStateSubsystem;
class UPRRoomSubsystem;
class UPRCompanionQuestSubsystem;
class UPRDivergenceSubsystem;
class UPRCompanionDialogueSubsystem;
class UPRMemoryRegistryDataAsset;

/** Sole owner of v0.5.2 bounded memory summaries, local provider fallback, and persistence retry. */
UCLASS()
class PROJECTR_API UPRMemorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	bool GetSnapshot(FPRMemorySnapshot& OutSnapshot) const;
	bool GetLatestSummary(FPRMemorySummary& OutSummary) const;
	EPRMemoryOperationResult RetryPendingPersistence();
	EPRMemoryOperationResult SubmitLatestPlayerOption(EPRMemoryPlayerOptionSlot Slot);
	FPRMemoryStateChangedNative& OnStateChanged() { return StateChanged; }
	FPRMemorySummaryReadyNative& OnSummaryReady() { return SummaryReady; }
	FPRMemoryOperationNative& OnOperation() { return Operation; }

private:
	void HandleAccountDeleted(const struct FPRAccountDeletedEvent& Event);
	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	void HandleRoomEventResolved(const struct FPRRoomEventResult& Event);
	void HandleDivergenceResult(const struct FPRDivergenceResult& Event);
	void HandleQuestCompleted(FName QuestId);
	void BindWorld(UWorld* World);
	void UnbindWorld(UWorld* World);
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void HandleDialogueResult(const struct FPRDialogueResult& Event);
	void BeginFromAccountRecord(const struct FPRAccountRecord& Record);
	void BeginProvider(const struct FPRAccountRecord& Record, FPRMemorySummary&& Draft);
	void HandleProviderCandidate(const FGuid& ResponseRequestId, const FPRPostRunDialogueCandidate& Candidate, const TArray<FName>& WireFields);
	void CompleteWithFallback(FName ReasonId);
	void BeginPersistence();
	bool BuildPendingPersistence();
	void PublishState();
	void RefreshSnapshotDisplayProjection();
	void PublishOperation(EPRMemoryOperationResult Result, FName ReasonId);
	void ResetRuntimeFacts();
	void CancelProvider();
	bool HandleProviderTimeout(float DeltaSeconds);
	bool HandleDeferredPersistence(float DeltaSeconds);
	const UPRMemoryRegistryDataAsset* GetRegistry() const;
	void ApplyDialogueResult(const FPRPostRunDialogueResult& Result);

	TWeakObjectPtr<UPRSaveSubsystem> SaveSubsystem;
	TWeakObjectPtr<UPRProgressionSubsystem> ProgressionSubsystem;
	TWeakObjectPtr<UPRRunStateSubsystem> RunStateSubsystem;
	TWeakObjectPtr<UPRRoomSubsystem> RoomSubsystem;
	TWeakObjectPtr<UPRDivergenceSubsystem> DivergenceSubsystem;
	TWeakObjectPtr<UPRCompanionQuestSubsystem> QuestSubsystem;
	TWeakObjectPtr<UPRCompanionDialogueSubsystem> DialogueSubsystem;
	TSoftObjectPtr<UPRMemoryRegistryDataAsset> RegistryAsset;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle AccountDeletedHandle;
	FDelegateHandle RoomEventHandle;
	FDelegateHandle DivergenceResultHandle;
	FDelegateHandle QuestCompletedHandle;
	FDelegateHandle DialogueResultHandle;
	FDelegateHandle PostWorldInitializationHandle;
	FDelegateHandle WorldCleanupHandle;
	FTSTicker::FDelegateHandle ProviderTimeoutHandle;
	FTSTicker::FDelegateHandle DeferredPersistenceHandle;
	FPRMemorySummaryBuilder SummaryBuilder;
	TUniquePtr<IPRPostRunDialogueProvider> Provider;
	FPRMemorySnapshot Snapshot;
	FPRMemorySummary PendingSummary;
	FPRMemoryPersistenceData PendingExpectedMemory;
	FPRMemoryPersistenceData PendingTargetMemory;
	FPRProgressionPersistenceData PendingExpectedProgression;
	FPRProgressionPersistenceData PendingTargetProgression;
	FGuid PendingProviderRequestId;
	FGuid PendingSaveRequestId;
	FGuid PendingSummaryId;
	bool bPendingFragmentAward = false;
	bool bHasFrozenTransaction = false;
	// The deterministic mock may invoke its callback synchronously.  Keep its
	// instance alive until BeginRequest has returned before releasing it.
	bool bProviderCallInProgress = false;
	FPRMemoryStateChangedNative StateChanged;
	FPRMemorySummaryReadyNative SummaryReady;
	FPRMemoryOperationNative Operation;
};
