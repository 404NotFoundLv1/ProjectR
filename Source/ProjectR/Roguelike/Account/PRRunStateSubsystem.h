// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Roguelike/Account/PRAccountRuntimeTypes.h"
#include "Roguelike/Account/PRRunSummaryBuilder.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRRunStateSubsystem.generated.h"

class UPRAccountIdentityRegistryDataAsset;
class UPRSaveSubsystem;
class UPRRoomSubsystem;
class UPRCombatSubsystem;
class UPRQTESubsystem;
class UPRBossSubsystem;

/** Sole owner of the active account and bounded run finalization lifecycle. */
UCLASS()
class PROJECTR_API UPRRunStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	EPRAccountOperationResult RequestCreateAccount(FPrimaryAssetId IdentityId, FGuid& OutRequestId);
	EPRAccountOperationResult RequestStartRun(int32 Seed, FGuid& OutRequestId);
	EPRAccountOperationResult RetryPendingPersistence(FGuid& OutRequestId);
	EPRAccountOperationResult RetryReturnToReality();
	FPRRunRuntimeState GetRunRuntimeState() const;
	bool GetActiveAccountSnapshot(FPRActiveAccountSaveData& OutAccount) const;
	void GetGraveyardSnapshot(TArray<FPRAccountRecord>& OutGraveyard) const;
	bool GetLastRunSummary(FPRRunSummary& OutSummary) const;
	FPRRunStateChangedNative& OnRunStateChanged();
	FPRAccountOperationNative& OnAccountOperation();
	FPRAccountDeletedNative& OnAccountDeleted();

#if WITH_DEV_AUTOMATION_TESTS
	/** Value-only automation seam; it never accesses physical user slots. */
	bool InjectAccountPersistenceForAutomation(const FPRAccountPersistenceData& Persistence);
	/** Fixed-value PIE seam; accepts no slots, paths, objects or event logs. */
	bool FinalizeActiveAccountForAutomation(EPRAccountTerminationReason Reason);
#endif

private:
	enum class EPendingKind : uint8 { None, Create, Start, Finalize };
	struct FPendingTransaction
	{
		EPendingKind Kind = EPendingKind::None;
		FGuid RequestId;
		FGuid SaveRequestId;
		FPRAccountPersistenceData TargetPersistence;
		FPRAccountRecord DeletedRecord;
		bool bHasDeletedRecord = false;
	};

	void BindGameInstanceSources();
	void UnbindGameInstanceSources();
	void BindWorld(UWorld* World);
	void UnbindWorld(UWorld* World);
	void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleSaveOperation(const struct FPRSaveOperationEvent& Event);
	void HandleRoomSequenceCompleted(const struct FPRRoomSequenceCompleted& Completion);
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void HandleQTEResult(const struct FPRQTEResult& Result);
	void HandleBossCompleted(const struct FPRPrototypeRunResult& Result);
	void HandleDivergenceResult(const struct FPRDivergenceResult& Result);
	void ResolveDeferredDeath();
	void BeginFinalization(EPRAccountTerminationReason Reason);
	bool StageAndRequest(FPendingTransaction& Transaction);
	void CompleteCreate();
	void CompleteStart();
	void CompleteFinalization();
	void PublishOperation(EPRAccountOperationType Operation, EPRAccountOperationResult Result, const FGuid& RequestId, const FGuid& SaveRequestId = FGuid());
	void SetState(EPRRunLifecycleState NewState);
	void RefreshRuntimeFromProfile();
	void CaptureFinalValues();
	UPRAccountIdentityRegistryDataAsset* LoadRegistry() const;
	int64 GetUtcNow() const;
	double GetWorldTimeSeconds() const;

	TWeakObjectPtr<UPRSaveSubsystem> SaveSubsystem;
	TWeakObjectPtr<UPRRoomSubsystem> RoomSubsystem;
	TWeakObjectPtr<UWorld> BoundWorld;
	TWeakObjectPtr<UPRCombatSubsystem> BoundCombat;
	TWeakObjectPtr<UPRQTESubsystem> BoundQTE;
	TWeakObjectPtr<UPRBossSubsystem> BoundBoss;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle RoomCompletedHandle;
	FDelegateHandle DivergenceResultHandle;
	FDelegateHandle PostWorldInitializationHandle;
	FDelegateHandle WorldCleanupHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle QTEResultHandle;
	FDelegateHandle BossCompletedHandle;
	FTimerHandle DeferredDeathTimer;
	FPRRunRuntimeState RuntimeState;
	FPRRunSummaryBuilder SummaryBuilder;
	FPRRunSummary LastRunSummary;
	bool bHasLastRunSummary = false;
	bool bDeathPending = false;
	FPRRunDeathCause PendingDeathCause;
	FPendingTransaction Pending;
	FPRRunStateChangedNative RunStateChanged;
	FPRAccountOperationNative AccountOperation;
	FPRAccountDeletedNative AccountDeleted;
};
