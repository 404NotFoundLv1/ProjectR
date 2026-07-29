// Copyright ProjectR. All Rights Reserved.

#include "Chapters/PRChapterSubsystem.h"

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/Warden/PRWardenChapterDataAsset.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Enemies/PREnemyContentRegistryDataAsset.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemySubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Memory/PRMemorySubsystem.h"
#include "Quests/PRCompanionQuestSubsystem.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomSubsystem.h"
#include "Save/PRSaveSubsystem.h"
#include "UI/PRWardenChapterWidget.h"
#include "Kismet/GameplayStatics.h"

namespace PRChapterSubsystemPrivate
{
	const FPrimaryAssetId AllocatorRoomRegistryId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Allocator"));
	const FPrimaryAssetId AllocatorEnemyRegistryId(TEXT("ProjectREnemyContentRegistry"), TEXT("DA_EnemyContentRegistry_Allocator"));
	const FPrimaryAssetId AllocatorFinalRoomId(TEXT("ProjectRRoom"), TEXT("DA_Room_Allocator_Boss_Allocator"));
	const FSoftObjectPath WardenEnemyRegistryPath(TEXT("/Game/ProjectR/Chapters/Warden/DA_EnemyContentRegistry_Warden.DA_EnemyContentRegistry_Warden"));

	const UPRChapterRoguelikeContentRegistryDataAsset* LoadRoomRegistry(const FPrimaryAssetId& RegistryId)
	{
		const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(RegistryId);
		if (const UPRChapterRoguelikeContentRegistryDataAsset* Registry = Path.IsValid() ? Cast<UPRChapterRoguelikeContentRegistryDataAsset>(Path.TryLoad()) : nullptr) return Registry;
		// AssetManager settings are frozen for this release. This exact typed fallback
		// completes the closed Warden PrimaryAssetId seam without accepting a path input.
		if (RegistryId == UPRChapterContentRegistryDataAsset::GetWardenRoomRegistryId())
		{
			return LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Warden/DA_RoguelikeContentRegistry_Warden.DA_RoguelikeContentRegistry_Warden"));
		}
		return nullptr;
	}

	const UPRWardenChapterDataAsset* LoadWardenDefinition()
	{
		const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(UPRChapterContentRegistryDataAsset::GetWardenChapterId());
		if (const UPRWardenChapterDataAsset* Definition = Path.IsValid() ? Cast<UPRWardenChapterDataAsset>(Path.TryLoad()) : nullptr) return Definition;
		return LoadObject<UPRWardenChapterDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Warden/DA_Chapter_Warden.DA_Chapter_Warden"));
	}

	bool EnsureWardenEnemyRegistryIsRegistered()
	{
		UAssetManager& AssetManager = UAssetManager::Get();
		const FPrimaryAssetId RegistryId = UPRChapterContentRegistryDataAsset::GetWardenEnemyRegistryId();
		if (AssetManager.GetPrimaryAssetPath(RegistryId).IsValid()) return true;
		// Project settings are frozen for this release. Register only the exact
		// cooked Warden registry as a dynamic PrimaryAsset, so editor and packaged
		// runtime share the same closed path/class/identity-free boundary.
		FAssetBundleData BundleData;
		BundleData.AddBundleAsset(TEXT("WardenRegistry"), WardenEnemyRegistryPath.GetAssetPath());
		if (!AssetManager.AddDynamicAsset(RegistryId, WardenEnemyRegistryPath, BundleData)) return false;
		return Cast<UPREnemyContentRegistryDataAsset>(WardenEnemyRegistryPath.TryLoad()) != nullptr
			&& AssetManager.GetPrimaryAssetPath(RegistryId) == WardenEnemyRegistryPath;
	}
}

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
	ConfigureActiveContent();
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
	return SubmitPendingSettlement() ? EPRChapterOperationResult::Succeeded : EPRChapterOperationResult::PersistenceFailed;
}

FPRChapterStateChangedNative& UPRChapterSubsystem::OnStateChanged() { return StateChanged; }
FPRChapterCompletionNative& UPRChapterSubsystem::OnChapterCompleted() { return ChapterCompleted; }

#if WITH_DEV_AUTOMATION_TESTS
bool UPRChapterSubsystem::StageFixedAllocatorProofForAutomation()
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Current;
	if (!Save || !UPRSaveSubsystem::HasAutomationStorageOverride() || !Save->GetChapterPersistenceSnapshot(Current)) return false;
	if (Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAllocatorProofId())) return true;
	if (Current.CompletedChapterIds.Num() >= FPRChapterPersistenceContract::MaxEntries || Current.HumanAnomalyProofIds.Num() >= FPRChapterPersistenceContract::MaxEntries) return false;
	FPRChapterPersistenceData Target = Current;
	Target.CompletedChapterIds.Add(UPRChapterContentRegistryDataAsset::GetAllocatorChapterId());
	Target.HumanAnomalyProofIds.Add(UPRChapterContentRegistryDataAsset::GetAllocatorProofId());
	Target.SettlementSequence = Current.SettlementSequence + 1;
	FPRChapterPersistenceContract::Normalize(Target);
	FGuid RequestId;
	return Save->StageChapterPersistenceTransaction(Current, Target)
		&& Save->RequestSaveCurrentProfile(RequestId) == EPRSaveRequestStatus::Started;
}
#endif

void UPRChapterSubsystem::HandleRunStateChanged(const FPRRunRuntimeState& State)
{
	if (State.State == EPRRunLifecycleState::AccountReady)
	{
		// Account/profile transitions must not retain a Warden presentation or frozen
		// chapter result from the previous account. The new account is then selected
		// solely from its persisted Schema-7 proof snapshot.
		ResetTransientSession();
		ConfigureActiveContent();
		return;
	}
	if (State.State == EPRRunLifecycleState::RunActive && State.RunId.IsValid() && State.AccountId.IsValid())
	{
		FrozenRunId = State.RunId;
		FrozenAccountId = State.AccountId;
		FrozenSeed = State.Seed;
		Snapshot.State = EPRChapterLifecycleState::RunActive;
		// AccountReady already selected and configured the closed registry before
		// RunState publishes RunActive. Reconfiguring here can race RoomSubsystem's
		// first flow transition and incorrectly turn a valid Warden selection into Busy.
		if (!ActiveDefinition.ChapterId.IsValid() && !ConfigureActiveContent()) return;
		Snapshot.ChapterId = ActiveDefinition.ChapterId;
		Snapshot.ContentId = ActiveDefinition.ContentId;
		Snapshot.DirectiveId = UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(ActiveDefinition.ContentId, State.Seed);
		Snapshot.AllocationPressure = 0;
		Snapshot.RiskPressure = 0;
		Snapshot.FallbackReason = NAME_None;
		if (UPRRoomSubsystem* Room = GetGameInstance()->GetSubsystem<UPRRoomSubsystem>())
		{
			Room->ConfigureContentContext(ActiveDefinition.ContentId, Snapshot.DirectiveId, 0);
		}
		RefreshWardenStoryProjection();
		PublishState();
	}
}

void UPRChapterSubsystem::HandleRoomCompleted(const FPRRoomSequenceCompleted& Completion)
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive || Completion.Seed != FrozenSeed || !IsActiveSequence(Completion)) return;
	bRoomSequenceVerified = true;
}

void UPRChapterSubsystem::HandleRoomEventResolved(const FPRRoomEventResult& Result)
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive) return;
	const UPRChapterRoguelikeContentRegistryDataAsset* Registry = PRChapterSubsystemPrivate::LoadRoomRegistry(ActiveDefinition.RoomRegistryId);
	int32 Delta = 0;
	if (!Registry || !Registry->FindPressureDelta(Result.EventId, Result.ChoiceId, Delta)) return;
	if (ActiveDefinition.bIsWarden) Snapshot.RiskPressure = FMath::Clamp(Snapshot.RiskPressure + Delta, 0, 4);
	else Snapshot.AllocationPressure = FMath::Clamp(Snapshot.AllocationPressure + Delta, 0, 4);
	PublishState();
}

void UPRChapterSubsystem::HandleBossCompleted(const FPRPrototypeRunResult& Completion)
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive || bBossVerified || !IsExpectedBossCompletion(Completion)) return;
	bBossVerified = true;
	VerifiedBossCompletionId = Completion.CompletionId;
	VerifiedBossSpawnId = Completion.BossSpawnId;
}

void UPRChapterSubsystem::HandleAccountDeleted(const FPRAccountDeletedEvent& Event)
{
	const FPRAccountRecord& Record = Event.Record;
	if (Snapshot.State != EPRChapterLifecycleState::RunActive || !bRoomSequenceVerified || !bBossVerified
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
		if (ActiveDefinition.EnemyRegistryId.IsValid()) Enemies->ConfigureContentRegistry(ActiveDefinition.EnemyRegistryId);
	}
	EnsureWardenPresentation();
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
	ClearWardenPresentation();
	FrozenRunId.Invalidate();
	FrozenAccountId.Invalidate();
	FrozenSeed = 0;
	bRoomSequenceVerified = false;
	bBossVerified = false;
	VerifiedBossCompletionId.Invalidate();
	VerifiedBossSpawnId.Invalidate();
	ActiveDefinition = FActiveChapterDefinition();
	PendingSettlement = FPendingSettlement();
	Snapshot = FPRChapterSnapshot();
}

bool UPRChapterSubsystem::SelectActiveDefinition(FActiveChapterDefinition& OutDefinition) const
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Persistence;
	if (!Save || !Save->GetChapterPersistenceSnapshot(Persistence)) return false;
	const bool bAllocatorProof = Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAllocatorProofId());
	if (!bAllocatorProof)
	{
		OutDefinition.ChapterId = UPRChapterContentRegistryDataAsset::GetAllocatorChapterId();
		OutDefinition.RoomRegistryId = PRChapterSubsystemPrivate::AllocatorRoomRegistryId;
		OutDefinition.EnemyRegistryId = PRChapterSubsystemPrivate::AllocatorEnemyRegistryId;
		OutDefinition.FinalRoomId = PRChapterSubsystemPrivate::AllocatorFinalRoomId;
		OutDefinition.ContentId = UPRChapterContentRegistryDataAsset::GetAllocatorContentId();
		OutDefinition.BossId = UPRChapterContentRegistryDataAsset::GetAllocatorBossId();
		OutDefinition.ProofId = UPRChapterContentRegistryDataAsset::GetAllocatorProofId();
		return true;
	}
	const UPRWardenChapterDataAsset* Warden = PRChapterSubsystemPrivate::LoadWardenDefinition();
	if (!Warden || !Warden->IsWardenDefinitionValid()) return false;
	OutDefinition.ChapterId = Warden->ChapterId;
	OutDefinition.RoomRegistryId = Warden->RoomContentRegistryId;
	OutDefinition.EnemyRegistryId = Warden->EnemyContentRegistryId;
	OutDefinition.FinalRoomId = UPRChapterContentRegistryDataAsset::GetWardenFinalRoomId();
	OutDefinition.ContentId = Warden->ContentId;
	OutDefinition.BossId = Warden->BossId;
	OutDefinition.ProofId = Warden->ProofId;
	OutDefinition.bIsWarden = true;
	return true;
}

bool UPRChapterSubsystem::ConfigureActiveContent()
{
	FActiveChapterDefinition Selected;
	UPRRoomSubsystem* Room = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	if (!Room || !SelectActiveDefinition(Selected))
	{
		Snapshot.State = EPRChapterLifecycleState::Rejected;
		Snapshot.FallbackReason = TEXT("Chapter.RegistryUnavailable");
		PublishState();
		return false;
	}
	if (Selected.bIsWarden && !PRChapterSubsystemPrivate::EnsureWardenEnemyRegistryIsRegistered())
	{
		Snapshot.State = EPRChapterLifecycleState::Rejected;
		Snapshot.FallbackReason = TEXT("Chapter.EnemyRegistryUnavailable");
		PublishState();
		return false;
	}
	const EPRRoomContentResult Result = Room->ConfigureContentRegistry(Selected.RoomRegistryId);
	if (Result != EPRRoomContentResult::Succeeded && Room->GetConfiguredContentRegistryId() != Selected.RoomRegistryId)
	{
		Snapshot.State = EPRChapterLifecycleState::Rejected;
		Snapshot.FallbackReason = TEXT("Chapter.RoomRegistryUnavailable");
		PublishState();
		return false;
	}
	ActiveDefinition = Selected;
	Snapshot.ChapterId = Selected.ChapterId;
	Snapshot.ContentId = Selected.ContentId;
	Snapshot.bHasHumanAnomalyProof = false;
	// A completed Warden proof remains replayable, but no subsequent completion can stage a new transaction.
	FPRChapterPersistenceData Persistence;
	if (UPRSaveSubsystem* Save = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>())
	{
		if (Save->GetChapterPersistenceSnapshot(Persistence)) Snapshot.bHasHumanAnomalyProof = Persistence.HumanAnomalyProofIds.Contains(Selected.ProofId);
	}
	Snapshot.State = EPRChapterLifecycleState::Configured;
	RefreshWardenStoryProjection();
	EnsureWardenPresentation();
	PublishState();
	return true;
}

bool UPRChapterSubsystem::IsActiveSequence(const FPRRoomSequenceCompleted& Completion) const
{
	if (!ActiveDefinition.FinalRoomId.IsValid() || Completion.CompletedPath.Num() < 6 || Completion.CompletedPath.Num() > 10) return false;
	if (Completion.CompletedPath.Last().SelectedRoomId != ActiveDefinition.FinalRoomId) return false;
	const UPRChapterRoguelikeContentRegistryDataAsset* Registry = PRChapterSubsystemPrivate::LoadRoomRegistry(ActiveDefinition.RoomRegistryId);
	if (!Registry || !Registry->IsRegistryReady()) return false;
	for (const FPRRoomPathStep& Step : Completion.CompletedPath) if (!Registry->FindRoom(Step.SelectedRoomId)) return false;
	return true;
}

bool UPRChapterSubsystem::IsExpectedBossCompletion(const FPRPrototypeRunResult& Completion) const
{
	if (!Completion.CompletionId.IsValid() || !Completion.BossSpawnId.IsValid() || Completion.BossId != ActiveDefinition.BossId) return false;
	UPRRoomSubsystem* Room = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	FGuid ExpectedSpawn;
	return Room && Room->GetExpectedBossSpawnId(ExpectedSpawn) && ExpectedSpawn == Completion.BossSpawnId;
}

bool UPRChapterSubsystem::BeginSettlement()
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Current;
	if (!Save || !Save->GetChapterPersistenceSnapshot(Current)) return false;
	if (Current.CompletedChapterIds.Contains(ActiveDefinition.ChapterId) || Current.HumanAnomalyProofIds.Contains(ActiveDefinition.ProofId))
	{
		Snapshot.State = EPRChapterLifecycleState::Completed;
		Snapshot.bHasHumanAnomalyProof = true;
		PublishState();
		return true;
	}
	if (Current.CompletedChapterIds.Num() >= FPRChapterPersistenceContract::MaxEntries || Current.HumanAnomalyProofIds.Num() >= FPRChapterPersistenceContract::MaxEntries || Current.SettlementSequence == MAX_int64) return false;
	FPendingSettlement Transaction;
	Transaction.Expected = Current;
	Transaction.Target = Current;
	Transaction.Target.CompletedChapterIds.Add(ActiveDefinition.ChapterId);
	Transaction.Target.HumanAnomalyProofIds.Add(ActiveDefinition.ProofId);
	Transaction.Target.SettlementSequence = Current.SettlementSequence + 1;
	FPRChapterPersistenceContract::Normalize(Transaction.Target);
	Transaction.Completion.CompletionId = FGuid::NewGuid();
	Transaction.Completion.ChapterId = ActiveDefinition.ChapterId;
	Transaction.Completion.ProofId = ActiveDefinition.ProofId;
	Transaction.Completion.SettlementSequence = Transaction.Target.SettlementSequence;
	Transaction.Completion.bProofAwarded = true;
	Transaction.bPending = true;
	PendingSettlement = MoveTemp(Transaction);
	return SubmitPendingSettlement();
}

bool UPRChapterSubsystem::SubmitPendingSettlement()
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	if (!Save || !PendingSettlement.bPending) return false;
	PendingSettlement.SaveRequestId.Invalidate();
	if (!Save->StageChapterPersistenceTransaction(PendingSettlement.Expected, PendingSettlement.Target)) return false;
	if (Save->RequestSaveCurrentProfile(PendingSettlement.SaveRequestId) != EPRSaveRequestStatus::Started) return false;
	return true;
}

void UPRChapterSubsystem::RefreshWardenStoryProjection()
{
	Snapshot.WardenStory = FPRWardenStoryProjection();
	if (!ActiveDefinition.bIsWarden) return;
	const UPRWardenChapterDataAsset* Warden = PRChapterSubsystemPrivate::LoadWardenDefinition();
	UPRCompanionSubsystem* Companions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	UPRCompanionQuestSubsystem* Quests = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionQuestSubsystem>() : nullptr;
	UPRMemorySubsystem* Memory = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRMemorySubsystem>() : nullptr;
	FPRMemorySummary LatestSummary;
	FPRCompanionRelationshipRecord AxiomRelationship;
	FPRCompanionQuestEntitlementSnapshot QuestEntitlements;
	const FGameplayTag AxiomId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false);
	const bool bAxiomRelationshipAvailable = Companions && Companions->GetRelationshipSnapshot(AxiomId, AxiomRelationship);
	const bool bQuestEntitlementsAvailable = Quests && Quests->GetEntitlementSnapshot(QuestEntitlements);
	const bool bDependencies = Warden && Companions && Quests && Memory && bAxiomRelationshipAvailable && bQuestEntitlementsAvailable && Memory->GetLatestSummary(LatestSummary);
	const bool bAxiomPrimary = Companions && Companions->GetSyncState().PrimaryCompanionId == AxiomId;
	const bool bLowProbability = Quests && Quests->IsQuestCompleted(TEXT("Quest.Axiom.LowProbabilitySample"));
	const bool bImperfect = Quests && Quests->IsQuestCompleted(TEXT("Quest.Axiom.ImperfectOptimum"))
		&& QuestEntitlements.EntitlementIds.Contains(TEXT("Line:Axiom_ImperfectOptimum"));
	if (Warden) Snapshot.WardenStory = Warden->BuildStoryProjection(bAxiomPrimary, bLowProbability, bImperfect, bDependencies);
}

void UPRChapterSubsystem::ClearWardenPresentation()
{
	if (UPRWardenChapterWidget* Overlay = WardenOverlay.Get()) Overlay->RemoveFromParent();
	WardenOverlay.Reset();
}

void UPRChapterSubsystem::EnsureWardenPresentation()
{
	if (!ActiveDefinition.bIsWarden || WardenOverlay.IsValid()) return;
	const UPRWardenChapterDataAsset* Warden = PRChapterSubsystemPrivate::LoadWardenDefinition();
	UClass* OverlayClass = Warden ? Warden->OverlayWidgetClass.LoadSynchronous() : nullptr;
	UWorld* World = BoundWorld.Get();
	APlayerController* Controller = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!OverlayClass || !Controller) return;
	if (UPRWardenChapterWidget* Overlay = CreateWidget<UPRWardenChapterWidget>(Controller, OverlayClass))
	{
		WardenOverlay = Overlay;
		Overlay->AddToViewport(20);
	}
}
