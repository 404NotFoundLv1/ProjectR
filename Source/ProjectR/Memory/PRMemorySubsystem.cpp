// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memory/PRMemorySubsystem.h"

#include "Dialogue/PRCompanionDialogueSubsystem.h"
#include "Memory/PRMemoryRegistryDataAsset.h"
#include "Memory/PRMemoryPersonaDataAsset.h"
#include "Memory/PRMockPostRunDialogueProvider.h"
#include "Memory/PRPostRunDialogueProvider.h"
#include "Memory/PRPostRunDialogueValidator.h"
#include "Quests/PRCompanionQuestSubsystem.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Roguelike/Progression/PRProgressionSubsystem.h"
#include "Divergence/PRDivergenceSubsystem.h"
#include "Save/PRSaveSubsystem.h"
#include "Engine/World.h"
#include "ProjectR.h"

namespace PRMemorySubsystemPrivate
{
	static bool IsSaveSuccess(const EPRSaveResult Result) { return Result == EPRSaveResult::Success || Result == EPRSaveResult::RecoveredFromAlternate; }
	static FName ProviderCompanionId(const FGameplayTag Tag)
	{
		return Tag == FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false) ? TEXT("Kindle") : Tag == FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false) ? TEXT("Null") : TEXT("Axiom");
	}
	static FPRPostRunDialogueCandidate BuildFallback(const FPRMemorySummary& Summary)
	{
		FPRPostRunDialogueCandidate Candidate;
		Candidate.SceneId = TEXT("post_run_summary");
		Candidate.CompanionId = ProviderCompanionId(Summary.CompanionId);
		if (Candidate.CompanionId == TEXT("Kindle"))
		{
			Candidate.EmotionId = TEXT("relieved"); Candidate.Summary = TEXT("The run is over. We keep the verified facts and face the next one prepared.");
			Candidate.PlayerOptionIds = { TEXT("kindle_steady"), TEXT("kindle_critique"), TEXT("kindle_thank") };
		}
		else if (Candidate.CompanionId == TEXT("Null"))
		{
			Candidate.EmotionId = TEXT("sincere"); Candidate.Summary = TEXT("Archived, bounded, and intact. That is enough to remember this run safely.");
			Candidate.PlayerOptionIds = { TEXT("null_promise"), TEXT("null_callout"), TEXT("null_analyze") };
		}
		else
		{
			Candidate.EmotionId = TEXT("analytical"); Candidate.Summary = TEXT("The verified outcome is archived. We can use its bounded facts for the next decision.");
			Candidate.PlayerOptionIds = { TEXT("axiom_reflect"), TEXT("axiom_challenge"), TEXT("axiom_deflect") };
		}
		return Candidate;
	}
}

void UPRMemorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegistryAsset = TSoftObjectPtr<UPRMemoryRegistryDataAsset>(FSoftObjectPath(TEXT("/Game/ProjectR/Data/Memory/DA_MemoryRegistry.DA_MemoryRegistry")));
	SaveSubsystem = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>();
	ProgressionSubsystem = GetGameInstance()->GetSubsystem<UPRProgressionSubsystem>();
	RunStateSubsystem = GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>();
	RoomSubsystem = GetGameInstance()->GetSubsystem<UPRRoomSubsystem>();
	DivergenceSubsystem = GetGameInstance()->GetSubsystem<UPRDivergenceSubsystem>();
	QuestSubsystem = GetGameInstance()->GetSubsystem<UPRCompanionQuestSubsystem>();
	if (SaveSubsystem.IsValid()) SaveOperationHandle = SaveSubsystem->OnSaveOperation().AddUObject(this, &UPRMemorySubsystem::HandleSaveOperation);
	if (RunStateSubsystem.IsValid()) AccountDeletedHandle = RunStateSubsystem->OnAccountDeleted().AddUObject(this, &UPRMemorySubsystem::HandleAccountDeleted);
	if (RoomSubsystem.IsValid()) RoomEventHandle = RoomSubsystem->OnRoomEventResolved().AddUObject(this, &UPRMemorySubsystem::HandleRoomEventResolved);
	if (DivergenceSubsystem.IsValid()) DivergenceResultHandle = DivergenceSubsystem->OnDivergenceResult().AddUObject(this, &UPRMemorySubsystem::HandleDivergenceResult);
	if (QuestSubsystem.IsValid()) QuestCompletedHandle = QuestSubsystem->OnQuestCompleted().AddUObject(this, &UPRMemorySubsystem::HandleQuestCompleted);
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UPRMemorySubsystem::HandlePostWorldInitialization);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UPRMemorySubsystem::HandleWorldCleanup);
	if (UWorld* World = GetGameInstance()->GetWorld()) BindWorld(World);
	Snapshot.State = EPRMemoryState::Ready;
	FPRMemoryPersistenceData Persistence;
	Snapshot.bProfileLoaded = SaveSubsystem.IsValid() && SaveSubsystem->GetMemoryPersistenceSnapshot(Persistence);
	Snapshot.bRegistryReady = GetRegistry() && GetRegistry()->IsRegistryReady();
	if (Persistence.Summaries.Num() > 0) { Snapshot.bHasLatestSummary = true; Snapshot.LatestSummary = Persistence.Summaries.Last(); RefreshSnapshotDisplayProjection(); }
	PublishState();
}

void UPRMemorySubsystem::Deinitialize()
{
	CancelProvider();
	if (DeferredPersistenceHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(DeferredPersistenceHandle);
	DeferredPersistenceHandle.Reset();
	if (bPendingFragmentAward && ProgressionSubsystem.IsValid()) ProgressionSubsystem->EndSingleMemoryFragmentAward(PendingSummaryId, false);
	if (SaveSubsystem.IsValid()) SaveSubsystem->OnSaveOperation().Remove(SaveOperationHandle);
	if (RunStateSubsystem.IsValid()) RunStateSubsystem->OnAccountDeleted().Remove(AccountDeletedHandle);
	if (RoomSubsystem.IsValid()) RoomSubsystem->OnRoomEventResolved().Remove(RoomEventHandle);
	if (DivergenceSubsystem.IsValid()) DivergenceSubsystem->OnDivergenceResult().Remove(DivergenceResultHandle);
	if (QuestSubsystem.IsValid()) QuestSubsystem->OnQuestCompleted().Remove(QuestCompletedHandle);
	UnbindWorld(nullptr);
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	Snapshot = FPRMemorySnapshot(); Snapshot.State = EPRMemoryState::ShuttingDown;
	SaveSubsystem.Reset(); ProgressionSubsystem.Reset(); RunStateSubsystem.Reset(); RoomSubsystem.Reset(); DivergenceSubsystem.Reset(); QuestSubsystem.Reset();
	Super::Deinitialize();
}

bool UPRMemorySubsystem::GetSnapshot(FPRMemorySnapshot& OutSnapshot) const { OutSnapshot = Snapshot; return Snapshot.bProfileLoaded; }
bool UPRMemorySubsystem::GetLatestSummary(FPRMemorySummary& OutSummary) const { OutSummary = Snapshot.LatestSummary; return Snapshot.bHasLatestSummary; }
const UPRMemoryRegistryDataAsset* UPRMemorySubsystem::GetRegistry() const { return RegistryAsset.LoadSynchronous(); }
void UPRMemorySubsystem::PublishState() { StateChanged.Broadcast(Snapshot); }
void UPRMemorySubsystem::RefreshSnapshotDisplayProjection()
{
	Snapshot.LatestOptionDisplayTexts.Reset();
	if (!Snapshot.bHasLatestSummary) return;
	const UPRMemoryRegistryDataAsset* Registry = GetRegistry();
	const UPRMemoryPersonaDataAsset* Persona = Registry ? Registry->FindPersona(PRMemorySubsystemPrivate::ProviderCompanionId(Snapshot.LatestSummary.CompanionId)) : nullptr;
	if (!Persona) return;
	for (const FName OptionId : Snapshot.LatestSummary.PlayerOptionIds)
	{
		const FPRMemoryPlayerOptionDefinition* Definition = Persona->PlayerOptions.FindByPredicate([OptionId](const FPRMemoryPlayerOptionDefinition& Entry) { return Entry.OptionId == OptionId; });
		if (!Definition) { Snapshot.LatestOptionDisplayTexts.Reset(); return; }
		Snapshot.LatestOptionDisplayTexts.Add(Definition->DisplayText);
	}
	if (Snapshot.LatestOptionDisplayTexts.Num() != Snapshot.LatestSummary.PlayerOptionIds.Num()) Snapshot.LatestOptionDisplayTexts.Reset();
}
void UPRMemorySubsystem::PublishOperation(const EPRMemoryOperationResult Result, const FName ReasonId)
{
	FPRMemoryOperationEvent Event; Event.RequestId = PendingSummaryId; Event.Result = Result; Event.ReasonId = ReasonId;
	UE_LOG(LogProjectR, Log, TEXT("Memory operation result=%d reason=%s."), static_cast<int32>(Result), *ReasonId.ToString());
	Operation.Broadcast(Event);
}
void UPRMemorySubsystem::ResetRuntimeFacts() { SummaryBuilder.Reset(); }
void UPRMemorySubsystem::CancelProvider()
{
	if (ProviderTimeoutHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(ProviderTimeoutHandle);
	ProviderTimeoutHandle.Reset();
	if (Provider && PendingProviderRequestId.IsValid()) Provider->CancelRequest(PendingProviderRequestId);
	PendingProviderRequestId.Invalidate();
	// Do not destroy a provider from inside a synchronous provider callback.
	if (!bProviderCallInProgress) Provider.Reset();
}
bool UPRMemorySubsystem::HandleProviderTimeout(float)
{
	if (bHasFrozenTransaction && Snapshot.State == EPRMemoryState::ProviderPending) CompleteWithFallback(TEXT("ProviderTimeout"));
	return false;
}

bool UPRMemorySubsystem::HandleDeferredPersistence(float)
{
	if (!bHasFrozenTransaction || Snapshot.State != EPRMemoryState::PersistencePending || !SaveSubsystem.IsValid()) return false;
	if (SaveSubsystem->GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready) return true;
	DeferredPersistenceHandle.Reset();
	BeginPersistence();
	return false;
}

void UPRMemorySubsystem::HandleRoomEventResolved(const FPRRoomEventResult& Event) { if (!bHasFrozenTransaction) SummaryBuilder.RecordRoomEvent(Event); }
void UPRMemorySubsystem::HandleDivergenceResult(const FPRDivergenceResult& Event) { if (!bHasFrozenTransaction) SummaryBuilder.RecordDivergenceResult(Event); }
void UPRMemorySubsystem::HandleQuestCompleted(const FName QuestId)
{
	if (bHasFrozenTransaction || !QuestSubsystem.IsValid()) return;
	FPRCompanionQuestSnapshot QuestSnapshot;
	if (!QuestSubsystem->GetSnapshot(QuestSnapshot)) return;
	TArray<FName> Completed;
	for (const FPRCompanionQuestEntrySnapshot& Entry : QuestSnapshot.Entries) if (Entry.State == EPRCompanionQuestState::Completed) Completed.Add(Entry.QuestId);
	SummaryBuilder.SetCompletedQuestIds(Completed);
}

void UPRMemorySubsystem::BindWorld(UWorld* World)
{
	if (!World || DialogueSubsystem.IsValid()) return;
	DialogueSubsystem = World->GetSubsystem<UPRCompanionDialogueSubsystem>();
	if (DialogueSubsystem.IsValid()) DialogueResultHandle = DialogueSubsystem->OnDialogueResult().AddUObject(this, &UPRMemorySubsystem::HandleDialogueResult);
}
void UPRMemorySubsystem::UnbindWorld(UWorld*) { if (DialogueSubsystem.IsValid()) DialogueSubsystem->OnDialogueResult().Remove(DialogueResultHandle); DialogueResultHandle.Reset(); DialogueSubsystem.Reset(); }
void UPRMemorySubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues) { BindWorld(World); }
void UPRMemorySubsystem::HandleWorldCleanup(UWorld* World, bool, bool) { if (DialogueSubsystem.IsValid() && DialogueSubsystem->GetWorld() == World) UnbindWorld(World); }
void UPRMemorySubsystem::HandleDialogueResult(const FPRDialogueResult& Event) { if (!bHasFrozenTransaction) SummaryBuilder.RecordDialogueResult(Event); }

void UPRMemorySubsystem::HandleAccountDeleted(const FPRAccountDeletedEvent& Event)
{
	if (bHasFrozenTransaction || Event.Record.RecordId == PendingSummaryId) return;
	BeginFromAccountRecord(Event.Record);
}

void UPRMemorySubsystem::BeginFromAccountRecord(const FPRAccountRecord& Record)
{
	FPRMemorySummary Draft;
	if (QuestSubsystem.IsValid())
	{
		FPRCompanionQuestSnapshot QuestSnapshot;
		if (QuestSubsystem->GetSnapshot(QuestSnapshot))
		{
			TArray<FName> Completed;
			for (const FPRCompanionQuestEntrySnapshot& Entry : QuestSnapshot.Entries) if (Entry.State == EPRCompanionQuestState::Completed) Completed.Add(Entry.QuestId);
			SummaryBuilder.SetCompletedQuestIds(Completed);
		}
	}
	if (!SaveSubsystem.IsValid() || !SaveSubsystem->GetMemoryPersistenceSnapshot(PendingExpectedMemory) || !SummaryBuilder.Build(Record, Draft))
	{
		PublishOperation(EPRMemoryOperationResult::InvalidData, TEXT("MemoryUnavailable")); return;
	}
	if (PendingExpectedMemory.LastProcessedGraveyardOrdinal >= Record.GraveyardOrdinal || PendingExpectedMemory.Summaries.ContainsByPredicate([&Record](const FPRMemorySummary& Existing) { return Existing.SummaryId == Record.RecordId; }))
	{
		PublishOperation(EPRMemoryOperationResult::Succeeded, TEXT("AlreadyProcessed")); ResetRuntimeFacts(); return;
	}
	bHasFrozenTransaction = true;
	PendingSummaryId = Draft.SummaryId;
	BeginProvider(Record, MoveTemp(Draft));
}

void UPRMemorySubsystem::BeginProvider(const FPRAccountRecord& Record, FPRMemorySummary&& Draft)
{
	PendingSummary = MoveTemp(Draft);
	FPRPostRunDialogueRequest Request;
	Request.RequestId = FGuid::NewGuid(); Request.SummaryId = PendingSummary.SummaryId; Request.SceneId = TEXT("post_run_summary"); Request.CompanionId = PRMemorySubsystemPrivate::ProviderCompanionId(PendingSummary.CompanionId);
	Request.DurationSeconds = FMath::Clamp(Record.Summary.DurationSeconds, 0, 86400);
	Request.DeathCount = FMath::Clamp(Record.Summary.DeathCount, 0, 99);
	Request.RuleLevel = PendingSummary.DirectorRules.Num() > 0 ? FMath::Clamp(PendingSummary.DirectorRules[0].Level, 1, 5) : 1;
	Request.QTECount = FMath::Clamp(PendingSummary.QTEResults.Num(), 1, 999);
	Request.KeyEventIds = PendingSummary.KeyEventIds;
	PendingProviderRequestId = Request.RequestId;
	Snapshot.State = EPRMemoryState::ProviderPending; PublishState();
	ProviderTimeoutHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UPRMemorySubsystem::HandleProviderTimeout), 1.0f);
	if (const UPRMemoryRegistryDataAsset* Registry = GetRegistry(); Registry && Registry->IsRegistryReady()) Provider = MakeUnique<FPRMockPostRunDialogueProvider>(Registry);
	bProviderCallInProgress = true;
	const bool bStarted = Provider && Provider->BeginRequest(Request, [this](const FGuid& ResponseRequestId, const FPRPostRunDialogueCandidate& Candidate, const TArray<FName>& Fields) { HandleProviderCandidate(ResponseRequestId, Candidate, Fields); });
	bProviderCallInProgress = false;
	if (!bStarted) CompleteWithFallback(TEXT("ProviderUnavailable"));
	if (Snapshot.State != EPRMemoryState::ProviderPending) CancelProvider();
}

void UPRMemorySubsystem::HandleProviderCandidate(const FGuid& ResponseRequestId, const FPRPostRunDialogueCandidate& Candidate, const TArray<FName>& WireFields)
{
	if (!bHasFrozenTransaction || !FPRPostRunDialogueProviderContract::IsMatchingActiveRequest(PendingProviderRequestId, ResponseRequestId) || Snapshot.State != EPRMemoryState::ProviderPending) return;
	FPRPostRunDialogueRequest Request; Request.RequestId = PendingProviderRequestId; Request.SummaryId = PendingSummary.SummaryId; Request.SceneId = TEXT("post_run_summary"); Request.CompanionId = PRMemorySubsystemPrivate::ProviderCompanionId(PendingSummary.CompanionId);
	FPRPostRunDialogueResult Result;
	if (!FPRPostRunDialogueValidator::Validate(Request, Candidate, WireFields, Result)) { CompleteWithFallback(TEXT("ProviderInvalid")); return; }
	ApplyDialogueResult(Result);
}

void UPRMemorySubsystem::CompleteWithFallback(const FName ReasonId)
{
	if (!bHasFrozenTransaction) return;
	const FPRPostRunDialogueCandidate Candidate = PRMemorySubsystemPrivate::BuildFallback(PendingSummary);
	FPRPostRunDialogueRequest Request; Request.RequestId = PendingProviderRequestId.IsValid() ? PendingProviderRequestId : FGuid::NewGuid(); Request.SummaryId = PendingSummary.SummaryId; Request.SceneId = TEXT("post_run_summary"); Request.CompanionId = Candidate.CompanionId;
	FPRPostRunDialogueResult Result;
	if (!FPRPostRunDialogueValidator::Validate(Request, Candidate, { TEXT("scene"), TEXT("companion_id"), TEXT("emotion"), TEXT("summary"), TEXT("player_options") }, Result)) { Snapshot.State = EPRMemoryState::ReadyToRetry; PublishState(); PublishOperation(EPRMemoryOperationResult::InvalidData, ReasonId); return; }
	Result.bUsedFallback = true; Result.FallbackReasonId = ReasonId;
	ApplyDialogueResult(Result);
}

void UPRMemorySubsystem::ApplyDialogueResult(const FPRPostRunDialogueResult& Result)
{
	PendingSummary.SceneId = Result.SceneId;
	PendingSummary.EmotionId = Result.EmotionId;
	PendingSummary.SummaryText = Result.Summary;
	PendingSummary.PlayerOptionIds = Result.PlayerOptionIds;
	PendingSummary.bUsedFallback = Result.bUsedFallback;
	PendingSummary.FallbackReasonId = Result.FallbackReasonId;
	CancelProvider();
	BeginPersistence();
}

bool UPRMemorySubsystem::BuildPendingPersistence()
{
	if (!SaveSubsystem.IsValid()) return false;
	PendingTargetMemory = PendingExpectedMemory;
	PendingSummary.SummarySequence = PendingTargetMemory.SummarySequence + 1;
	const bool bEligible = PendingSummary.TerminationReason != EPRAccountTerminationReason::InterruptedRecovery && !PendingSummary.KeyEventIds.IsEmpty();
	UE_LOG(LogProjectR, Log, TEXT("Memory summary eligibility: termination=%d keyEvents=%d award=%d."),
		static_cast<int32>(PendingSummary.TerminationReason), PendingSummary.KeyEventIds.Num(), bEligible);
	PendingSummary.MemoryFragmentsAwarded = bEligible ? 1 : 0;
	PendingTargetMemory.Summaries.Add(PendingSummary);
	PendingTargetMemory.LastProcessedGraveyardOrdinal = FMath::Max(PendingTargetMemory.LastProcessedGraveyardOrdinal, PendingSummary.GraveyardOrdinal);
	++PendingTargetMemory.LifetimeSummaryCount;
	++PendingTargetMemory.SummarySequence;
	if (bEligible) ++PendingTargetMemory.LifetimeMemoryFragmentsAwarded;
	FPRMemoryPersistenceContract::Normalize(PendingTargetMemory);
	if (!FPRMemoryPersistenceContract::IsCanonical(PendingTargetMemory)) return false;
	bPendingFragmentAward = bEligible;
	if (!bPendingFragmentAward) return SaveSubsystem->StageMemoryPersistenceTransaction(PendingExpectedMemory, PendingTargetMemory);
	if (!ProgressionSubsystem.IsValid() || !ProgressionSubsystem->BeginSingleMemoryFragmentAward(PendingSummary.SummaryId, PendingExpectedProgression, PendingTargetProgression)) return false;
	return SaveSubsystem->StageMemoryProgressionTransaction(PendingExpectedMemory, PendingTargetMemory, PendingExpectedProgression, PendingTargetProgression);
}

void UPRMemorySubsystem::BeginPersistence()
{
	// AccountDeleted is broadcast from the completed account A/B callback before
	// SaveSubsystem transitions from Saving to Ready. Preserve the exact frozen
	// transaction and stage it on the first Ready frame instead of fabricating a
	// retryable persistence failure at this expected queue-ownership boundary.
	if (SaveSubsystem.IsValid() && SaveSubsystem->GetSaveRuntimeState().State == EPRSaveSubsystemState::Saving)
	{
		Snapshot.State = EPRMemoryState::PersistencePending;
		PublishState();
		if (!DeferredPersistenceHandle.IsValid())
		{
			DeferredPersistenceHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateUObject(this, &UPRMemorySubsystem::HandleDeferredPersistence), 0.0f);
		}
		return;
	}
	if (!BuildPendingPersistence())
	{
		if (bPendingFragmentAward && ProgressionSubsystem.IsValid()) ProgressionSubsystem->EndSingleMemoryFragmentAward(PendingSummary.SummaryId, false);
		bPendingFragmentAward = false;
		Snapshot.State = EPRMemoryState::ReadyToRetry; PublishState(); PublishOperation(EPRMemoryOperationResult::PersistenceConflict, TEXT("PersistenceStageRejected")); return;
	}
	if (SaveSubsystem->RequestSaveCurrentProfile(PendingSaveRequestId) != EPRSaveRequestStatus::Started)
	{
		if (bPendingFragmentAward && ProgressionSubsystem.IsValid()) ProgressionSubsystem->EndSingleMemoryFragmentAward(PendingSummary.SummaryId, false);
		Snapshot.State = EPRMemoryState::ReadyToRetry; PublishState(); PublishOperation(EPRMemoryOperationResult::PersistenceFailed, TEXT("PersistenceNotStarted")); return;
	}
	Snapshot.State = EPRMemoryState::PersistencePending; PublishState(); PublishOperation(EPRMemoryOperationResult::Started, NAME_None);
}

void UPRMemorySubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if (Event.Operation != EPRSaveOperationType::Save || Event.RequestId != PendingSaveRequestId)
	{
		if ((Event.Operation == EPRSaveOperationType::Load || Event.Operation == EPRSaveOperationType::Create) && PRMemorySubsystemPrivate::IsSaveSuccess(Event.Result))
		{
			CancelProvider();
			if (bPendingFragmentAward && ProgressionSubsystem.IsValid()) ProgressionSubsystem->EndSingleMemoryFragmentAward(PendingSummaryId, false);
			bPendingFragmentAward = false; bHasFrozenTransaction = false; PendingSaveRequestId.Invalidate(); PendingSummaryId.Invalidate(); PendingSummary = FPRMemorySummary(); ResetRuntimeFacts();
			FPRMemoryPersistenceData Persistence;
			Snapshot = FPRMemorySnapshot(); Snapshot.State = EPRMemoryState::Ready; Snapshot.bProfileLoaded = SaveSubsystem.IsValid() && SaveSubsystem->GetMemoryPersistenceSnapshot(Persistence); Snapshot.bRegistryReady = GetRegistry() && GetRegistry()->IsRegistryReady();
			if (Persistence.Summaries.Num() > 0) { Snapshot.bHasLatestSummary = true; Snapshot.LatestSummary = Persistence.Summaries.Last(); RefreshSnapshotDisplayProjection(); }
			PublishState();
		}
		return;
	}
	PendingSaveRequestId.Invalidate();
	const bool bSuccess = PRMemorySubsystemPrivate::IsSaveSuccess(Event.Result);
	if (bPendingFragmentAward && ProgressionSubsystem.IsValid()) ProgressionSubsystem->EndSingleMemoryFragmentAward(PendingSummary.SummaryId, bSuccess);
	if (!bSuccess) { Snapshot.State = EPRMemoryState::ReadyToRetry; PublishState(); PublishOperation(EPRMemoryOperationResult::PersistenceFailed, TEXT("SaveVerificationFailed")); return; }
	Snapshot.State = EPRMemoryState::Ready;
	Snapshot.bHasLatestSummary = true;
	Snapshot.LatestSummary = PendingSummary;
	Snapshot.bProfileLoaded = true;
	RefreshSnapshotDisplayProjection();
	SummaryReady.Broadcast(PendingSummary);
	PublishState(); PublishOperation(PendingSummary.bUsedFallback ? EPRMemoryOperationResult::UsedFallback : EPRMemoryOperationResult::Succeeded, NAME_None);
	bHasFrozenTransaction = false; bPendingFragmentAward = false; PendingSummaryId.Invalidate(); ResetRuntimeFacts();
}

EPRMemoryOperationResult UPRMemorySubsystem::RetryPendingPersistence()
{
	if (!bHasFrozenTransaction || Snapshot.State != EPRMemoryState::ReadyToRetry) return EPRMemoryOperationResult::NoRetry;
	if (!SaveSubsystem.IsValid() || !SaveSubsystem->GetMemoryPersistenceSnapshot(PendingExpectedMemory)) return EPRMemoryOperationResult::NotReady;
	BeginPersistence();
	return PendingSaveRequestId.IsValid() ? EPRMemoryOperationResult::Started : EPRMemoryOperationResult::PersistenceFailed;
}

EPRMemoryOperationResult UPRMemorySubsystem::SubmitLatestPlayerOption(const EPRMemoryPlayerOptionSlot Slot)
{
	const int32 Index = static_cast<int32>(Slot);
	if (!Snapshot.bHasLatestSummary || Index < 0 || Index >= Snapshot.LatestSummary.PlayerOptionIds.Num()) return EPRMemoryOperationResult::InvalidOption;
	PublishOperation(EPRMemoryOperationResult::Succeeded, Snapshot.LatestSummary.PlayerOptionIds[Index]);
	return EPRMemoryOperationResult::Succeeded;
}
