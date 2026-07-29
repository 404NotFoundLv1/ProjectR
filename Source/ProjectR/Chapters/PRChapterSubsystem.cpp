// Copyright ProjectR. All Rights Reserved.

#include "Chapters/PRChapterSubsystem.h"

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemySubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Save/PRSaveSubsystem.h"

void UPRChapterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UPRRunStateSubsystem* RunState = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
	UPRRoomSubsystem* Room = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	if (RunState)
	{
		RunStateHandle = RunState->OnRunStateChanged().AddUObject(this, &UPRChapterSubsystem::HandleRunStateChanged);
		AccountDeletedHandle = RunState->OnAccountDeleted().AddUObject(this, &UPRChapterSubsystem::HandleAccountDeleted);
		HandleRunStateChanged(RunState->GetRunRuntimeState());
	}
	if (Room)
	{
		RoomCompletedHandle = Room->OnRoomSequenceCompleted().AddUObject(this, &UPRChapterSubsystem::HandleRoomCompleted);
		RoomEventHandle = Room->OnRoomEventResolved().AddUObject(this, &UPRChapterSubsystem::HandleRoomEventResolved);
	}
	if (Save) SaveOperationHandle = Save->OnSaveOperation().AddUObject(this, &UPRChapterSubsystem::HandleSaveOperation);
	WorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UPRChapterSubsystem::HandlePostWorldInitialization);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UPRChapterSubsystem::HandleWorldCleanup);
	if (UWorld* World = GetWorld()) BindWorld(World);
	ConfigureAllocatorContent();
}

void UPRChapterSubsystem::Deinitialize()
{
	UnbindWorld(BoundWorld.Get());
	if (UPRRunStateSubsystem* RunState = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>() : nullptr)
	{
		RunState->OnRunStateChanged().Remove(RunStateHandle);
		RunState->OnAccountDeleted().Remove(AccountDeletedHandle);
	}
	if (UPRRoomSubsystem* Room = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr)
	{
		Room->OnRoomSequenceCompleted().Remove(RoomCompletedHandle);
		Room->OnRoomEventResolved().Remove(RoomEventHandle);
	}
	if (UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr) Save->OnSaveOperation().Remove(SaveOperationHandle);
	FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitHandle);
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	RunStateHandle.Reset(); AccountDeletedHandle.Reset(); RoomCompletedHandle.Reset(); RoomEventHandle.Reset(); SaveOperationHandle.Reset(); WorldInitHandle.Reset(); WorldCleanupHandle.Reset();
	ResetTransientSession();
	StateChanged.Clear();
	ChapterCompleted.Clear();
	Super::Deinitialize();
}

bool UPRChapterSubsystem::GetSnapshot(FPRChapterSnapshot& OutSnapshot) const
{
	OutSnapshot = Snapshot;
	return true;
}

bool UPRChapterSubsystem::GetLatestCompletion(FPRChapterCompletionResult& OutResult) const
{
	OutResult = LatestCompletion;
	return bHasLatestCompletion;
}

EPRChapterOperationResult UPRChapterSubsystem::RetryPendingSettlement()
{
	if (Snapshot.State != EPRChapterLifecycleState::ReadyToRetry || !PendingSettlement.bPending) return EPRChapterOperationResult::RejectedInvalidState;
	return BeginSettlement() ? EPRChapterOperationResult::Succeeded : EPRChapterOperationResult::PersistenceFailed;
}

FPRChapterStateChangedNative& UPRChapterSubsystem::OnStateChanged() { return StateChanged; }
FPRChapterCompletionNative& UPRChapterSubsystem::OnChapterCompleted() { return ChapterCompleted; }

void UPRChapterSubsystem::HandleRunStateChanged(const FPRRunRuntimeState& State)
{
	if (State.State == EPRRunLifecycleState::AccountReady)
	{
		ConfigureAllocatorContent();
		return;
	}
	if (State.State == EPRRunLifecycleState::RunActive && State.RunId.IsValid() && State.AccountId.IsValid())
	{
		FrozenRunId = State.RunId;
		FrozenAccountId = State.AccountId;
		FrozenSeed = State.Seed;
		Snapshot.State = EPRChapterLifecycleState::RunActive;
		Snapshot.ChapterId = UPRChapterContentRegistryDataAsset::GetAllocatorChapterId();
		Snapshot.ContentId = UPRChapterContentRegistryDataAsset::GetAllocatorContentId();
		Snapshot.DirectiveId = UPRChapterContentRegistryDataAsset::GetDirectiveForSeed(State.Seed);
		Snapshot.AllocationPressure = 0;
		PublishState();
	}
}

void UPRChapterSubsystem::HandleRoomCompleted(const FPRRoomSequenceCompleted& Completion)
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive || Completion.Seed != FrozenSeed || !IsAllocatorSequence(Completion)) return;
	bRoomSequenceVerified = true;
}

void UPRChapterSubsystem::HandleRoomEventResolved(const FPRRoomEventResult& Result)
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive) return;
	const FPrimaryAssetId RegistryId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Allocator"));
	const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(RegistryId);
	const UPRChapterRoguelikeContentRegistryDataAsset* Registry = Path.IsValid() ? Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Path.TryLoad()) : nullptr;
	int32 Delta = 0;
	if (!Registry || !Registry->FindPressureDelta(Result.EventId, Result.ChoiceId, Delta)) return;
	Snapshot.AllocationPressure = FMath::Clamp(Snapshot.AllocationPressure + Delta, 0, 4);
	PublishState();
}

void UPRChapterSubsystem::HandleBossCompleted(const FPRPrototypeRunResult& Completion)
{
	if (Snapshot.State == EPRChapterLifecycleState::RunActive && Completion.CompletionId.IsValid() && Completion.BossId == UPRChapterContentRegistryDataAsset::GetAllocatorBossId())
	{
		bAllocatorBossVerified = true;
	}
}

void UPRChapterSubsystem::HandleAccountDeleted(const FPRAccountDeletedEvent& Event)
{
	const FPRAccountRecord& Record = Event.Record;
	if (Snapshot.State != EPRChapterLifecycleState::RunActive || !bRoomSequenceVerified || !bAllocatorBossVerified
		|| Record.TerminationReason != EPRAccountTerminationReason::RoomSequenceCompleted
		|| Record.AccountId != FrozenAccountId || Record.Summary.RunId != FrozenRunId || Record.Summary.Seed != FrozenSeed) return;
	BeginSettlement();
}

void UPRChapterSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if (!PendingSettlement.bPending || Event.Operation != EPRSaveOperationType::Save || Event.RequestId != PendingSettlement.SaveRequestId) return;
	if (Event.Result != EPRSaveResult::Success)
	{
		Snapshot.State = EPRChapterLifecycleState::ReadyToRetry;
		PublishState();
		return;
	}
	LatestCompletion = PendingSettlement.Completion;
	bHasLatestCompletion = true;
	Snapshot.State = EPRChapterLifecycleState::Completed;
	Snapshot.bHasHumanAnomalyProof = true;
	PendingSettlement = FPendingSettlement();
	ChapterCompleted.Broadcast(LatestCompletion);
	PublishState();
}

void UPRChapterSubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues)
{
	BindWorld(World);
}

void UPRChapterSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	UnbindWorld(World);
}

void UPRChapterSubsystem::BindWorld(UWorld* World)
{
	if (!World || BoundWorld.Get() == World) return;
	UnbindWorld(BoundWorld.Get());
	BoundWorld = World;
	if (UPRBossSubsystem* Boss = World->GetSubsystem<UPRBossSubsystem>()) BossCompletedHandle = Boss->OnPrototypeRunCompleted().AddUObject(this, &UPRChapterSubsystem::HandleBossCompleted);
	// Room encounters carry only closed PrimaryAssetIds. Configure the matching
	// world-owned enemy whitelist before a chapter room can request one; no class
	// or path crosses this runtime boundary.
	if (UPREnemySubsystem* Enemies = World->GetSubsystem<UPREnemySubsystem>())
	{
		const FPrimaryAssetId EnemyRegistryId(TEXT("ProjectREnemyContentRegistry"), TEXT("DA_EnemyContentRegistry_Allocator"));
		Enemies->ConfigureContentRegistry(EnemyRegistryId);
	}
}

void UPRChapterSubsystem::UnbindWorld(UWorld* World)
{
	if (!World || BoundWorld.Get() != World) return;
	if (UPRBossSubsystem* Boss = World->GetSubsystem<UPRBossSubsystem>()) Boss->OnPrototypeRunCompleted().Remove(BossCompletedHandle);
	BossCompletedHandle.Reset();
	BoundWorld.Reset();
}

void UPRChapterSubsystem::PublishState() { StateChanged.Broadcast(Snapshot); }

void UPRChapterSubsystem::ResetTransientSession()
{
	FrozenRunId.Invalidate();
	FrozenAccountId.Invalidate();
	FrozenSeed = 0;
	bRoomSequenceVerified = false;
	bAllocatorBossVerified = false;
	PendingSettlement = FPendingSettlement();
	Snapshot = FPRChapterSnapshot();
}

bool UPRChapterSubsystem::ConfigureAllocatorContent()
{
	UPRRoomSubsystem* Room = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	if (!Room) return false;
	const FPrimaryAssetId RegistryId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Allocator"));
	const EPRRoomContentResult Result = Room->ConfigureContentRegistry(RegistryId);
	return Result == EPRRoomContentResult::Succeeded || Room->GetConfiguredContentRegistryId() == RegistryId;
}

bool UPRChapterSubsystem::IsAllocatorSequence(const FPRRoomSequenceCompleted& Completion) const
{
	if (Completion.CompletedPath.Num() < 6 || Completion.CompletedPath.Num() > 10) return false;
	const FPrimaryAssetId FinalRoom(TEXT("ProjectRRoom"), TEXT("DA_Room_Allocator_Boss_Allocator"));
	return Completion.CompletedPath.Last().SelectedRoomId == FinalRoom;
}

bool UPRChapterSubsystem::BeginSettlement()
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Current;
	if (!Save || !Save->GetChapterPersistenceSnapshot(Current)) return false;
	const FPrimaryAssetId ChapterId = UPRChapterContentRegistryDataAsset::GetAllocatorChapterId();
	const FName ProofId = UPRChapterContentRegistryDataAsset::GetAllocatorProofId();
	if (Current.CompletedChapterIds.Contains(ChapterId) || Current.HumanAnomalyProofIds.Contains(ProofId)) return false;
	FPendingSettlement Transaction;
	Transaction.Expected = Current;
	Transaction.Target = Current;
	Transaction.Target.CompletedChapterIds.Add(ChapterId);
	Transaction.Target.HumanAnomalyProofIds.Add(ProofId);
	Transaction.Target.SettlementSequence = Current.SettlementSequence == MAX_int64 ? MAX_int64 : Current.SettlementSequence + 1;
	FPRChapterPersistenceContract::Normalize(Transaction.Target);
	Transaction.Completion.CompletionId = FGuid::NewGuid();
	Transaction.Completion.ChapterId = ChapterId;
	Transaction.Completion.ProofId = ProofId;
	Transaction.Completion.SettlementSequence = Transaction.Target.SettlementSequence;
	Transaction.Completion.bProofAwarded = true;
	Transaction.bPending = true;
	PendingSettlement = MoveTemp(Transaction);
	if (!Save->StageChapterPersistenceTransaction(PendingSettlement.Expected, PendingSettlement.Target)) { PendingSettlement = FPendingSettlement(); return false; }
	if (Save->RequestSaveCurrentProfile(PendingSettlement.SaveRequestId) != EPRSaveRequestStatus::Started) { PendingSettlement = FPendingSettlement(); return false; }
	return true;
}
