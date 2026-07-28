// Copyright Epic Games, Inc. All Rights Reserved.
#include "Quests/PRCompanionQuestSubsystem.h"
#include "Quests/PRCompanionQuestRegistryDataAsset.h"
#include "Quests/PRCompanionQuestDataAsset.h"
#include "Quests/PRCompanionQuestDialogueProvider.h"
#include "Quests/PRCompanionQuestEvidence.h"
#include "Save/PRSaveSubsystem.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Roguelike/Progression/PRProgressionSubsystem.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Divergence/PRDivergenceSubsystem.h"
#include "Engine/AssetManager.h"

void UPRCompanionQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegistryAsset = TSoftObjectPtr<UPRCompanionQuestRegistryDataAsset>(FSoftObjectPath(TEXT("/Game/ProjectR/Data/Quests/DA_CompanionQuestRegistry.DA_CompanionQuestRegistry")));
	SaveSubsystem = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>();
	CompanionSubsystem = GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>();
	ProgressionSubsystem = GetGameInstance()->GetSubsystem<UPRProgressionSubsystem>();
	RunStateSubsystem = GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>();
	RoomSubsystem = GetGameInstance()->GetSubsystem<UPRRoomSubsystem>();
	DivergenceSubsystem = GetGameInstance()->GetSubsystem<UPRDivergenceSubsystem>();
	if (SaveSubsystem.IsValid()) SaveOperationHandle = SaveSubsystem->OnSaveOperation().AddUObject(this, &UPRCompanionQuestSubsystem::HandleSaveOperation);
	if (RunStateSubsystem.IsValid()) AccountDeletedHandle = RunStateSubsystem->OnAccountDeleted().AddUObject(this, &UPRCompanionQuestSubsystem::HandleAccountDeleted);
	if (RoomSubsystem.IsValid()) RoomEventHandle = RoomSubsystem->OnRoomEventResolved().AddUObject(this, &UPRCompanionQuestSubsystem::HandleRoomEventResolved);
	if (DivergenceSubsystem.IsValid()) DivergenceResultHandle = DivergenceSubsystem->OnDivergenceResult().AddUObject(this, &UPRCompanionQuestSubsystem::HandleDivergenceResult);
	Refresh();
}
void UPRCompanionQuestSubsystem::Deinitialize()
{
	if (SaveSubsystem.IsValid()) SaveSubsystem->OnSaveOperation().Remove(SaveOperationHandle);
	if (RunStateSubsystem.IsValid()) RunStateSubsystem->OnAccountDeleted().Remove(AccountDeletedHandle);
	if (RoomSubsystem.IsValid()) RoomSubsystem->OnRoomEventResolved().Remove(RoomEventHandle);
	if (DivergenceSubsystem.IsValid()) DivergenceSubsystem->OnDivergenceResult().Remove(DivergenceResultHandle);
	SaveOperationHandle.Reset(); AccountDeletedHandle.Reset(); RoomEventHandle.Reset(); DivergenceResultHandle.Reset(); SaveSubsystem.Reset(); CompanionSubsystem.Reset(); ProgressionSubsystem.Reset(); RunStateSubsystem.Reset(); RoomSubsystem.Reset(); DivergenceSubsystem.Reset(); AxiomRescueEvidenceId.Invalidate(); PendingSaveRequestId.Invalidate(); PendingOperationId.Invalidate(); PendingQuestId = NAME_None; PendingPersistence = FPRCompanionQuestPersistenceData(); PendingTransaction = EPendingTransaction::None; Snapshot = FPRCompanionQuestSnapshot();
	Super::Deinitialize();
}
const UPRCompanionQuestRegistryDataAsset* UPRCompanionQuestSubsystem::GetRegistry() const { return RegistryAsset.LoadSynchronous(); }
EPRCompanionQuestState UPRCompanionQuestSubsystem::GetState(FName QuestId, const FPRCompanionQuestPersistenceData& Persistence) const
{
	if (QuestId == PendingQuestId)
	{
		return PendingSaveRequestId.IsValid() ? EPRCompanionQuestState::PersistencePending : PendingOperationId.IsValid() ? EPRCompanionQuestState::ReadyToRetry : EPRCompanionQuestState::Locked;
	}
	for (const FPRCompanionQuestRecord& Record : Persistence.Records) if (Record.QuestId == QuestId) return Record.State == EPRCompanionQuestPersistentState::Completed ? EPRCompanionQuestState::Completed : Record.State == EPRCompanionQuestPersistentState::Active ? EPRCompanionQuestState::Active : EPRCompanionQuestState::Available;
	if (!Snapshot.bProfileLoaded) return EPRCompanionQuestState::Locked;
	if (const UPRCompanionQuestDataAsset* Quest = GetRegistry() ? GetRegistry()->FindQuest(QuestId) : nullptr) return IsEligible(*Quest) ? EPRCompanionQuestState::Available : EPRCompanionQuestState::Locked;
	return EPRCompanionQuestState::Locked;
}
bool UPRCompanionQuestSubsystem::IsEligible(const UPRCompanionQuestDataAsset& Quest) const
{
	if (!CompanionSubsystem.IsValid() || !ProgressionSubsystem.IsValid()) return false;
	if (CompanionSubsystem->GetSyncState().PrimaryCompanionId != Quest.CompanionId) return false;
	FPRCompanionRelationshipRecord Relationship;
	if (!CompanionSubsystem->GetRelationshipSnapshot(Quest.CompanionId, Relationship) || Relationship.State.Trust < 60) return false;
	const FPrimaryAssetId Story(TEXT("ProgressionNode"), TEXT("BondStory"));
	if (!ProgressionSubsystem->IsNodeUnlocked(Story)) return false;
	const bool bQTE = Quest.EntitlementId.ToString().StartsWith(TEXT("QTE:"));
	return ProgressionSubsystem->IsNodeUnlocked(FPrimaryAssetId(TEXT("ProgressionNode"), bQTE ? TEXT("BondAdvancedCombo") : TEXT("BondVoice")));
}
void UPRCompanionQuestSubsystem::Refresh()
{
	Snapshot = FPRCompanionQuestSnapshot(); Snapshot.bRegistryReady = GetRegistry() && GetRegistry()->IsRegistryReady();
	FPRCompanionQuestPersistenceData Persistence;
	Snapshot.bProfileLoaded = SaveSubsystem.IsValid() && SaveSubsystem->GetCompanionQuestPersistenceSnapshot(Persistence);
	if (Snapshot.bRegistryReady) for (const TSoftObjectPtr<UPRCompanionQuestDataAsset>& Ref : GetRegistry()->Quests) if (const UPRCompanionQuestDataAsset* Quest = Ref.LoadSynchronous()) { FPRCompanionQuestEntrySnapshot& Entry = Snapshot.Entries.AddDefaulted_GetRef(); Entry.QuestId=Quest->QuestId; Entry.CompanionId=Quest->CompanionId; Entry.State=GetState(Quest->QuestId, Persistence); Entry.EntitlementId=Quest->EntitlementId; Entry.DisplayName=Quest->DisplayName; Entry.ObjectiveText=Quest->ObjectiveText; }
	Snapshot.Entries.Sort([](const FPRCompanionQuestEntrySnapshot& A, const FPRCompanionQuestEntrySnapshot& B) { return A.QuestId.LexicalLess(B.QuestId); }); StateChanged.Broadcast(Snapshot);
}
bool UPRCompanionQuestSubsystem::GetSnapshot(FPRCompanionQuestSnapshot& OutSnapshot) const { OutSnapshot = Snapshot; return Snapshot.bRegistryReady; }
bool UPRCompanionQuestSubsystem::IsQuestCompleted(FName QuestId) const { return Snapshot.Entries.ContainsByPredicate([QuestId](const FPRCompanionQuestEntrySnapshot& Entry) { return Entry.QuestId == QuestId && Entry.State == EPRCompanionQuestState::Completed; }); }
bool UPRCompanionQuestSubsystem::GetEntitlementSnapshot(FPRCompanionQuestEntitlementSnapshot& OutSnapshot) const { OutSnapshot.EntitlementIds.Reset(); for (const FPRCompanionQuestEntrySnapshot& Entry : Snapshot.Entries) if (Entry.State == EPRCompanionQuestState::Completed) OutSnapshot.EntitlementIds.Add(Entry.EntitlementId); return Snapshot.bRegistryReady; }
void UPRCompanionQuestSubsystem::PublishOperation(FName QuestId, const FGuid& RequestId, EPRCompanionQuestOperationResult Result) { FPRCompanionQuestOperationEvent Event; Event.RequestId=RequestId; Event.QuestId=QuestId; Event.Result=Result; Operation.Broadcast(Event); }
EPRCompanionQuestOperationResult UPRCompanionQuestSubsystem::RequestActivateQuest(FName QuestId, FGuid& OutRequestId)
{
	OutRequestId=FGuid::NewGuid(); if (!Snapshot.bRegistryReady) { PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::RejectedRegistryUnavailable); return EPRCompanionQuestOperationResult::RejectedRegistryUnavailable; }
	if (!Snapshot.bProfileLoaded) { PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::RejectedNoProfile); return EPRCompanionQuestOperationResult::RejectedNoProfile; }
	if (PendingSaveRequestId.IsValid()) { PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::RejectedBusy); return EPRCompanionQuestOperationResult::RejectedBusy; }
	const UPRCompanionQuestDataAsset* Quest=GetRegistry()->FindQuest(QuestId); if (!Quest) { PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::RejectedUnknownQuest); return EPRCompanionQuestOperationResult::RejectedUnknownQuest; }
	FPRCompanionQuestPersistenceData Persistence; if (!SaveSubsystem->GetCompanionQuestPersistenceSnapshot(Persistence)) return EPRCompanionQuestOperationResult::RejectedNoProfile;
	FPRCompanionQuestRecord* Record=Persistence.Records.FindByPredicate([QuestId](const FPRCompanionQuestRecord& Value){return Value.QuestId==QuestId;});
	if (Record && Record->State==EPRCompanionQuestPersistentState::Completed) { PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::AlreadyCompleted); return EPRCompanionQuestOperationResult::AlreadyCompleted; }
	if (Record && Record->State==EPRCompanionQuestPersistentState::Active) { PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::AlreadyActive); return EPRCompanionQuestOperationResult::AlreadyActive; }
	if (!IsEligible(*Quest)) { PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::RejectedNotEligible); return EPRCompanionQuestOperationResult::RejectedNotEligible; }
	if (!Record) { Record=&Persistence.Records.AddDefaulted_GetRef(); Record->QuestId=QuestId; }
	Record->State=EPRCompanionQuestPersistentState::Active; FPRCompanionQuestPersistenceContract::Normalize(Persistence);
	if (!SaveSubsystem->StageCompanionQuestPersistence(Persistence) || SaveSubsystem->RequestSaveCurrentProfile(PendingSaveRequestId)!=EPRSaveRequestStatus::Started) { PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::PersistenceFailed); return EPRCompanionQuestOperationResult::PersistenceFailed; }
	PendingOperationId=OutRequestId; PendingQuestId=QuestId; PendingPersistence=Persistence; PendingTransaction=EPendingTransaction::Activation; Refresh(); PublishOperation(QuestId,OutRequestId,EPRCompanionQuestOperationResult::Started); return EPRCompanionQuestOperationResult::Started;
}
EPRCompanionQuestOperationResult UPRCompanionQuestSubsystem::RetryPendingPersistence()
{
	if (!PendingOperationId.IsValid() || PendingSaveRequestId.IsValid() || PendingQuestId.IsNone()) return EPRCompanionQuestOperationResult::RejectedNoPendingPersistence;
	if (!SaveSubsystem.IsValid() || !SaveSubsystem->StageCompanionQuestPersistence(PendingPersistence) || SaveSubsystem->RequestSaveCurrentProfile(PendingSaveRequestId) != EPRSaveRequestStatus::Started)
	{
		PublishOperation(PendingQuestId, PendingOperationId, EPRCompanionQuestOperationResult::PersistenceFailed);
		return EPRCompanionQuestOperationResult::PersistenceFailed;
	}
	Refresh();
	PublishOperation(PendingQuestId, PendingOperationId, EPRCompanionQuestOperationResult::Started);
	return EPRCompanionQuestOperationResult::Started;
}
EPRCompanionQuestOperationResult UPRCompanionQuestSubsystem::ConfirmNullRememberMeRecordsViewed()
{
	const FName QuestId(TEXT("Quest.Null.RememberMe"));
	if (!RunStateSubsystem.IsValid() || !Snapshot.bProfileLoaded || !Snapshot.bRegistryReady) return EPRCompanionQuestOperationResult::RejectedNoProfile;
	const UPRCompanionQuestDataAsset* Quest = GetRegistry()->FindQuest(QuestId);
	if (!Quest || !IsEligible(*Quest)) return EPRCompanionQuestOperationResult::RejectedNotEligible;
	TArray<FPRAccountRecord> Records; RunStateSubsystem->GetGraveyardSnapshot(Records);
	if (!FPRCompanionQuestEvidenceContract::HasFiveUniqueGraveyardRecords(Records)) return EPRCompanionQuestOperationResult::RejectedNotEligible;
	TSet<FGuid> UniqueRecords;
	for (const FPRAccountRecord& Record : Records) if (Record.RecordId.IsValid()) UniqueRecords.Add(Record.RecordId);
	BeginCompletion(QuestId, *UniqueRecords.CreateConstIterator(), FGuid(), UniqueRecords.Num());
	return PendingSaveRequestId.IsValid() ? EPRCompanionQuestOperationResult::Started : EPRCompanionQuestOperationResult::RejectedNotEligible;
}
void UPRCompanionQuestSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if (Event.Operation != EPRSaveOperationType::Save) { if (Event.Result==EPRSaveResult::Success || Event.Result==EPRSaveResult::RecoveredFromAlternate) Refresh(); return; }
	if (Event.RequestId != PendingSaveRequestId) return;
	const bool bSuccess=Event.Result==EPRSaveResult::Success || Event.Result==EPRSaveResult::RecoveredFromAlternate;
	const FName QuestId=PendingQuestId; const FGuid OperationId=PendingOperationId; const EPendingTransaction Transaction=PendingTransaction; PendingSaveRequestId.Invalidate();
	if (bSuccess)
	{
		PendingOperationId.Invalidate(); PendingQuestId=NAME_None; PendingPersistence=FPRCompanionQuestPersistenceData(); PendingTransaction=EPendingTransaction::None;
	}
	Refresh(); PublishOperation(QuestId,OperationId,bSuccess?EPRCompanionQuestOperationResult::Succeeded:EPRCompanionQuestOperationResult::PersistenceFailed);
	if (bSuccess && Transaction == EPendingTransaction::Completion && IsQuestCompleted(QuestId)) PublishVerifiedCompletion(QuestId);
}

void UPRCompanionQuestSubsystem::PublishVerifiedCompletion(const FName QuestId)
{
	QuestCompleted.Broadcast(QuestId);
	FPRCompanionQuestEntitlementSnapshot Entitlements;
	GetEntitlementSnapshot(Entitlements);
	Entitlements.EntitlementIds.Sort(FNameLexicalLess());
	EntitlementsChanged.Broadcast(Entitlements);
	FPRCompanionQuestDialogueLine CompletionLine;
	if (FPRCompanionQuestDialogueProvider::GetCompletionLine(QuestId, CompletionLine))
	{
		Dialogue.Broadcast(CompletionLine);
	}
}

void UPRCompanionQuestSubsystem::BeginCompletion(FName QuestId, const FGuid& EvidenceId, const FGuid& AccountId, const int32 ArchivedAccountCount)
{
	if (!SaveSubsystem.IsValid() || PendingSaveRequestId.IsValid() || IsQuestCompleted(QuestId)) return;
	FPRCompanionQuestPersistenceData Persistence;
	if (!SaveSubsystem->GetCompanionQuestPersistenceSnapshot(Persistence)) return;
	FPRCompanionQuestRecord* Record = Persistence.Records.FindByPredicate([QuestId](const FPRCompanionQuestRecord& Value) { return Value.QuestId == QuestId; });
	if (!Record || Record->State != EPRCompanionQuestPersistentState::Active) return;
	Record->State = EPRCompanionQuestPersistentState::Completed;
	Record->EvidenceIds.Add(EvidenceId); Record->LastAccountId = AccountId; Record->ArchivedAccountCount = ArchivedAccountCount;
	Record->CompletionSequence = FMath::Max<int64>(1, Record->CompletionSequence + 1);
	FPRCompanionQuestPersistenceContract::Normalize(Persistence);
	FGuid RequestId;
	if (SaveSubsystem->StageCompanionQuestPersistence(Persistence) && SaveSubsystem->RequestSaveCurrentProfile(RequestId) == EPRSaveRequestStatus::Started)
	{
		PendingSaveRequestId = RequestId; PendingOperationId = FGuid::NewGuid(); PendingQuestId = QuestId; PendingPersistence = Persistence; PendingTransaction = EPendingTransaction::Completion; Refresh();
	}
}

void UPRCompanionQuestSubsystem::HandleAccountDeleted(const FPRAccountDeletedEvent& Event)
{
	const int32 ArchivedAccountCount = static_cast<int32>(FMath::Clamp<int64>(Event.Record.GraveyardOrdinal, 0, FPRCompanionQuestPersistenceContract::MaxArchivedAccountCount));
	if (FPRCompanionQuestEvidenceContract::IsAxiomImperfectOptimum(Event.Record, AxiomRescueEvidenceId)) { BeginCompletion(TEXT("Quest.Axiom.ImperfectOptimum"), AxiomRescueEvidenceId, Event.Record.AccountId, ArchivedAccountCount); AxiomRescueEvidenceId.Invalidate(); }
	if (FPRCompanionQuestEvidenceContract::IsKindleNoRetreat(Event.Record)) BeginCompletion(TEXT("Quest.Kindle.NoRetreatLine"), Event.Record.RecordId, Event.Record.AccountId, ArchivedAccountCount);
	if (FPRCompanionQuestEvidenceContract::IsKindleLearnToRetreat(Event.Record)) BeginCompletion(TEXT("Quest.Kindle.LearnToRetreat"), Event.Record.RecordId, Event.Record.AccountId, ArchivedAccountCount);
	if (FPRCompanionQuestEvidenceContract::IsNullGarbageCollection(Event.Record)) BeginCompletion(TEXT("Quest.Null.GarbageCollection"), Event.Record.RecordId, Event.Record.AccountId, ArchivedAccountCount);
}

void UPRCompanionQuestSubsystem::HandleRoomEventResolved(const FPRRoomEventResult& Event)
{
	if (!CompanionSubsystem.IsValid()) return;
	if (FPRCompanionQuestEvidenceContract::IsAxiomLowProbabilitySample(Event, CompanionSubsystem->GetSyncState().PrimaryCompanionId)) BeginCompletion(TEXT("Quest.Axiom.LowProbabilitySample"), Event.ResolutionId, FGuid(), 0);
}

void UPRCompanionQuestSubsystem::HandleDivergenceResult(const FPRDivergenceResult& Event)
{
	if (FPRCompanionQuestEvidenceContract::IsAxiomRescueCandidate(Event)) AxiomRescueEvidenceId = Event.ResultId;
}
