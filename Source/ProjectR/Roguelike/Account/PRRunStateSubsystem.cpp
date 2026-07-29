// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/Account/PRRunStateSubsystem.h"

#include "Combat/PRCombatSubsystem.h"
#include "Core/PRGameInstance.h"
#include "Director/PRDirectorSubsystem.h"
#include "Director/PRPlayerProfileSubsystem.h"
#include "Divergence/PRDivergenceSubsystem.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "QTE/PRQTESubsystem.h"
#include "Roguelike/Account/PRAccountIdentityRegistryDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Save/PRSaveSubsystem.h"
#include "Companions/PRCompanionSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

void UPRRunStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>();
	RoomSubsystem = GetGameInstance()->GetSubsystem<UPRRoomSubsystem>();
	BindGameInstanceSources();
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UPRRunStateSubsystem::HandlePostWorldInitialization);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UPRRunStateSubsystem::HandleWorldCleanup);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPRRunStateSubsystem::HandlePostLoadMap);
	RefreshRuntimeFromProfile();
	if (RuntimeState.State == EPRRunLifecycleState::RunActive)
	{
		// A loaded in-progress account is intentionally never resumed. Persist its bounded interrupted record instead.
		BeginFinalization(EPRAccountTerminationReason::InterruptedRecovery);
	}
	if (GetWorld()) BindWorld(GetWorld());
}

void UPRRunStateSubsystem::Deinitialize()
{
	UnbindWorld(BoundWorld.Get());
	UnbindGameInstanceSources();
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	RunStateChanged.Clear();
	AccountOperation.Clear();
	AccountDeleted.Clear();
	Super::Deinitialize();
}

EPRAccountOperationResult UPRRunStateSubsystem::RequestCreateAccount(const FPrimaryAssetId IdentityId, FGuid& OutRequestId)
{
	OutRequestId.Invalidate();
	if (RuntimeState.State != EPRRunLifecycleState::Idle) return EPRAccountOperationResult::RejectedBusy;
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	FPRProfileSaveData Profile;
	if (!Save || !Save->GetLoadedProfileSnapshot(Profile)) return EPRAccountOperationResult::RejectedNoProfile;
	if (Profile.AccountPersistence.bHasActiveAccount) return EPRAccountOperationResult::RejectedActiveAccount;
	UPRAccountIdentityRegistryDataAsset* Registry = LoadRegistry();
	if (!Registry || !Registry->IsRegistryReady()) return EPRAccountOperationResult::RejectedRegistryUnavailable;
	if (!Registry->FindIdentity(IdentityId)) return EPRAccountOperationResult::RejectedUnknownIdentity;

	FPendingTransaction Transaction;
	Transaction.Kind = EPendingKind::Create;
	Transaction.RequestId = FGuid::NewGuid();
	Transaction.TargetPersistence = Profile.AccountPersistence;
	Transaction.TargetPersistence.bHasActiveAccount = true;
	Transaction.TargetPersistence.ActiveAccount.AccountId = FGuid::NewGuid();
	Transaction.TargetPersistence.ActiveAccount.IdentityId = IdentityId;
	Transaction.TargetPersistence.ActiveAccount.CreatedUtc = GetUtcNow();
	FPRAccountPersistenceContract::Normalize(Transaction.TargetPersistence);
	Pending = MoveTemp(Transaction);
	OutRequestId = Pending.RequestId;
	SetState(EPRRunLifecycleState::CreatingAccount);
	return StageAndRequest(Pending) ? EPRAccountOperationResult::Started : EPRAccountOperationResult::PersistenceFailed;
}

EPRAccountOperationResult UPRRunStateSubsystem::RequestStartRun(const int32 Seed, FGuid& OutRequestId)
{
	OutRequestId.Invalidate();
	if (RuntimeState.State != EPRRunLifecycleState::AccountReady) return EPRAccountOperationResult::RejectedInvalidState;
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	FPRProfileSaveData Profile;
	if (!Save || !Save->GetLoadedProfileSnapshot(Profile)) return EPRAccountOperationResult::RejectedNoProfile;
	if (!Profile.AccountPersistence.bHasActiveAccount) return EPRAccountOperationResult::RejectedNoAccount;
	if (Profile.AccountPersistence.ActiveAccount.bRunStarted) return EPRAccountOperationResult::RejectedRunAlreadyStarted;

	FPendingTransaction Transaction;
	Transaction.Kind = EPendingKind::Start;
	Transaction.RequestId = FGuid::NewGuid();
	Transaction.TargetPersistence = Profile.AccountPersistence;
	FPRActiveAccountSaveData& Account = Transaction.TargetPersistence.ActiveAccount;
	Account.bRunStarted = true;
	Account.RunId = FGuid::NewGuid();
	Account.Seed = Seed;
	Account.StartedUtc = GetUtcNow();
	FPRAccountPersistenceContract::Normalize(Transaction.TargetPersistence);
	Pending = MoveTemp(Transaction);
	OutRequestId = Pending.RequestId;
	SetState(EPRRunLifecycleState::StartingRun);
	return StageAndRequest(Pending) ? EPRAccountOperationResult::Started : EPRAccountOperationResult::PersistenceFailed;
}

EPRAccountOperationResult UPRRunStateSubsystem::RetryPendingPersistence(FGuid& OutRequestId)
{
	OutRequestId.Invalidate();
	if (RuntimeState.State != EPRRunLifecycleState::PersistenceFailed || Pending.Kind == EPendingKind::None)
	{
		return EPRAccountOperationResult::RejectedNoPendingPersistence;
	}
	OutRequestId = Pending.RequestId;
	return StageAndRequest(Pending) ? EPRAccountOperationResult::Started : EPRAccountOperationResult::PersistenceFailed;
}

EPRAccountOperationResult UPRRunStateSubsystem::RetryReturnToReality()
{
	if (RuntimeState.State != EPRRunLifecycleState::FinalizedTravelPending || !RuntimeState.bTravelPending)
	{
		return EPRAccountOperationResult::RejectedNoPendingTravel;
	}
	if (UPRGameInstance* GameInstance = Cast<UPRGameInstance>(GetGameInstance()); GameInstance && GameInstance->OpenMap(EPRMapId::RealityHub))
	{
		RuntimeState.bTravelPending = false;
		PublishOperation(EPRAccountOperationType::ReturnToReality, EPRAccountOperationResult::Started, FGuid::NewGuid());
		return EPRAccountOperationResult::Started;
	}
	PublishOperation(EPRAccountOperationType::ReturnToReality, EPRAccountOperationResult::TravelFailed, FGuid::NewGuid());
	return EPRAccountOperationResult::TravelFailed;
}

FPRRunRuntimeState UPRRunStateSubsystem::GetRunRuntimeState() const { return RuntimeState; }

bool UPRRunStateSubsystem::GetActiveAccountSnapshot(FPRActiveAccountSaveData& OutAccount) const
{
	OutAccount = FPRActiveAccountSaveData();
	FPRProfileSaveData Profile;
	if (!SaveSubsystem.IsValid() || !SaveSubsystem->GetLoadedProfileSnapshot(Profile) || !Profile.AccountPersistence.bHasActiveAccount) return false;
	OutAccount = Profile.AccountPersistence.ActiveAccount;
	return true;
}

void UPRRunStateSubsystem::GetGraveyardSnapshot(TArray<FPRAccountRecord>& OutGraveyard) const
{
	OutGraveyard.Reset();
	FPRProfileSaveData Profile;
	if (SaveSubsystem.IsValid() && SaveSubsystem->GetLoadedProfileSnapshot(Profile)) OutGraveyard = Profile.AccountPersistence.Graveyard;
}

bool UPRRunStateSubsystem::GetLastRunSummary(FPRRunSummary& OutSummary) const { OutSummary = LastRunSummary; return bHasLastRunSummary; }
FPRRunStateChangedNative& UPRRunStateSubsystem::OnRunStateChanged() { return RunStateChanged; }
FPRAccountOperationNative& UPRRunStateSubsystem::OnAccountOperation() { return AccountOperation; }
FPRAccountDeletedNative& UPRRunStateSubsystem::OnAccountDeleted() { return AccountDeleted; }

void UPRRunStateSubsystem::BindGameInstanceSources()
{
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get()) SaveOperationHandle = Save->OnSaveOperation().AddUObject(this, &UPRRunStateSubsystem::HandleSaveOperation);
	if (UPRRoomSubsystem* Room = RoomSubsystem.Get()) RoomCompletedHandle = Room->OnRoomSequenceCompleted().AddUObject(this, &UPRRunStateSubsystem::HandleRoomSequenceCompleted);
	if (UPRDivergenceSubsystem* Divergence = GetGameInstance()->GetSubsystem<UPRDivergenceSubsystem>()) DivergenceResultHandle = Divergence->OnDivergenceResult().AddUObject(this, &UPRRunStateSubsystem::HandleDivergenceResult);
}

void UPRRunStateSubsystem::UnbindGameInstanceSources()
{
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get()) Save->OnSaveOperation().Remove(SaveOperationHandle);
	if (UPRRoomSubsystem* Room = RoomSubsystem.Get()) Room->OnRoomSequenceCompleted().Remove(RoomCompletedHandle);
	if (UPRDivergenceSubsystem* Divergence = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRDivergenceSubsystem>() : nullptr) Divergence->OnDivergenceResult().Remove(DivergenceResultHandle);
}

void UPRRunStateSubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues) { BindWorld(World); }
void UPRRunStateSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources) { UnbindWorld(World); }

void UPRRunStateSubsystem::BindWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}
	if (BoundWorld.Get() != World)
	{
		UnbindWorld(BoundWorld.Get());
		BoundWorld = World;
	}

	// GameInstance subsystems can observe the PIE world before its WorldSubsystems
	// finish initializing. Re-entering for the same world must fill missing
	// bindings without duplicating delegates that are already active.
	if (!BoundCombat.IsValid())
	{
		if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>())
		{
			BoundCombat = Combat;
			CombatEventHandle = Combat->OnCombatEvent().AddUObject(
				this,
				&UPRRunStateSubsystem::HandleCombatEvent);
		}
	}
	if (!BoundQTE.IsValid())
	{
		if (UPRQTESubsystem* QTE = World->GetSubsystem<UPRQTESubsystem>())
		{
			BoundQTE = QTE;
			QTEResultHandle = QTE->OnQTEResult().AddUObject(
				this,
				&UPRRunStateSubsystem::HandleQTEResult);
		}
	}
	if (!BoundBoss.IsValid())
	{
		if (UPRBossSubsystem* Boss = World->GetSubsystem<UPRBossSubsystem>())
		{
			BoundBoss = Boss;
			BossCompletedHandle = Boss->OnPrototypeRunCompleted().AddUObject(
				this,
				&UPRRunStateSubsystem::HandleBossCompleted);
		}
	}
}

void UPRRunStateSubsystem::UnbindWorld(UWorld* World)
{
	if (!World || BoundWorld.Get() != World) return;
	World->GetTimerManager().ClearTimer(DeferredDeathTimer);
	if (UPRCombatSubsystem* Combat = BoundCombat.Get()) Combat->OnCombatEvent().Remove(CombatEventHandle);
	if (UPRQTESubsystem* QTE = BoundQTE.Get()) QTE->OnQTEResult().Remove(QTEResultHandle);
	if (UPRBossSubsystem* Boss = BoundBoss.Get()) Boss->OnPrototypeRunCompleted().Remove(BossCompletedHandle);
	CombatEventHandle.Reset(); QTEResultHandle.Reset(); BossCompletedHandle.Reset();
	BoundCombat.Reset(); BoundQTE.Reset(); BoundBoss.Reset(); BoundWorld.Reset();
}

void UPRRunStateSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	BindWorld(LoadedWorld);
	if (RuntimeState.State == EPRRunLifecycleState::FinalizedTravelPending && !RuntimeState.bTravelPending
		&& LoadedWorld && LoadedWorld->GetMapName().Contains(TEXT("L_RealityHub")))
	{
		SetState(EPRRunLifecycleState::Idle);
	}
}

void UPRRunStateSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if (Event.Operation == EPRSaveOperationType::Load && Pending.Kind == EPendingKind::None
		&& (Event.Result == EPRSaveResult::Success || Event.Result == EPRSaveResult::RecoveredFromAlternate))
	{
		RefreshRuntimeFromProfile();
		if (RuntimeState.State == EPRRunLifecycleState::RunActive)
		{
			BeginFinalization(EPRAccountTerminationReason::InterruptedRecovery);
		}
		return;
	}
	if (Event.Operation != EPRSaveOperationType::Save || Pending.Kind == EPendingKind::None || Event.RequestId != Pending.SaveRequestId) return;
	if (Event.Result != EPRSaveResult::Success)
	{
		SetState(EPRRunLifecycleState::PersistenceFailed);
		PublishOperation(EPRAccountOperationType::RetryPersistence, EPRAccountOperationResult::PersistenceFailed, Pending.RequestId, Event.RequestId);
		return;
	}
	switch (Pending.Kind)
	{
	case EPendingKind::Create: CompleteCreate(); break;
	case EPendingKind::Start: CompleteStart(); break;
	case EPendingKind::Finalize: CompleteFinalization(); break;
	default: break;
	}
}

void UPRRunStateSubsystem::HandleRoomSequenceCompleted(const FPRRoomSequenceCompleted& Completion)
{
	if (RuntimeState.State != EPRRunLifecycleState::RunActive && RuntimeState.State != EPRRunLifecycleState::AwaitingDivergence) return;
	for (const FPRRoomPathStep& Step : Completion.CompletedPath) if (Step.SelectedRoomId.IsValid()) SummaryBuilder.RecordRoom(Step.SelectedRoomId);
	for (const FPrimaryAssetId& RewardId : Completion.RewardIds) SummaryBuilder.RecordReward(RewardId);
	for (const FGameplayTag& RuleId : Completion.DirectorRuleIds) SummaryBuilder.RecordDirectorRule(RuleId, 1);
	bDeathPending = false;
	if (UWorld* World = BoundWorld.Get()) World->GetTimerManager().ClearTimer(DeferredDeathTimer);
	BeginFinalization(EPRAccountTerminationReason::RoomSequenceCompleted);
}

void UPRRunStateSubsystem::HandleCombatEvent(const FPRCombatEvent& Event)
{
	if (RuntimeState.State != EPRRunLifecycleState::RunActive && RuntimeState.State != EPRRunLifecycleState::AwaitingDivergence) return;
	// CombatEvent is global and includes damage against enemies. Run summary and
	// account death authority may only consume events targeting the current player.
	// Legacy bounded automation facts intentionally omit the transient actor and use
	// the stable combatant ID instead, so retain that value-contract fallback.
	const APawn* PlayerPawn = BoundWorld.IsValid() ? UGameplayStatics::GetPlayerPawn(BoundWorld.Get(), 0) : nullptr;
	if (Event.Target.IsValid()
		? Event.Target.Get() != PlayerPawn
		: Event.TargetId != FName(TEXT("Player")))
	{
		return;
	}
	const float Ratio = Event.MaxHealth > 0.0f ? Event.RemainingHealth / Event.MaxHealth : 1.0f;
	SummaryBuilder.RecordDamage(Event.HealthDamage, Event.bFatal ? Event.HealthDamage : 0.0, Event.ShieldAbsorbed, Ratio);
	if (!Event.bFatal || !Event.EventId.IsValid() || bDeathPending) return;
	bDeathPending = true;
	PendingDeathCause.CombatEventId = Event.EventId;
	PendingDeathCause.SourceId = FGameplayTag();
	PendingDeathCause.AbilityTag = Event.AbilityTag;
	PendingDeathCause.DamageTags = Event.DamageTags;
	PendingDeathCause.ResponseTags = Event.ResponseTags;
	SummaryBuilder.SetDeathCause(PendingDeathCause);
	SetState(EPRRunLifecycleState::AwaitingDivergence);
	if (UWorld* World = BoundWorld.Get()) World->GetTimerManager().SetTimerForNextTick(this, &UPRRunStateSubsystem::ResolveDeferredDeath);
}

void UPRRunStateSubsystem::HandleQTEResult(const FPRQTEResult& Result)
{
	if (RuntimeState.State == EPRRunLifecycleState::RunActive || RuntimeState.State == EPRRunLifecycleState::AwaitingDivergence)
	{
		const FName Grade = Result.TimingGrade == EPRQTETimingGrade::Perfect ? TEXT("Perfect") : Result.TimingGrade == EPRQTETimingGrade::Standard ? TEXT("Standard") : TEXT("None");
		SummaryBuilder.RecordQTEResult(Result.QTEId, Result.CompanionId, Result.ResultTag, Grade);
	}
}

void UPRRunStateSubsystem::HandleBossCompleted(const FPRPrototypeRunResult& Result)
{
	if (RuntimeState.State == EPRRunLifecycleState::RunActive || RuntimeState.State == EPRRunLifecycleState::AwaitingDivergence)
	{
		SummaryBuilder.SetBossResult(FGameplayTag(), true);
	}
}

void UPRRunStateSubsystem::HandleDivergenceResult(const FPRDivergenceResult& Result)
{
	if (!bDeathPending || (RuntimeState.State != EPRRunLifecycleState::AwaitingDivergence && RuntimeState.State != EPRRunLifecycleState::RunActive)) return;
	if (Result.FutureDisposition == EPRDivergenceFutureDisposition::ChallengeContinues)
	{
		bDeathPending = false;
		SetState(EPRRunLifecycleState::RunActive);
		return;
	}
	if (Result.FutureDisposition == EPRDivergenceFutureDisposition::RescueEvacuationRequested)
	{
		bDeathPending = false;
		BeginFinalization(EPRAccountTerminationReason::DivergenceEvacuation);
		return;
	}
	if (Result.FutureDisposition == EPRDivergenceFutureDisposition::LeaveRunRequested)
	{
		bDeathPending = false;
		BeginFinalization(EPRAccountTerminationReason::DivergenceLeave);
		return;
	}
	bDeathPending = false;
	BeginFinalization(EPRAccountTerminationReason::PlayerDeath);
}

void UPRRunStateSubsystem::ResolveDeferredDeath()
{
	if (!bDeathPending || RuntimeState.State != EPRRunLifecycleState::AwaitingDivergence) return;
	if (UPRDivergenceSubsystem* Divergence = GetGameInstance()->GetSubsystem<UPRDivergenceSubsystem>())
	{
		if (Divergence->GetRuntimeState().State == EPRDivergenceState::AwaitingChoice) return;
	}
	bDeathPending = false;
	BeginFinalization(EPRAccountTerminationReason::PlayerDeath);
}

void UPRRunStateSubsystem::BeginFinalization(const EPRAccountTerminationReason Reason)
{
	if (Pending.Kind != EPendingKind::None || RuntimeState.State == EPRRunLifecycleState::Finalizing) return;
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	FPRProfileSaveData Profile;
	if (!Save || !Save->GetLoadedProfileSnapshot(Profile) || !Profile.AccountPersistence.bHasActiveAccount || !Profile.AccountPersistence.ActiveAccount.bRunStarted) return;
	CaptureFinalValues();
	FPRRunSummary Summary = SummaryBuilder.Build(Reason, GetUtcNow());
	FPRAccountRecord Record;
	Record.RecordId = FGuid::NewGuid();
	Record.AccountId = Profile.AccountPersistence.ActiveAccount.AccountId;
	Record.IdentityId = Profile.AccountPersistence.ActiveAccount.IdentityId;
	Record.EndedUtc = Summary.EndedUtc;
	Record.TerminationReason = Reason;
	Record.Summary = Summary;
	Record.GraveyardOrdinal = Profile.AccountPersistence.LifetimeDeletedAccountCount == MAX_int64 ? MAX_int64 : Profile.AccountPersistence.LifetimeDeletedAccountCount + 1;

	FPendingTransaction Transaction;
	Transaction.Kind = EPendingKind::Finalize;
	Transaction.RequestId = FGuid::NewGuid();
	Transaction.DeletedRecord = Record;
	Transaction.bHasDeletedRecord = true;
	Transaction.TargetPersistence = Profile.AccountPersistence;
	Transaction.TargetPersistence.bHasActiveAccount = false;
	Transaction.TargetPersistence.ActiveAccount = FPRActiveAccountSaveData();
	Transaction.TargetPersistence.Graveyard.Add(Record);
	Transaction.TargetPersistence.LifetimeDeletedAccountCount = Record.GraveyardOrdinal;
	Transaction.TargetPersistence.CounterproofFragments = FMath::Min(MAX_int32, Transaction.TargetPersistence.CounterproofFragments + Summary.CounterproofFragmentsAwarded);
	FPRAccountPersistenceContract::Normalize(Transaction.TargetPersistence);
	Pending = MoveTemp(Transaction);
	SetState(EPRRunLifecycleState::Finalizing);
	StageAndRequest(Pending);
}

bool UPRRunStateSubsystem::StageAndRequest(FPendingTransaction& Transaction)
{
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	if (!Save || !Save->StageAccountPersistence(Transaction.TargetPersistence))
	{
		SetState(EPRRunLifecycleState::PersistenceFailed);
		PublishOperation(EPRAccountOperationType::RetryPersistence, EPRAccountOperationResult::PersistenceFailed, Transaction.RequestId);
		return false;
	}
	const EPRSaveRequestStatus Status = Save->RequestSaveCurrentProfile(Transaction.SaveRequestId);
	if (Status != EPRSaveRequestStatus::Started)
	{
		SetState(EPRRunLifecycleState::PersistenceFailed);
		PublishOperation(EPRAccountOperationType::RetryPersistence, EPRAccountOperationResult::PersistenceFailed, Transaction.RequestId, Transaction.SaveRequestId);
		return false;
	}
	return true;
}

void UPRRunStateSubsystem::CompleteCreate()
{
	const FGuid RequestId = Pending.RequestId;
	Pending = FPendingTransaction();
	RefreshRuntimeFromProfile();
	SetState(EPRRunLifecycleState::AccountReady);
	PublishOperation(EPRAccountOperationType::CreateAccount, EPRAccountOperationResult::Succeeded, RequestId);
}

void UPRRunStateSubsystem::CompleteStart()
{
	const FGuid RequestId = Pending.RequestId;
	RefreshRuntimeFromProfile();
	FPRActiveAccountSaveData Account;
	const int64 StartedUtc = GetActiveAccountSnapshot(Account) ? Account.StartedUtc : GetUtcNow();
	SummaryBuilder.Reset(RuntimeState.RunId, RuntimeState.AccountId, RuntimeState.Seed, RuntimeState.IdentityId, StartedUtc);
	Pending = FPendingTransaction();
	FGuid SessionId;
	if (UPRRoomSubsystem* Room = RoomSubsystem.Get(); Room && Room->StartRoomSequence(RuntimeState.Seed, SessionId) == EPRRoomOperationResult::Succeeded)
	{
		SetState(EPRRunLifecycleState::RunActive);
		PublishOperation(EPRAccountOperationType::StartRun, EPRAccountOperationResult::Succeeded, RequestId);
		return;
	}
	SetState(EPRRunLifecycleState::RunActive);
	PublishOperation(EPRAccountOperationType::StartRun, EPRAccountOperationResult::RoomStartFailed, RequestId);
	BeginFinalization(EPRAccountTerminationReason::InterruptedRecovery);
}

void UPRRunStateSubsystem::CompleteFinalization()
{
	const FGuid RequestId = Pending.RequestId;
	const FPRAccountRecord Record = Pending.DeletedRecord;
	Pending = FPendingTransaction();
	LastRunSummary = Record.Summary;
	bHasLastRunSummary = true;
	RuntimeState = FPRRunRuntimeState();
	RuntimeState.State = EPRRunLifecycleState::FinalizedTravelPending;
	RuntimeState.bTravelPending = true;
	RunStateChanged.Broadcast(RuntimeState);
	FPRAccountDeletedEvent Deleted;
	Deleted.RequestId = RequestId;
	Deleted.Record = Record;
	Deleted.WorldTimeSeconds = GetWorldTimeSeconds();
	AccountDeleted.Broadcast(Deleted);
	PublishOperation(EPRAccountOperationType::FinalizeAccount, EPRAccountOperationResult::Succeeded, RequestId);
	RetryReturnToReality();
}

void UPRRunStateSubsystem::PublishOperation(const EPRAccountOperationType Operation, const EPRAccountOperationResult Result, const FGuid& RequestId, const FGuid& SaveRequestId)
{
	FPRAccountOperationEvent Event;
	Event.RequestId = RequestId;
	Event.Operation = Operation;
	Event.Result = Result;
	Event.State = RuntimeState.State;
	Event.AccountId = RuntimeState.AccountId;
	Event.RunId = RuntimeState.RunId;
	Event.SaveRequestId = SaveRequestId;
	Event.WorldTimeSeconds = GetWorldTimeSeconds();
	AccountOperation.Broadcast(Event);
}

void UPRRunStateSubsystem::SetState(const EPRRunLifecycleState NewState)
{
	RuntimeState.State = NewState;
	RuntimeState.bPersistencePending = NewState == EPRRunLifecycleState::CreatingAccount || NewState == EPRRunLifecycleState::StartingRun || NewState == EPRRunLifecycleState::Finalizing;
	RunStateChanged.Broadcast(RuntimeState);
}

void UPRRunStateSubsystem::RefreshRuntimeFromProfile()
{
	FPRActiveAccountSaveData Account;
	RuntimeState = FPRRunRuntimeState();
	if (GetActiveAccountSnapshot(Account))
	{
		RuntimeState.AccountId = Account.AccountId;
		RuntimeState.RunId = Account.RunId;
		RuntimeState.IdentityId = Account.IdentityId;
		RuntimeState.Seed = Account.Seed;
		RuntimeState.State = Account.bRunStarted ? EPRRunLifecycleState::RunActive : EPRRunLifecycleState::AccountReady;
		if (Account.bRunStarted)
		{
			SummaryBuilder.Reset(Account.RunId, Account.AccountId, Account.Seed, Account.IdentityId, Account.StartedUtc);
		}
	}
}

void UPRRunStateSubsystem::CaptureFinalValues()
{
	if (UPRCompanionSubsystem* Companions = GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>())
	{
		TArray<FPRCompanionRelationshipRecord> Relationships;
		Companions->GetAllRelationshipSnapshots(Relationships);
		SummaryBuilder.SetCompanionSnapshot(Companions->GetSyncState().PrimaryCompanionId, Relationships);
	}
	if (UPRDirectorSubsystem* Director = GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>())
	{
		TArray<FPRAppliedDirectorRuleHandle> Rules;
		Director->GetAppliedRules(Rules);
		for (const FPRAppliedDirectorRuleHandle& Rule : Rules) SummaryBuilder.RecordDirectorRule(Rule.RuleId, Rule.Level);
	}
	if (UPRPlayerProfileSubsystem* Profile = GetGameInstance()->GetSubsystem<UPRPlayerProfileSubsystem>())
	{
		FPRPlayerProfileSnapshot Snapshot;
		if (Profile->GetSnapshot(Snapshot))
		{
			SummaryBuilder.RecordDamage(Snapshot.Resources.DamageDealt, Snapshot.Resources.DamageTaken, Snapshot.Resources.ShieldAbsorbed, Snapshot.Resources.MinimumHealthRatio);
			for (const FPRPlayerProfileSkillMetric& Skill : Snapshot.SkillMetrics)
			{
				for (int32 Use = 0; Use < FMath::Min(Skill.UseCount, 64); ++Use) SummaryBuilder.RecordSkill(Skill.SkillTag, Use < Skill.CommitCount);
			}
		}
	}
}

UPRAccountIdentityRegistryDataAsset* UPRRunStateSubsystem::LoadRegistry() const
{
	return LoadObject<UPRAccountIdentityRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Roguelike/Accounts/DA_AccountIdentityRegistry.DA_AccountIdentityRegistry"));
}

int64 UPRRunStateSubsystem::GetUtcNow() const { return FDateTime::UtcNow().ToUnixTimestamp(); }
double UPRRunStateSubsystem::GetWorldTimeSeconds() const { return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0; }

#if WITH_DEV_AUTOMATION_TESTS
bool UPRRunStateSubsystem::InjectAccountPersistenceForAutomation(const FPRAccountPersistenceData& Persistence)
{
	return SaveSubsystem.IsValid() && SaveSubsystem->StageAccountPersistence(Persistence);
}

bool UPRRunStateSubsystem::FinalizeActiveAccountForAutomation(const EPRAccountTerminationReason Reason)
{
	if (RuntimeState.State != EPRRunLifecycleState::RunActive || Pending.Kind != EPendingKind::None)
	{
		return false;
	}
	BeginFinalization(Reason);
	return Pending.Kind == EPendingKind::Finalize;
}
#endif
