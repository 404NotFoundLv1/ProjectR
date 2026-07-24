// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dialogue/PRCompanionDialogueSubsystem.h"

#include "Characters/PRPlayerCharacter.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRTagLibrary.h"
#include "Dialogue/PRCompanionDialogueComponent.h"
#include "Dialogue/PRCompanionDialogueDataAsset.h"
#include "Dialogue/PRCompanionDialogueRegistryDataAsset.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/Bosses/PRBossTypes.h"
#include "Enemies/PREnemySubsystem.h"
#include "Enemies/PREnemyTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "QTE/PRQTESubsystem.h"
#include "Save/PRSaveSubsystem.h"
#include "UI/PRCompanionDialogueWidget.h"
#include "ProjectR.h"

namespace PRDialogue
{
constexpr int32 MaximumQueuedRequests = 4;
constexpr int32 MaximumRecentEvents = 64;
constexpr int32 MaximumStartsPerTenSeconds = 3;
constexpr double MinimumStartIntervalSeconds = 1.25;
constexpr double PreemptionMinimumAgeSeconds = 0.35;
constexpr int32 PreemptionPriorityDelta = 20;
constexpr float BarkExpirySeconds = 3.0f;
constexpr float ChoiceExpirySeconds = 15.0f;
constexpr float SafeStateDelaySeconds = 2.0f;
}

void UPRCompanionDialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadFixedRegistry();
	CombatSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UPRCombatSubsystem>() : nullptr;
	QTESubsystem = GetWorld() ? GetWorld()->GetSubsystem<UPRQTESubsystem>() : nullptr;
	CompanionRuntimeSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UPRCompanionRuntimeSubsystem>() : nullptr;
	BossSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UPRBossSubsystem>() : nullptr;
	EnemySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UPREnemySubsystem>() : nullptr;
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		CompanionSubsystem = GameInstance->GetSubsystem<UPRCompanionSubsystem>();
		SaveSubsystem = GameInstance->GetSubsystem<UPRSaveSubsystem>();
	}
	if (UPRCombatSubsystem* Subsystem = CombatSubsystem.Get()) CombatEventHandle = Subsystem->OnCombatEvent().AddUObject(this, &UPRCompanionDialogueSubsystem::HandleCombatEvent);
	if (UPRQTESubsystem* Subsystem = QTESubsystem.Get()) QTEResultHandle = Subsystem->OnQTEResult().AddUObject(this, &UPRCompanionDialogueSubsystem::HandleQTEResult);
	if (UPRCompanionRuntimeSubsystem* Subsystem = CompanionRuntimeSubsystem.Get()) SupportEventHandle = Subsystem->OnSupportEvent().AddUObject(this, &UPRCompanionDialogueSubsystem::HandleSupportEvent);
	if (UPRBossSubsystem* Subsystem = BossSubsystem.Get())
	{
		BossStateHandle = Subsystem->OnBossStateChanged().AddUObject(this, &UPRCompanionDialogueSubsystem::HandleBossState);
		BossCompletionHandle = Subsystem->OnPrototypeRunCompleted().AddUObject(this, &UPRCompanionDialogueSubsystem::HandleBossCompletion);
	}
	if (UPREnemySubsystem* Subsystem = EnemySubsystem.Get()) EnemyStateHandle = Subsystem->OnEnemyStateChanged().AddUObject(this, &UPRCompanionDialogueSubsystem::HandleEnemyState);
	if (UPRCompanionSubsystem* Subsystem = CompanionSubsystem.Get()) PrimarySyncHandle = Subsystem->OnPrimarySyncChanged().AddUObject(this, &UPRCompanionDialogueSubsystem::HandlePrimarySyncChanged);
	if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(InputReconcileTimer, this, &UPRCompanionDialogueSubsystem::ReconcileInputBridge, 0.25f, true);
	ReconcileInputBridge();
}

void UPRCompanionDialogueSubsystem::Deinitialize()
{
	if (UPRCombatSubsystem* Subsystem = CombatSubsystem.Get(); Subsystem && CombatEventHandle.IsValid()) Subsystem->OnCombatEvent().Remove(CombatEventHandle);
	if (UPRQTESubsystem* Subsystem = QTESubsystem.Get(); Subsystem && QTEResultHandle.IsValid()) Subsystem->OnQTEResult().Remove(QTEResultHandle);
	if (UPRCompanionRuntimeSubsystem* Subsystem = CompanionRuntimeSubsystem.Get(); Subsystem && SupportEventHandle.IsValid()) Subsystem->OnSupportEvent().Remove(SupportEventHandle);
	if (UPRBossSubsystem* Subsystem = BossSubsystem.Get())
	{
		if (BossStateHandle.IsValid()) Subsystem->OnBossStateChanged().Remove(BossStateHandle);
		if (BossCompletionHandle.IsValid()) Subsystem->OnPrototypeRunCompleted().Remove(BossCompletionHandle);
	}
	if (UPREnemySubsystem* Subsystem = EnemySubsystem.Get(); Subsystem && EnemyStateHandle.IsValid()) Subsystem->OnEnemyStateChanged().Remove(EnemyStateHandle);
	if (UPRCompanionSubsystem* Subsystem = CompanionSubsystem.Get(); Subsystem && PrimarySyncHandle.IsValid()) Subsystem->OnPrimarySyncChanged().Remove(PrimarySyncHandle);
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearAllTimersForObject(this);
	ClearRuntime();
	InputBridge.Reset();
	SaveSubsystem.Reset();
	LoadedAssets.Reset();
	Super::Deinitialize();
}

bool UPRCompanionDialogueSubsystem::IsRegistryReady() const { return bRegistryReady; }
FPRDialogueRuntimeState UPRCompanionDialogueSubsystem::GetRuntimeState() const { return RuntimeState; }
FPRDialogueStateChangedNative& UPRCompanionDialogueSubsystem::OnDialogueStateChanged() { return StateChanged; }
FPRDialogueResultNative& UPRCompanionDialogueSubsystem::OnDialogueResult() { return ResultPublished; }

void UPRCompanionDialogueSubsystem::LoadFixedRegistry()
{
	RegistryAsset = TSoftObjectPtr<UPRCompanionDialogueRegistryDataAsset>(FSoftObjectPath(TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogueRegistry.DA_CompanionDialogueRegistry")));
	UPRCompanionDialogueRegistryDataAsset* Registry = RegistryAsset.LoadSynchronous();
	FString Error;
	if (!Registry || !Registry->ValidateRegistry(Error))
	{
		UE_LOG(LogProjectR, Warning, TEXT("Dialogue registry unavailable: %s"), *Error);
		return;
	}
	for (const TSoftObjectPtr<UPRCompanionDialogueDataAsset>& AssetRef : { Registry->Axiom, Registry->Kindle, Registry->Null })
	{
		UPRCompanionDialogueDataAsset* Asset = AssetRef.LoadSynchronous();
		if (!Asset || !Asset->ValidateDefinition(Error) || LoadedAssets.Contains(Asset->CompanionId))
		{
			UE_LOG(LogProjectR, Warning, TEXT("Dialogue asset unavailable: %s"), *Error);
			LoadedAssets.Reset();
			return;
		}
		LoadedAssets.Add(Asset->CompanionId, Asset);
	}
	bRegistryReady = LoadedAssets.Num() == 3;
}

void UPRCompanionDialogueSubsystem::ReconcileInputBridge()
{
	APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APRPlayerCharacter* Pawn = Controller ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
	if (!Controller || !Pawn)
	{
		return;
	}
	UPRCompanionDialogueComponent* Component = Controller->FindComponentByClass<UPRCompanionDialogueComponent>();
	if (!Component)
	{
		Component = NewObject<UPRCompanionDialogueComponent>(Controller, TEXT("CompanionDialogueComponent"));
		Component->RegisterComponent();
	}
	InputBridge = Component;
	Component->InitializeForSubsystem(this);
	if (UPRCompanionDialogueRegistryDataAsset* Registry = RegistryAsset.Get()) Component->SetDialogueWidgetClass(Registry->WidgetClass.LoadSynchronous());
	if (bHasBoundDialoguePawn && BoundDialoguePawn.Get() != Pawn)
	{
		CancelDialogue();
	}
	BoundDialoguePawn = Pawn;
	bHasBoundDialoguePawn = true;
	Component->RebindPlayerPawn(Pawn);
}

void UPRCompanionDialogueSubsystem::HandleCombatEvent(const FPRCombatEvent& Event)
{
	if (Event.EventTag.MatchesTagExact(UPRTagLibrary::GetCombatEventDamageTag()) && Event.HealthDamage > 0.0f)
	{
		if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(SafeStateTimer);
		if (RuntimeState.PresentationState == EPRDialoguePresentationState::Choice) CancelDialogue();
		if (Event.TargetId == TEXT("Player") && Event.MaxHealth > 0.0f && !Event.bFatal)
		{
			const float HealthRatio = Event.RemainingHealth / Event.MaxHealth;
			if (HealthRatio > 0.0f && HealthRatio <= 0.15f) Enqueue(EPRDialogueTriggerKind::CriticalLowHealth, Event.EventId, Event.WorldTimeSeconds);
			else if (HealthRatio <= 0.35f) Enqueue(EPRDialogueTriggerKind::LowHealth, Event.EventId, Event.WorldTimeSeconds);
		}
	}
	if (Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePredictionBlockedTag())) Enqueue(EPRDialogueTriggerKind::PredictionBlocked, Event.EventId, Event.WorldTimeSeconds);
	if (Event.bFatal)
	{
		KnownEnemyAlive.FindOrAdd(Event.TargetId) = false;
		TryScheduleSafeState();
	}
}

void UPRCompanionDialogueSubsystem::HandleQTEResult(const FPRQTEResult& Result)
{
	if (Result.ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultSuccessTag())) Enqueue(EPRDialogueTriggerKind::QTESuccess, Result.ResultId, Result.WorldTimeSeconds);
	else if (Result.ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultFailureTag()) || Result.ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultRejectedTag()) || Result.ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultTimeoutTag())) Enqueue(EPRDialogueTriggerKind::QTEFailure, Result.ResultId, Result.WorldTimeSeconds);
}

void UPRCompanionDialogueSubsystem::HandleSupportEvent(const FPRCompanionSupportEvent& Event)
{
	Enqueue(Event.Result == EPRCompanionSupportResult::Applied ? EPRDialogueTriggerKind::SupportApplied : EPRDialogueTriggerKind::SupportRejected, Event.EventId, Event.WorldTimeSeconds);
}

void UPRCompanionDialogueSubsystem::HandleBossState(const FPRBossRuntimeState& State)
{
	if ((State.Phase == EPRAuditorBossPhase::RuleAudit || State.Phase == EPRAuditorBossPhase::PredictionShield) && State.PhaseSequence > LastBossPhaseSequence)
	{
		LastBossPhaseSequence = State.PhaseSequence;
		Enqueue(EPRDialogueTriggerKind::BossPhaseChanged, FGuid::NewGuid(), State.WorldTimeSeconds);
	}
}

void UPRCompanionDialogueSubsystem::HandleBossCompletion(const FPRPrototypeRunResult& Result)
{
	bBossCompleted = true;
	TryScheduleSafeState();
}

void UPRCompanionDialogueSubsystem::HandleEnemyState(const FPREnemyRuntimeState& State)
{
	KnownEnemyAlive.FindOrAdd(State.CombatantId) = State.bAlive;
	if (State.bAlive)
	{
		bBossCompleted = false;
		if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(SafeStateTimer);
	}
}

void UPRCompanionDialogueSubsystem::HandlePrimarySyncChanged(const FPRPrimaryCompanionSyncChangedEvent& Event)
{
	CancelDialogue();
}

bool UPRCompanionDialogueSubsystem::Enqueue(const EPRDialogueTriggerKind TriggerKind, const FGuid& EventId, const double EventTimeSeconds)
{
	if (!bRegistryReady || !EventId.IsValid()) return false;
	for (auto Iterator = RecentEventTimes.CreateIterator(); Iterator; ++Iterator)
	{
		if (!FPRDialogueContract::IsWithinEventDedupWindow(Iterator.Value(), EventTimeSeconds)) Iterator.RemoveCurrent();
	}
	if (RecentEventTimes.Contains(EventId)) return false;
	UPRCompanionDialogueDataAsset* Asset = GetPrimaryDialogueAsset();
	const FPRDialogueLineDefinition* Line = Asset ? Asset->FindLine(TriggerKind) : nullptr;
	const FName CooldownKey = Asset && Line ? FPRDialogueContract::GetLineCooldownKey(Asset->CompanionId, Line->LineId) : NAME_None;
	if (!Line || (LineCooldownEnds.FindRef(CooldownKey) > EventTimeSeconds)) return false;
	RecentEventTimes.Add(EventId, EventTimeSeconds);
	while (RecentEventTimes.Num() > PRDialogue::MaximumRecentEvents)
	{
		double OldestTime = TNumericLimits<double>::Max();
		FGuid OldestId;
		for (const TPair<FGuid, double>& Pair : RecentEventTimes)
		{
			if (Pair.Value < OldestTime)
			{
				OldestTime = Pair.Value;
				OldestId = Pair.Key;
			}
		}
		RecentEventTimes.Remove(OldestId);
	}
	FQueuedDialogue Candidate;
	Candidate.Asset = Asset;
	Candidate.Request.RequestId = FGuid::NewGuid();
	Candidate.Request.LineId = Line->LineId;
	Candidate.Request.CompanionId = Asset->CompanionId;
	Candidate.Request.TriggerKind = TriggerKind;
	Candidate.Request.Priority = Line->Priority;
	Candidate.Request.SourceEventId = EventId;
	Candidate.Request.WorldTimeSeconds = EventTimeSeconds;
	Candidate.EnqueueTimeSeconds = EventTimeSeconds;
	if (RuntimeState.PresentationState != EPRDialoguePresentationState::Idle && Candidate.Request.Priority >= ActiveDialogue.Request.Priority + PRDialogue::PreemptionPriorityDelta && EventTimeSeconds - RuntimeState.StartTimeSeconds >= PRDialogue::PreemptionMinimumAgeSeconds)
	{
		ClearActive();
	}
	Queue.Add(Candidate);
	Queue.Sort([](const FQueuedDialogue& A, const FQueuedDialogue& B) { return FPRDialogueContract::IsRequestBefore(A.Request, B.Request); });
	if (Queue.Num() > PRDialogue::MaximumQueuedRequests) Queue.SetNum(PRDialogue::MaximumQueuedRequests);
	return StartNext();
}

bool UPRCompanionDialogueSubsystem::StartNext()
{
	if (RuntimeState.PresentationState != EPRDialoguePresentationState::Idle || Queue.IsEmpty()) return false;
	const double Now = GetWorldTimeSeconds();
	Queue.RemoveAll([Now](const FQueuedDialogue& Queued) { return FPRDialogueContract::IsQueuedRequestExpired(Queued.EnqueueTimeSeconds, Now); });
	if (Queue.IsEmpty()) return false;
	RecentStartTimes.RemoveAll([Now](const double Time) { return Now - Time > 10.0; });
	const double DeferredStartDelay = FPRDialogueContract::GetDeferredStartDelay(RecentStartTimes, Now);
	if (DeferredStartDelay > 0.0)
	{
		ScheduleStartNext(DeferredStartDelay);
		return false;
	}
	ActiveDialogue = Queue[0];
	Queue.RemoveAt(0);
	UPRCompanionDialogueDataAsset* Asset = ActiveDialogue.Asset.Get();
	const FPRDialogueLineDefinition* Line = Asset ? Asset->FindLine(ActiveDialogue.Request.TriggerKind) : nullptr;
	if (!Asset || !Line) { ClearActive(); return false; }
	RuntimeState.RequestId = ActiveDialogue.Request.RequestId;
	RuntimeState.CompanionId = Asset->CompanionId;
	RuntimeState.LineId = Line->LineId;
	RuntimeState.TriggerKind = Line->TriggerKind;
	RuntimeState.SpeakerText = Asset->SpeakerName;
	RuntimeState.DialogueText = Line->Text;
	RuntimeState.StartTimeSeconds = Now;
	RuntimeState.PresentationState = FPRDialogueContract::IsSafeStateTrigger(Line->TriggerKind) && HasLoadedProfile() ? EPRDialoguePresentationState::Choice : EPRDialoguePresentationState::Bark;
	RuntimeState.Choices = RuntimeState.PresentationState == EPRDialoguePresentationState::Choice ? Asset->SafeChoices : TArray<FPRDialogueChoiceDefinition>();
	RuntimeState.ExpireTimeSeconds = Now + (RuntimeState.PresentationState == EPRDialoguePresentationState::Choice ? PRDialogue::ChoiceExpirySeconds : FMath::Min(Line->DisplayDurationSeconds, PRDialogue::BarkExpirySeconds));
	LineCooldownEnds.Add(FPRDialogueContract::GetLineCooldownKey(Asset->CompanionId, Line->LineId), Now + Line->CooldownSeconds);
	RecentStartTimes.Add(Now);
	BroadcastState(RuntimeState.PresentationState);
	if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(ActiveExpiryTimer, this, &UPRCompanionDialogueSubsystem::ExpireActive, RuntimeState.ExpireTimeSeconds - Now, false);
	return true;
}

void UPRCompanionDialogueSubsystem::ScheduleStartNext(const double DelaySeconds)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DeferredStartTimer, this, &UPRCompanionDialogueSubsystem::HandleDeferredStart, FMath::Max(0.01, DelaySeconds), false);
	}
}

void UPRCompanionDialogueSubsystem::HandleDeferredStart()
{
	StartNext();
}

bool UPRCompanionDialogueSubsystem::SubmitChoice(const bool bAnalyze)
{
	if (RuntimeState.PresentationState != EPRDialoguePresentationState::Choice || RuntimeState.Choices.Num() != 2) return false;
	FPRDialogueResult Result;
	Result.ResultId = FGuid::NewGuid();
	Result.RequestId = RuntimeState.RequestId;
	Result.CompanionId = RuntimeState.CompanionId;
	Result.ChoiceId = RuntimeState.Choices[bAnalyze ? 1 : 0].ChoiceId;
	Result.RelationshipDeltaRequest = RuntimeState.Choices[bAnalyze ? 1 : 0].RelationshipDelta;
	Result.WorldTimeSeconds = GetWorldTimeSeconds();
	UPRCompanionSubsystem* Relationships = CompanionSubsystem.Get();
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	FPRCompanionRelationshipRecord Before;
	FPRCompanionRelationshipRecord After;
	if (!Relationships || !Save || !Relationships->GetRelationshipSnapshot(Result.CompanionId, Before))
	{
		Result.Resolution = EPRDialogueChoiceResolution::RejectedNoProfile;
	}
	else
	{
		Relationships->ApplyRelationshipDelta(Result.RelationshipDeltaRequest);
		if (!Relationships->GetRelationshipSnapshot(Result.CompanionId, After) || !IsRelationshipStateDifferent(Before, After))
		{
			Result.Resolution = EPRDialogueChoiceResolution::NoRelationshipChange;
		}
		else
		{
			Result.Resolution = EPRDialogueChoiceResolution::Applied;
			Save->RequestSaveCurrentProfile(Result.SaveRequestId);
		}
	}
	ResultPublished.Broadcast(Result);
	ClearActive();
	StartNext();
	return true;
}

void UPRCompanionDialogueSubsystem::CancelDialogue()
{
	if (RuntimeState.PresentationState != EPRDialoguePresentationState::Idle)
	{
		FPRDialogueResult Result;
		Result.ResultId = FGuid::NewGuid();
		Result.RequestId = RuntimeState.RequestId;
		Result.CompanionId = RuntimeState.CompanionId;
		Result.Resolution = EPRDialogueChoiceResolution::Cancelled;
		Result.WorldTimeSeconds = GetWorldTimeSeconds();
		ResultPublished.Broadcast(Result);
	}
	ClearRuntime();
}

void UPRCompanionDialogueSubsystem::ExpireActive()
{
	if (RuntimeState.PresentationState == EPRDialoguePresentationState::Choice)
	{
		FPRDialogueResult Result;
		Result.ResultId = FGuid::NewGuid();
		Result.RequestId = RuntimeState.RequestId;
		Result.CompanionId = RuntimeState.CompanionId;
		Result.Resolution = EPRDialogueChoiceResolution::RejectedExpired;
		Result.WorldTimeSeconds = GetWorldTimeSeconds();
		ResultPublished.Broadcast(Result);
	}
	ClearActive();
	StartNext();
}

void UPRCompanionDialogueSubsystem::TryScheduleSafeState()
{
	bool bAnyAlive = false;
	for (const TPair<FName, bool>& Pair : KnownEnemyAlive)
	{
		if (Pair.Value)
		{
			bAnyAlive = true;
			break;
		}
	}
	if ((KnownEnemyAlive.Num() > 0 && !bAnyAlive) || bBossCompleted)
	{
		if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(SafeStateTimer, this, &UPRCompanionDialogueSubsystem::HandleSafeStateTimer, PRDialogue::SafeStateDelaySeconds, false);
	}
}

void UPRCompanionDialogueSubsystem::HandleSafeStateTimer()
{
	Enqueue(EPRDialogueTriggerKind::SafeCombatCleared, FGuid::NewGuid(), GetWorldTimeSeconds());
}

void UPRCompanionDialogueSubsystem::BroadcastState(const EPRDialoguePresentationState NewState)
{
	RuntimeState.PresentationState = NewState;
	StateChanged.Broadcast(RuntimeState);
}

void UPRCompanionDialogueSubsystem::ClearActive()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(ActiveExpiryTimer);
	ActiveDialogue = FQueuedDialogue();
	RuntimeState = FPRDialogueRuntimeState();
	StateChanged.Broadcast(RuntimeState);
}

void UPRCompanionDialogueSubsystem::ClearRuntime()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveExpiryTimer);
		World->GetTimerManager().ClearTimer(SafeStateTimer);
		World->GetTimerManager().ClearTimer(DeferredStartTimer);
	}
	Queue.Reset();
	RuntimeState = FPRDialogueRuntimeState();
	StateChanged.Broadcast(RuntimeState);
}

UPRCompanionDialogueDataAsset* UPRCompanionDialogueSubsystem::GetPrimaryDialogueAsset() const
{
	if (const UPRCompanionSubsystem* Subsystem = CompanionSubsystem.Get()) return LoadedAssets.FindRef(Subsystem->GetSyncState().PrimaryCompanionId);
	return nullptr;
}

bool UPRCompanionDialogueSubsystem::HasLoadedProfile() const
{
	FPRProfileSaveData Profile;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UPRSaveSubsystem* Save = GameInstance ? GameInstance->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	return Save && Save->GetLoadedProfileSnapshot(Profile);
}

bool UPRCompanionDialogueSubsystem::IsRelationshipStateDifferent(const FPRCompanionRelationshipRecord& Left, const FPRCompanionRelationshipRecord& Right) const
{
	return Left.State.Trust != Right.State.Trust || Left.State.Affection != Right.State.Affection || Left.State.Evaluation != Right.State.Evaluation || Left.State.Overload != Right.State.Overload;
}

double UPRCompanionDialogueSubsystem::GetWorldTimeSeconds() const { return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0; }
