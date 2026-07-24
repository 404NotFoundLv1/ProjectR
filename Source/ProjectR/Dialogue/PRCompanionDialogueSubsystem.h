// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialogue/PRDialogueTypes.h"
#include "Subsystems/WorldSubsystem.h"

#include "PRCompanionDialogueSubsystem.generated.h"

class APRPlayerCharacter;
class UPRCombatSubsystem;
class UPRCompanionDialogueComponent;
class UPRCompanionDialogueDataAsset;
class UPRCompanionDialogueRegistryDataAsset;
class UPRCompanionRuntimeSubsystem;
class UPRCompanionSubsystem;
class UPRQTESubsystem;
class UPRBossSubsystem;
class UPREnemySubsystem;
class UPRSaveSubsystem;

/** World-owned deterministic dialogue arbiter. It consumes published value events only. */
UCLASS()
class PROJECTR_API UPRCompanionDialogueSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsRegistryReady() const;
	FPRDialogueRuntimeState GetRuntimeState() const;
	bool SubmitChoice(bool bAnalyze);
	void CancelDialogue();
	FPRDialogueStateChangedNative& OnDialogueStateChanged();
	FPRDialogueResultNative& OnDialogueResult();

private:
	struct FQueuedDialogue
	{
		TWeakObjectPtr<UPRCompanionDialogueDataAsset> Asset;
		FPRDialogueRequest Request;
		double EnqueueTimeSeconds = 0.0;
	};

	void LoadFixedRegistry();
	void ReconcileInputBridge();
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void HandleQTEResult(const struct FPRQTEResult& Result);
	void HandleSupportEvent(const struct FPRCompanionSupportEvent& Event);
	void HandleBossState(const struct FPRBossRuntimeState& State);
	void HandleBossCompletion(const struct FPRPrototypeRunResult& Result);
	void HandleEnemyState(const struct FPREnemyRuntimeState& State);
	void HandlePrimarySyncChanged(const struct FPRPrimaryCompanionSyncChangedEvent& Event);
	bool Enqueue(EPRDialogueTriggerKind TriggerKind, const FGuid& EventId, double EventTimeSeconds);
	bool StartNext();
	void ScheduleStartNext(double DelaySeconds);
	void HandleDeferredStart();
	void ExpireActive();
	void TryScheduleSafeState();
	void HandleSafeStateTimer();
	void BroadcastState(EPRDialoguePresentationState NewState);
	void ClearActive();
	void ClearRuntime();
	UPRCompanionDialogueDataAsset* GetPrimaryDialogueAsset() const;
	bool HasLoadedProfile() const;
	bool IsRelationshipStateDifferent(const struct FPRCompanionRelationshipRecord& Left, const struct FPRCompanionRelationshipRecord& Right) const;
	double GetWorldTimeSeconds() const;

	TSoftObjectPtr<UPRCompanionDialogueRegistryDataAsset> RegistryAsset;
	TMap<FGameplayTag, TObjectPtr<UPRCompanionDialogueDataAsset>> LoadedAssets;
	TWeakObjectPtr<UPRCombatSubsystem> CombatSubsystem;
	TWeakObjectPtr<UPRQTESubsystem> QTESubsystem;
	TWeakObjectPtr<UPRCompanionRuntimeSubsystem> CompanionRuntimeSubsystem;
	TWeakObjectPtr<UPRBossSubsystem> BossSubsystem;
	TWeakObjectPtr<UPREnemySubsystem> EnemySubsystem;
	TWeakObjectPtr<UPRCompanionSubsystem> CompanionSubsystem;
	TWeakObjectPtr<UPRSaveSubsystem> SaveSubsystem;
	TWeakObjectPtr<UPRCompanionDialogueComponent> InputBridge;
	TWeakObjectPtr<APRPlayerCharacter> BoundDialoguePawn;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle QTEResultHandle;
	FDelegateHandle SupportEventHandle;
	FDelegateHandle BossStateHandle;
	FDelegateHandle BossCompletionHandle;
	FDelegateHandle EnemyStateHandle;
	FDelegateHandle PrimarySyncHandle;
	FTimerHandle InputReconcileTimer;
	FTimerHandle ActiveExpiryTimer;
	FTimerHandle SafeStateTimer;
	FTimerHandle DeferredStartTimer;
	TArray<FQueuedDialogue> Queue;
	TMap<FGuid, double> RecentEventTimes;
	TMap<FName, double> LineCooldownEnds;
	TArray<double> RecentStartTimes;
	TMap<FName, bool> KnownEnemyAlive;
	FQueuedDialogue ActiveDialogue;
	FPRDialogueRuntimeState RuntimeState;
	FPRDialogueStateChangedNative StateChanged;
	FPRDialogueResultNative ResultPublished;
	int32 LastBossPhaseSequence = 0;
	bool bBossCompleted = false;
	bool bRegistryReady = false;
	bool bHasBoundDialoguePawn = false;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FPRDialogueRuntimeChoiceTest;
#endif
};
