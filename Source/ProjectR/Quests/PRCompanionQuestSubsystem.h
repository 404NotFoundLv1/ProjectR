// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Quests/PRCompanionQuestTypes.h"
#include "PRCompanionQuestSubsystem.generated.h"
class UPRSaveSubsystem;
class UPRCompanionQuestRegistryDataAsset;
class UPRCompanionSubsystem;
class UPRProgressionSubsystem;
class UPRRunStateSubsystem;
class UPRRoomSubsystem;
class UPRDivergenceSubsystem;
struct FPRSaveOperationEvent;
UCLASS()
class PROJECTR_API UPRCompanionQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	bool GetSnapshot(FPRCompanionQuestSnapshot& OutSnapshot) const;
	bool GetEntitlementSnapshot(FPRCompanionQuestEntitlementSnapshot& OutSnapshot) const;
	bool IsQuestCompleted(FName QuestId) const;
	EPRCompanionQuestOperationResult RequestActivateQuest(FName QuestId, FGuid& OutRequestId);
	EPRCompanionQuestOperationResult RetryPendingPersistence();
	EPRCompanionQuestOperationResult ConfirmNullRememberMeRecordsViewed();
	FPRCompanionQuestStateChangedNative& OnStateChanged() { return StateChanged; }
	FPRCompanionQuestOperationNative& OnOperation() { return Operation; }
	FPRCompanionQuestCompletedNative& OnQuestCompleted() { return QuestCompleted; }
	FPRCompanionQuestEntitlementsChangedNative& OnEntitlementsChanged() { return EntitlementsChanged; }
	FPRCompanionQuestDialogueNative& OnDialogue() { return Dialogue; }
private:
	void Refresh();
	void HandleSaveOperation(const FPRSaveOperationEvent& Event);
	void HandleAccountDeleted(const struct FPRAccountDeletedEvent& Event);
	void HandleRoomEventResolved(const struct FPRRoomEventResult& Event);
	void HandleDivergenceResult(const struct FPRDivergenceResult& Event);
	void BeginCompletion(FName QuestId, const FGuid& EvidenceId, const FGuid& AccountId, int32 ArchivedAccountCount);
	void PublishVerifiedCompletion(FName QuestId);
	const UPRCompanionQuestRegistryDataAsset* GetRegistry() const;
	EPRCompanionQuestState GetState(FName QuestId, const FPRCompanionQuestPersistenceData& Persistence) const;
	bool IsEligible(const class UPRCompanionQuestDataAsset& Quest) const;
	void PublishOperation(FName QuestId, const FGuid& RequestId, EPRCompanionQuestOperationResult Result);
	TWeakObjectPtr<UPRSaveSubsystem> SaveSubsystem;
	TWeakObjectPtr<UPRCompanionSubsystem> CompanionSubsystem;
	TWeakObjectPtr<UPRProgressionSubsystem> ProgressionSubsystem;
	TWeakObjectPtr<UPRRunStateSubsystem> RunStateSubsystem;
	TWeakObjectPtr<UPRRoomSubsystem> RoomSubsystem;
	TWeakObjectPtr<UPRDivergenceSubsystem> DivergenceSubsystem;
	TSoftObjectPtr<UPRCompanionQuestRegistryDataAsset> RegistryAsset;
	FDelegateHandle SaveOperationHandle;
	FDelegateHandle AccountDeletedHandle;
	FDelegateHandle RoomEventHandle;
	FDelegateHandle DivergenceResultHandle;
	FGuid AxiomRescueEvidenceId;
	FGuid PendingSaveRequestId;
	FGuid PendingOperationId;
	FName PendingQuestId;
	FPRCompanionQuestPersistenceData PendingPersistence;
	enum class EPendingTransaction : uint8 { None, Activation, Completion };
	EPendingTransaction PendingTransaction = EPendingTransaction::None;
	FPRCompanionQuestSnapshot Snapshot;
	FPRCompanionQuestStateChangedNative StateChanged;
	FPRCompanionQuestOperationNative Operation;
	FPRCompanionQuestCompletedNative QuestCompleted;
	FPRCompanionQuestEntitlementsChangedNative EntitlementsChanged;
	FPRCompanionQuestDialogueNative Dialogue;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FPRCompanionQuestLifecycleTest;
#endif
};
