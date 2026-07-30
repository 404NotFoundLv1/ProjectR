// Copyright ProjectR. All Rights Reserved.

#include "Chapters/PRChapterSubsystem.h"

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/Auditor/PRAuditorChapterDataAsset.h"
#include "Chapters/Headmind/PRHeadmindChapterDataAsset.h"
#include "Chapters/Headmind/PRHeadmindEndingEvaluator.h"
#include "Chapters/Headmind/PRHeadmindProjectionBoss.h"
#include "Chapters/Pacifier/PRPacifierChapterDataAsset.h"
#include "Chapters/Warden/PRWardenChapterDataAsset.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Containers/Ticker.h"
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
#include "Roguelike/Progression/PRProgressionSubsystem.h"
#include "Save/PRSaveSubsystem.h"
#include "UI/PRPacifierChapterWidget.h"
#include "UI/PRAuditorChapterWidget.h"
#include "UI/PRHeadmindChapterWidget.h"
#include "UI/PRWardenChapterWidget.h"
#include "Kismet/GameplayStatics.h"

namespace PRChapterSubsystemPrivate
{
	const FPrimaryAssetId AllocatorRoomRegistryId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Allocator"));
	const FPrimaryAssetId AllocatorEnemyRegistryId(TEXT("ProjectREnemyContentRegistry"), TEXT("DA_EnemyContentRegistry_Allocator"));
	const FPrimaryAssetId AllocatorFinalRoomId(TEXT("ProjectRRoom"), TEXT("DA_Room_Allocator_Boss_Allocator"));
	const FSoftObjectPath WardenEnemyRegistryPath(TEXT("/Game/ProjectR/Chapters/Warden/DA_EnemyContentRegistry_Warden.DA_EnemyContentRegistry_Warden"));
	const FSoftObjectPath PacifierEnemyRegistryPath(TEXT("/Game/ProjectR/Chapters/Pacifier/DA_EnemyContentRegistry_Pacifier.DA_EnemyContentRegistry_Pacifier"));
	const FSoftObjectPath AuditorEnemyRegistryPath(TEXT("/Game/ProjectR/Chapters/Auditor/DA_EnemyContentRegistry_Auditor.DA_EnemyContentRegistry_Auditor"));
	const FSoftObjectPath HeadmindEnemyRegistryPath(TEXT("/Game/ProjectR/Chapters/Headmind/DA_EnemyContentRegistry_Headmind.DA_EnemyContentRegistry_Headmind"));

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
		if (RegistryId == UPRChapterContentRegistryDataAsset::GetPacifierRoomRegistryId())
		{
			return LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Pacifier/DA_RoguelikeContentRegistry_Pacifier.DA_RoguelikeContentRegistry_Pacifier"));
		}
		if (RegistryId == UPRChapterContentRegistryDataAsset::GetAuditorRoomRegistryId())
		{
			return LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Auditor/DA_RoguelikeContentRegistry_Auditor.DA_RoguelikeContentRegistry_Auditor"));
		}
		if (RegistryId == UPRChapterContentRegistryDataAsset::GetHeadmindRoomRegistryId())
		{
			return LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Headmind/DA_RoguelikeContentRegistry_Headmind.DA_RoguelikeContentRegistry_Headmind"));
		}
		return nullptr;
	}

	const UPRWardenChapterDataAsset* LoadWardenDefinition()
	{
		const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(UPRChapterContentRegistryDataAsset::GetWardenChapterId());
		if (const UPRWardenChapterDataAsset* Definition = Path.IsValid() ? Cast<UPRWardenChapterDataAsset>(Path.TryLoad()) : nullptr) return Definition;
		return LoadObject<UPRWardenChapterDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Warden/DA_Chapter_Warden.DA_Chapter_Warden"));
	}

	const UPRPacifierChapterDataAsset* LoadPacifierDefinition()
	{
		const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(UPRChapterContentRegistryDataAsset::GetPacifierChapterId());
		if (const UPRPacifierChapterDataAsset* Definition = Path.IsValid() ? Cast<UPRPacifierChapterDataAsset>(Path.TryLoad()) : nullptr) return Definition;
		return LoadObject<UPRPacifierChapterDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Pacifier/DA_Chapter_Pacifier.DA_Chapter_Pacifier"));
	}

	const UPRAuditorChapterDataAsset* LoadAuditorDefinition()
	{
		const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(UPRChapterContentRegistryDataAsset::GetAuditorChapterId());
		if (const UPRAuditorChapterDataAsset* Definition = Path.IsValid() ? Cast<UPRAuditorChapterDataAsset>(Path.TryLoad()) : nullptr) return Definition;
		return LoadObject<UPRAuditorChapterDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Auditor/DA_Chapter_Auditor.DA_Chapter_Auditor"));
	}

	const UPRHeadmindChapterDataAsset* LoadHeadmindDefinition()
	{
		const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(UPRChapterContentRegistryDataAsset::GetHeadmindChapterId());
		if (const UPRHeadmindChapterDataAsset* Definition = Path.IsValid() ? Cast<UPRHeadmindChapterDataAsset>(Path.TryLoad()) : nullptr) return Definition;
		return LoadObject<UPRHeadmindChapterDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Headmind/DA_Chapter_Headmind.DA_Chapter_Headmind"));
	}

	bool EnsureFixedEnemyRegistryIsRegistered(const FPrimaryAssetId& RegistryId, const FSoftObjectPath& RegistryPath, const FName BundleName)
	{
		UAssetManager& AssetManager = UAssetManager::Get();
		if (AssetManager.GetPrimaryAssetPath(RegistryId).IsValid()) return true;
		FAssetBundleData BundleData;
		BundleData.AddBundleAsset(BundleName, RegistryPath.GetAssetPath());
		if (!AssetManager.AddDynamicAsset(RegistryId, RegistryPath, BundleData)) return false;
		return Cast<UPREnemyContentRegistryDataAsset>(RegistryPath.TryLoad()) != nullptr
			&& AssetManager.GetPrimaryAssetPath(RegistryId) == RegistryPath;
	}

	bool EnsureWardenEnemyRegistryIsRegistered()
	{
		return EnsureFixedEnemyRegistryIsRegistered(
			UPRChapterContentRegistryDataAsset::GetWardenEnemyRegistryId(),
			WardenEnemyRegistryPath,
			TEXT("WardenRegistry"));
	}

	bool EnsurePacifierEnemyRegistryIsRegistered()
	{
		return EnsureFixedEnemyRegistryIsRegistered(
			UPRChapterContentRegistryDataAsset::GetPacifierEnemyRegistryId(),
			PacifierEnemyRegistryPath,
			TEXT("PacifierRegistry"));
	}

	bool EnsureAuditorEnemyRegistryIsRegistered()
	{
		return EnsureFixedEnemyRegistryIsRegistered(
			UPRChapterContentRegistryDataAsset::GetAuditorEnemyRegistryId(),
			AuditorEnemyRegistryPath,
			TEXT("AuditorRegistry"));
	}

	bool EnsureHeadmindEnemyRegistryIsRegistered()
	{
		return EnsureFixedEnemyRegistryIsRegistered(
			UPRChapterContentRegistryDataAsset::GetHeadmindEnemyRegistryId(),
			HeadmindEnemyRegistryPath,
			TEXT("HeadmindRegistry"));
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
	if (SettlementRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SettlementRetryTickerHandle);
		SettlementRetryTickerHandle.Reset();
	}
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

bool UPRChapterSubsystem::StageFixedPacifierPrerequisitesForAutomation()
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Current;
	if (!Save || !UPRSaveSubsystem::HasAutomationStorageOverride() || !Save->GetChapterPersistenceSnapshot(Current)
		|| !IsFixedProofChainValid(Current)
		|| Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetPacifierProofId()))
	{
		return false;
	}
	const FPrimaryAssetId AllocatorChapterId = UPRChapterContentRegistryDataAsset::GetAllocatorChapterId();
	const FPrimaryAssetId WardenChapterId = UPRChapterContentRegistryDataAsset::GetWardenChapterId();
	const FName AllocatorProofId = UPRChapterContentRegistryDataAsset::GetAllocatorProofId();
	const FName WardenProofId = UPRChapterContentRegistryDataAsset::GetWardenProofId();
	int32 MissingSettlements = 0;
	MissingSettlements += Current.HumanAnomalyProofIds.Contains(AllocatorProofId) ? 0 : 1;
	MissingSettlements += Current.HumanAnomalyProofIds.Contains(WardenProofId) ? 0 : 1;
	if (MissingSettlements == 0) return true;
	if (Current.CompletedChapterIds.Num() + MissingSettlements > FPRChapterPersistenceContract::MaxEntries
		|| Current.HumanAnomalyProofIds.Num() + MissingSettlements > FPRChapterPersistenceContract::MaxEntries
		|| Current.SettlementSequence > MAX_int64 - MissingSettlements)
	{
		return false;
	}
	FPRChapterPersistenceData Target = Current;
	Target.CompletedChapterIds.Add(AllocatorChapterId);
	Target.CompletedChapterIds.Add(WardenChapterId);
	Target.HumanAnomalyProofIds.Add(AllocatorProofId);
	Target.HumanAnomalyProofIds.Add(WardenProofId);
	Target.SettlementSequence = Current.SettlementSequence + MissingSettlements;
	FPRChapterPersistenceContract::Normalize(Target);
	if (!IsFixedProofChainValid(Target)) return false;
	FGuid RequestId;
	return Save->StageChapterPersistenceTransaction(Current, Target)
		&& Save->RequestSaveCurrentProfile(RequestId) == EPRSaveRequestStatus::Started;
}

bool UPRChapterSubsystem::StageFixedAuditorPrerequisitesForAutomation()
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Current;
	if (!Save || !UPRSaveSubsystem::HasAutomationStorageOverride() || !Save->GetChapterPersistenceSnapshot(Current)
		|| !IsFixedProofChainValid(Current)
		|| Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAuditorProofId()))
	{
		return false;
	}
	struct FFixedPrerequisite
	{
		FPrimaryAssetId ChapterId;
		FName ProofId;
	};
	const TArray<FFixedPrerequisite> Required = {
		{ UPRChapterContentRegistryDataAsset::GetAllocatorChapterId(), UPRChapterContentRegistryDataAsset::GetAllocatorProofId() },
		{ UPRChapterContentRegistryDataAsset::GetWardenChapterId(), UPRChapterContentRegistryDataAsset::GetWardenProofId() },
		{ UPRChapterContentRegistryDataAsset::GetPacifierChapterId(), UPRChapterContentRegistryDataAsset::GetPacifierProofId() }};
	int32 MissingSettlements = 0;
	for (const FFixedPrerequisite& Entry : Required)
	{
		MissingSettlements += Current.HumanAnomalyProofIds.Contains(Entry.ProofId) ? 0 : 1;
	}
	if (MissingSettlements == 0) return true;
	if (Current.CompletedChapterIds.Num() + MissingSettlements > FPRChapterPersistenceContract::MaxEntries
		|| Current.HumanAnomalyProofIds.Num() + MissingSettlements > FPRChapterPersistenceContract::MaxEntries
		|| Current.SettlementSequence > MAX_int64 - MissingSettlements)
	{
		return false;
	}
	FPRChapterPersistenceData Target = Current;
	for (const FFixedPrerequisite& Entry : Required)
	{
		Target.CompletedChapterIds.Add(Entry.ChapterId);
		Target.HumanAnomalyProofIds.Add(Entry.ProofId);
	}
	Target.SettlementSequence = Current.SettlementSequence + MissingSettlements;
	FPRChapterPersistenceContract::Normalize(Target);
	if (!IsFixedProofChainValid(Target)) return false;
	FGuid RequestId;
	return Save->StageChapterPersistenceTransaction(Current, Target)
		&& Save->RequestSaveCurrentProfile(RequestId) == EPRSaveRequestStatus::Started;
}

bool UPRChapterSubsystem::RefreshFixedAuditorSelectionForAutomation()
{
	UPRRunStateSubsystem* RunState = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
	if (!UPRSaveSubsystem::HasAutomationStorageOverride()
		|| !RunState
		|| RunState->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady)
	{
		return false;
	}
	ResetTransientSession();
	return ConfigureActiveContent()
		&& ActiveDefinition.FixedContent == FActiveChapterDefinition::EFixedContent::Auditor;
}

bool UPRChapterSubsystem::StageFixedHeadmindPrerequisitesForAutomation()
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Current;
	if (!Save || !UPRSaveSubsystem::HasAutomationStorageOverride() || !Save->GetChapterPersistenceSnapshot(Current)
		|| !IsFixedProofChainValid(Current)
		|| Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetHeadmindProofId()))
	{
		return false;
	}
	struct FFixedPrerequisite
	{
		FPrimaryAssetId ChapterId;
		FName ProofId;
	};
	const TArray<FFixedPrerequisite> Required = {
		{ UPRChapterContentRegistryDataAsset::GetAllocatorChapterId(), UPRChapterContentRegistryDataAsset::GetAllocatorProofId() },
		{ UPRChapterContentRegistryDataAsset::GetWardenChapterId(), UPRChapterContentRegistryDataAsset::GetWardenProofId() },
		{ UPRChapterContentRegistryDataAsset::GetPacifierChapterId(), UPRChapterContentRegistryDataAsset::GetPacifierProofId() },
		{ UPRChapterContentRegistryDataAsset::GetAuditorChapterId(), UPRChapterContentRegistryDataAsset::GetAuditorProofId() }};
	int32 MissingSettlements = 0;
	for (const FFixedPrerequisite& Entry : Required)
	{
		MissingSettlements += Current.HumanAnomalyProofIds.Contains(Entry.ProofId) ? 0 : 1;
	}
	if (MissingSettlements == 0) return true;
	if (Current.CompletedChapterIds.Num() + MissingSettlements > FPRChapterPersistenceContract::MaxEntries
		|| Current.HumanAnomalyProofIds.Num() + MissingSettlements > FPRChapterPersistenceContract::MaxEntries
		|| Current.SettlementSequence > MAX_int64 - MissingSettlements)
	{
		return false;
	}
	FPRChapterPersistenceData Target = Current;
	for (const FFixedPrerequisite& Entry : Required)
	{
		Target.CompletedChapterIds.Add(Entry.ChapterId);
		Target.HumanAnomalyProofIds.Add(Entry.ProofId);
	}
	Target.SettlementSequence = Current.SettlementSequence + MissingSettlements;
	FPRChapterPersistenceContract::Normalize(Target);
	if (!IsFixedProofChainValid(Target)) return false;
	FGuid RequestId;
	return Save->StageChapterPersistenceTransaction(Current, Target)
		&& Save->RequestSaveCurrentProfile(RequestId) == EPRSaveRequestStatus::Started;
}

bool UPRChapterSubsystem::RefreshFixedHeadmindSelectionForAutomation()
{
	UPRRunStateSubsystem* RunState = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
	if (!UPRSaveSubsystem::HasAutomationStorageOverride()
		|| !RunState
		|| RunState->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady)
	{
		return false;
	}
	ResetTransientSession();
	return ConfigureActiveContent()
		&& ActiveDefinition.FixedContent == FActiveChapterDefinition::EFixedContent::Headmind;
}

bool UPRChapterSubsystem::StageFixedPacifierCompletionFactsForAutomation()
{
	if (!UPRSaveSubsystem::HasAutomationStorageOverride()
		|| Snapshot.State != EPRChapterLifecycleState::RunActive
		|| ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Pacifier
		|| !FrozenRunId.IsValid()
		|| !FrozenAccountId.IsValid()
		|| FrozenSeed < 61200
		|| FrozenSeed > 61204)
	{
		return false;
	}
	bRoomSequenceVerified = true;
	bBossVerified = true;
	return true;
}

bool UPRChapterSubsystem::StageFixedAuditorCompletionFactsForAutomation()
{
	if (!UPRSaveSubsystem::HasAutomationStorageOverride()
		|| Snapshot.State != EPRChapterLifecycleState::RunActive
		|| ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Auditor
		|| !FrozenRunId.IsValid()
		|| !FrozenAccountId.IsValid()
		|| FrozenSeed < 61300
		|| FrozenSeed > 61304)
	{
		return false;
	}
	bRoomSequenceVerified = true;
	bBossVerified = true;
	return true;
}

bool UPRChapterSubsystem::StageFixedHeadmindCompletionFactsForAutomation()
{
	if (!UPRSaveSubsystem::HasAutomationStorageOverride()
		|| Snapshot.State != EPRChapterLifecycleState::RunActive
		|| ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Headmind
		|| !FrozenRunId.IsValid()
		|| !FrozenAccountId.IsValid()
		|| FrozenSeed < 61400
		|| FrozenSeed > 61404)
	{
		return false;
	}
	bRoomSequenceVerified = true;
	bBossVerified = true;
	return true;
}

void UPRChapterSubsystem::GetFixedPacifierSettlementDiagnosticsForAutomation(
	bool& bOutRoomVerified,
	bool& bOutBossVerified,
	bool& bOutAccountDeletedVerified,
	bool& bOutSettlementRequested,
	bool& bOutSettlementPending) const
{
	bOutRoomVerified = bRoomSequenceVerified;
	bOutBossVerified = bBossVerified;
	bOutAccountDeletedVerified = bAccountDeletedVerified;
	bOutSettlementRequested = bSettlementRequested;
	bOutSettlementPending = PendingSettlement.bPending;
}

void UPRChapterSubsystem::GetFixedAuditorSettlementDiagnosticsForAutomation(
	bool& bOutRoomVerified,
	bool& bOutBossVerified,
	bool& bOutAccountDeletedVerified,
	bool& bOutSettlementRequested,
	bool& bOutSettlementPending) const
{
	bOutRoomVerified = bRoomSequenceVerified;
	bOutBossVerified = bBossVerified;
	bOutAccountDeletedVerified = bAccountDeletedVerified;
	bOutSettlementRequested = bSettlementRequested;
	bOutSettlementPending = PendingSettlement.bPending;
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
		Snapshot.ComfortPressure = 0;
		Snapshot.AuditPressure = 0;
		Snapshot.SynthesisPressure = 0;
		Snapshot.FallbackReason = NAME_None;
		if (UPRRoomSubsystem* Room = GetGameInstance()->GetSubsystem<UPRRoomSubsystem>())
		{
			Room->ConfigureContentContext(ActiveDefinition.ContentId, Snapshot.DirectiveId, 0);
		}
		RefreshWardenStoryProjection();
		RefreshPacifierStoryProjection();
		RefreshAuditorStoryProjection();
		RefreshHeadmindPresentation();
		PublishState();
	}
}

void UPRChapterSubsystem::HandleRoomCompleted(const FPRRoomSequenceCompleted& Completion)
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive || Completion.Seed != FrozenSeed || !IsActiveSequence(Completion)) return;
	bRoomSequenceVerified = true;
	TryBeginSettlement();
}

void UPRChapterSubsystem::HandleRoomEventResolved(const FPRRoomEventResult& Result)
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive) return;
	const UPRChapterRoguelikeContentRegistryDataAsset* Registry = PRChapterSubsystemPrivate::LoadRoomRegistry(ActiveDefinition.RoomRegistryId);
	int32 Delta = 0;
	if (!Registry || !Registry->FindPressureDelta(Result.EventId, Result.ChoiceId, Delta)) return;
	switch (ActiveDefinition.FixedContent)
	{
	case FActiveChapterDefinition::EFixedContent::Warden:
		Snapshot.RiskPressure = FMath::Clamp(Snapshot.RiskPressure + Delta, 0, 4);
		break;
	case FActiveChapterDefinition::EFixedContent::Pacifier:
		Snapshot.ComfortPressure = FMath::Clamp(Snapshot.ComfortPressure + Delta, 0, 4);
		break;
	case FActiveChapterDefinition::EFixedContent::Auditor:
		Snapshot.AuditPressure = FMath::Clamp(Snapshot.AuditPressure + Delta, 0, 4);
		break;
	case FActiveChapterDefinition::EFixedContent::Headmind:
		Snapshot.SynthesisPressure = FMath::Clamp(Snapshot.SynthesisPressure + Delta, 0, 4);
		break;
	default:
		Snapshot.AllocationPressure = FMath::Clamp(Snapshot.AllocationPressure + Delta, 0, 4);
		break;
	}
	PublishState();
}

void UPRChapterSubsystem::HandleBossCompleted(const FPRPrototypeRunResult& Completion)
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive || bBossVerified || !IsExpectedBossCompletion(Completion)) return;
	bBossVerified = true;
	VerifiedBossCompletionId = Completion.CompletionId;
	VerifiedBossSpawnId = Completion.BossSpawnId;
	TryBeginSettlement();
}

void UPRChapterSubsystem::HandleAccountDeleted(const FPRAccountDeletedEvent& Event)
{
	const FPRAccountRecord& Record = Event.Record;
	if (Snapshot.State != EPRChapterLifecycleState::RunActive
		|| Record.TerminationReason != EPRAccountTerminationReason::RoomSequenceCompleted
		|| Record.AccountId != FrozenAccountId || Record.Summary.RunId != FrozenRunId || Record.Summary.Seed != FrozenSeed) return;
	bAccountDeletedVerified = true;
	if (ActiveDefinition.FixedContent == FActiveChapterDefinition::EFixedContent::Headmind)
	{
		FPRProgressionSnapshot ProgressionSnapshot;
		UPRProgressionSubsystem* Progression = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRProgressionSubsystem>() : nullptr;
		if (!Progression || !Progression->GetProgressionSnapshot(ProgressionSnapshot)
			|| !FPRHeadmindEndingEvaluator::BuildInput(Record.Summary, ProgressionSnapshot, FrozenHeadmindEndingInput))
		{
			bAccountDeletedVerified = false;
			Snapshot.FallbackReason = FrozenHeadmindEndingInput.FallbackReason.IsNone() ? TEXT("Headmind.EndingInputUnavailable") : FrozenHeadmindEndingInput.FallbackReason;
			PublishState();
			return;
		}
		if (const UPRHeadmindChapterDataAsset* Headmind = PRChapterSubsystemPrivate::LoadHeadmindDefinition())
		{
			FText IgnoredText;
			Headmind->ResolveEndingParagraph(FrozenHeadmindEndingInput, Snapshot.HeadmindEnding, IgnoredText);
		}
	}
	TryBeginSettlement();
}

void UPRChapterSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if (PendingSettlement.bPending
		&& Event.Operation == EPRSaveOperationType::Save
		&& Event.RequestId == PendingSettlement.SaveRequestId)
	{
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
		return;
	}
	if (bSettlementRequested
		&& Event.Operation == EPRSaveOperationType::Save
		&& Event.Result == EPRSaveResult::Success)
	{
		if (PendingSettlement.bPending && !PendingSettlement.SaveRequestId.IsValid())
		{
			if (SubmitPendingSettlement()) bSettlementRequested = false;
			else ScheduleDeferredSettlementSubmit();
		}
		else if (!PendingSettlement.bPending && BeginSettlement())
		{
			bSettlementRequested = false;
		}
	}
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
	EnsurePacifierPresentation();
	EnsureAuditorPresentation();
	EnsureHeadmindPresentation();
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
	ClearPacifierPresentation();
	ClearAuditorPresentation();
	ClearHeadmindPresentation();
	FrozenRunId.Invalidate();
	FrozenAccountId.Invalidate();
	FrozenSeed = 0;
	bRoomSequenceVerified = false;
	bBossVerified = false;
	bAccountDeletedVerified = false;
	VerifiedBossCompletionId.Invalidate();
	VerifiedBossSpawnId.Invalidate();
	ActiveDefinition = FActiveChapterDefinition();
	PendingSettlement = FPendingSettlement();
	FrozenHeadmindEndingInput = FPRHeadmindEndingInputSnapshot();
	bSettlementRequested = false;
	if (SettlementRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SettlementRetryTickerHandle);
		SettlementRetryTickerHandle.Reset();
	}
	Snapshot = FPRChapterSnapshot();
}

bool UPRChapterSubsystem::SelectActiveDefinition(FActiveChapterDefinition& OutDefinition, FName& OutFailureReason) const
{
	OutFailureReason = NAME_None;
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Persistence;
	if (!Save || !Save->GetChapterPersistenceSnapshot(Persistence)) return false;
	if (!IsFixedProofChainValid(Persistence))
	{
		OutFailureReason = TEXT("Chapter.InvalidProofChain");
		return false;
	}
	const bool bAllocatorProof = Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAllocatorProofId());
	const bool bWardenProof = Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetWardenProofId());
	const bool bPacifierProof = Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetPacifierProofId());
	const bool bAuditorProof = Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAuditorProofId());
	const bool bHeadmindProof = Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetHeadmindProofId());
	if ((bWardenProof && !bAllocatorProof) || (bPacifierProof && (!bAllocatorProof || !bWardenProof)) || (bAuditorProof && (!bAllocatorProof || !bWardenProof || !bPacifierProof)) || (bHeadmindProof && (!bAllocatorProof || !bWardenProof || !bPacifierProof || !bAuditorProof)))
	{
		OutFailureReason = TEXT("Chapter.InvalidProofChain");
		return false;
	}
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
	if (!bWardenProof)
	{
		const UPRWardenChapterDataAsset* Warden = PRChapterSubsystemPrivate::LoadWardenDefinition();
		if (!Warden || !Warden->IsWardenDefinitionValid()) return false;
		OutDefinition.ChapterId = Warden->ChapterId;
		OutDefinition.RoomRegistryId = Warden->RoomContentRegistryId;
		OutDefinition.EnemyRegistryId = Warden->EnemyContentRegistryId;
		OutDefinition.FinalRoomId = UPRChapterContentRegistryDataAsset::GetWardenFinalRoomId();
		OutDefinition.ContentId = Warden->ContentId;
		OutDefinition.BossId = Warden->BossId;
		OutDefinition.ProofId = Warden->ProofId;
		OutDefinition.FixedContent = FActiveChapterDefinition::EFixedContent::Warden;
		return true;
	}
	if (!bPacifierProof)
	{
		const UPRPacifierChapterDataAsset* Pacifier = PRChapterSubsystemPrivate::LoadPacifierDefinition();
		if (!Pacifier || !Pacifier->IsPacifierDefinitionValid()) return false;
		OutDefinition.ChapterId = Pacifier->ChapterId;
		OutDefinition.RoomRegistryId = Pacifier->RoomContentRegistryId;
		OutDefinition.EnemyRegistryId = Pacifier->EnemyContentRegistryId;
		OutDefinition.FinalRoomId = UPRChapterContentRegistryDataAsset::GetPacifierFinalRoomId();
		OutDefinition.ContentId = Pacifier->ContentId;
		OutDefinition.BossId = Pacifier->BossId;
		OutDefinition.ProofId = Pacifier->ProofId;
		OutDefinition.FixedContent = FActiveChapterDefinition::EFixedContent::Pacifier;
		return true;
	}
	if (!bAuditorProof)
	{
		const UPRAuditorChapterDataAsset* Auditor = PRChapterSubsystemPrivate::LoadAuditorDefinition();
		if (!Auditor || !Auditor->IsAuditorDefinitionValid()) return false;
		OutDefinition.ChapterId = Auditor->ChapterId;
		OutDefinition.RoomRegistryId = Auditor->RoomContentRegistryId;
		OutDefinition.EnemyRegistryId = Auditor->EnemyContentRegistryId;
		OutDefinition.FinalRoomId = UPRChapterContentRegistryDataAsset::GetAuditorFinalRoomId();
		OutDefinition.ContentId = Auditor->ContentId;
		OutDefinition.BossId = Auditor->BossId;
		OutDefinition.ProofId = Auditor->ProofId;
		OutDefinition.FixedContent = FActiveChapterDefinition::EFixedContent::Auditor;
		return true;
	}
	if (!bHeadmindProof)
	{
		const UPRHeadmindChapterDataAsset* Headmind = PRChapterSubsystemPrivate::LoadHeadmindDefinition();
		if (!Headmind || !Headmind->IsHeadmindDefinitionValid()) return false;
		OutDefinition.ChapterId = Headmind->ChapterId;
		OutDefinition.RoomRegistryId = Headmind->RoomContentRegistryId;
		OutDefinition.EnemyRegistryId = Headmind->EnemyContentRegistryId;
		OutDefinition.FinalRoomId = UPRChapterContentRegistryDataAsset::GetHeadmindFinalRoomId();
		OutDefinition.ContentId = Headmind->ContentId;
		OutDefinition.BossId = Headmind->BossId;
		OutDefinition.ProofId = Headmind->ProofId;
		OutDefinition.FixedContent = FActiveChapterDefinition::EFixedContent::Headmind;
		return true;
	}
	// A completed Headmind proof has no alternate selector: the final chapter remains replayable.
	const UPRHeadmindChapterDataAsset* Headmind = PRChapterSubsystemPrivate::LoadHeadmindDefinition();
	if (!Headmind || !Headmind->IsHeadmindDefinitionValid()) return false;
	OutDefinition.ChapterId = Headmind->ChapterId;
	OutDefinition.RoomRegistryId = Headmind->RoomContentRegistryId;
	OutDefinition.EnemyRegistryId = Headmind->EnemyContentRegistryId;
	OutDefinition.FinalRoomId = UPRChapterContentRegistryDataAsset::GetHeadmindFinalRoomId();
	OutDefinition.ContentId = Headmind->ContentId;
	OutDefinition.BossId = Headmind->BossId;
	OutDefinition.ProofId = Headmind->ProofId;
	OutDefinition.FixedContent = FActiveChapterDefinition::EFixedContent::Headmind;
	return true;
}

bool UPRChapterSubsystem::IsFixedProofChainValid(const FPRChapterPersistenceData& Persistence)
{
	const FPrimaryAssetId AllocatorChapterId = UPRChapterContentRegistryDataAsset::GetAllocatorChapterId();
	const FPrimaryAssetId WardenChapterId = UPRChapterContentRegistryDataAsset::GetWardenChapterId();
	const FPrimaryAssetId PacifierChapterId = UPRChapterContentRegistryDataAsset::GetPacifierChapterId();
	const FPrimaryAssetId AuditorChapterId = UPRChapterContentRegistryDataAsset::GetAuditorChapterId();
	const FPrimaryAssetId HeadmindChapterId = UPRChapterContentRegistryDataAsset::GetHeadmindChapterId();
	const FName AllocatorProofId = UPRChapterContentRegistryDataAsset::GetAllocatorProofId();
	const FName WardenProofId = UPRChapterContentRegistryDataAsset::GetWardenProofId();
	const FName PacifierProofId = UPRChapterContentRegistryDataAsset::GetPacifierProofId();
	const FName AuditorProofId = UPRChapterContentRegistryDataAsset::GetAuditorProofId();
	const FName HeadmindProofId = UPRChapterContentRegistryDataAsset::GetHeadmindProofId();

	for (const FPrimaryAssetId& ChapterId : Persistence.CompletedChapterIds)
	{
		if (ChapterId != AllocatorChapterId && ChapterId != WardenChapterId && ChapterId != PacifierChapterId && ChapterId != AuditorChapterId && ChapterId != HeadmindChapterId)
		{
			return false;
		}
	}
	for (const FName ProofId : Persistence.HumanAnomalyProofIds)
	{
		if (ProofId != AllocatorProofId && ProofId != WardenProofId && ProofId != PacifierProofId && ProofId != AuditorProofId && ProofId != HeadmindProofId)
		{
			return false;
		}
	}

	const bool bAllocatorCompleted = Persistence.CompletedChapterIds.Contains(AllocatorChapterId);
	const bool bWardenCompleted = Persistence.CompletedChapterIds.Contains(WardenChapterId);
	const bool bPacifierCompleted = Persistence.CompletedChapterIds.Contains(PacifierChapterId);
	const bool bAuditorCompleted = Persistence.CompletedChapterIds.Contains(AuditorChapterId);
	const bool bHeadmindCompleted = Persistence.CompletedChapterIds.Contains(HeadmindChapterId);
	const bool bAllocatorProof = Persistence.HumanAnomalyProofIds.Contains(AllocatorProofId);
	const bool bWardenProof = Persistence.HumanAnomalyProofIds.Contains(WardenProofId);
	const bool bPacifierProof = Persistence.HumanAnomalyProofIds.Contains(PacifierProofId);
	const bool bAuditorProof = Persistence.HumanAnomalyProofIds.Contains(AuditorProofId);
	const bool bHeadmindProof = Persistence.HumanAnomalyProofIds.Contains(HeadmindProofId);
	if (bAllocatorCompleted != bAllocatorProof || bWardenCompleted != bWardenProof || bPacifierCompleted != bPacifierProof || bAuditorCompleted != bAuditorProof || bHeadmindCompleted != bHeadmindProof)
	{
		return false;
	}
	return (!bWardenProof || bAllocatorProof)
		&& (!bPacifierProof || (bAllocatorProof && bWardenProof))
		&& (!bAuditorProof || (bAllocatorProof && bWardenProof && bPacifierProof))
		&& (!bHeadmindProof || (bAllocatorProof && bWardenProof && bPacifierProof && bAuditorProof));
}

bool UPRChapterSubsystem::ConfigureActiveContent()
{
	FActiveChapterDefinition Selected;
	FName SelectionFailure;
	UPRRoomSubsystem* Room = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	if (!Room || !SelectActiveDefinition(Selected, SelectionFailure))
	{
		Snapshot.State = EPRChapterLifecycleState::Rejected;
		Snapshot.FallbackReason = SelectionFailure.IsNone() ? TEXT("Chapter.RegistryUnavailable") : SelectionFailure;
		PublishState();
		return false;
	}
	const bool bEnemyRegistryReady =
		Selected.FixedContent == FActiveChapterDefinition::EFixedContent::Warden
			? PRChapterSubsystemPrivate::EnsureWardenEnemyRegistryIsRegistered()
			: Selected.FixedContent == FActiveChapterDefinition::EFixedContent::Pacifier
				? PRChapterSubsystemPrivate::EnsurePacifierEnemyRegistryIsRegistered()
				: Selected.FixedContent == FActiveChapterDefinition::EFixedContent::Auditor
					? PRChapterSubsystemPrivate::EnsureAuditorEnemyRegistryIsRegistered()
					: Selected.FixedContent == FActiveChapterDefinition::EFixedContent::Headmind
						? PRChapterSubsystemPrivate::EnsureHeadmindEnemyRegistryIsRegistered()
						: true;
	if (!bEnemyRegistryReady)
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
	// The active World can already be bound when an account/profile transition
	// chooses its Chapter closure.  Configure the same closed Enemy registry in
	// that ordering too; otherwise the first Room would still attempt its
	// PrimaryAssetId spawns against the preceding chapter's whitelist.
	if (UWorld* ActiveWorld = BoundWorld.Get())
	{
		if (UPREnemySubsystem* Enemies = ActiveWorld->GetSubsystem<UPREnemySubsystem>())
		{
			const EPREnemyContentResult EnemyResult = Enemies->ConfigureContentRegistry(Selected.EnemyRegistryId);
			if (EnemyResult != EPREnemyContentResult::Succeeded
				&& Enemies->GetConfiguredContentRegistryId() != Selected.EnemyRegistryId)
			{
				Snapshot.State = EPRChapterLifecycleState::Rejected;
				Snapshot.FallbackReason = TEXT("Chapter.EnemyRegistryUnavailable");
				PublishState();
				return false;
			}
		}
	}
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
	RefreshPacifierStoryProjection();
	RefreshAuditorStoryProjection();
	RefreshHeadmindPresentation();
	EnsureWardenPresentation();
	EnsurePacifierPresentation();
	EnsureAuditorPresentation();
	EnsureHeadmindPresentation();
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
	if (!Completion.CompletionId.IsValid() || !Completion.BossSpawnId.IsValid()) return false;
	UPRRoomSubsystem* Room = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRoomSubsystem>() : nullptr;
	FGuid ExpectedSpawn;
	if (!Room || !Room->GetExpectedBossSpawnId(ExpectedSpawn) || ExpectedSpawn != Completion.BossSpawnId) return false;
	if (Completion.BossId == ActiveDefinition.BossId) return true;
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Headmind
		|| Completion.BossId != UPRChapterContentRegistryDataAsset::GetAuditorBossId()) return false;
	// The frozen Demo Auditor emits its original identity. Reinterpret it only
	// when the active closed Registry spawned the exact Headmind projection at
	// this expected spawn; every other chapter remains strict.
	APREnemyCharacter* SpawnedEnemy = nullptr;
	UPREnemySubsystem* Enemies = BoundWorld.IsValid() ? BoundWorld->GetSubsystem<UPREnemySubsystem>() : nullptr;
	return Enemies && Enemies->ResolveSpawnedEnemy(Completion.BossSpawnId, SpawnedEnemy)
		&& Cast<APRHeadmindProjectionBoss>(SpawnedEnemy) != nullptr;
}

bool UPRChapterSubsystem::BeginSettlement()
{
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	FPRChapterPersistenceData Current;
	if (!Save || !Save->GetChapterPersistenceSnapshot(Current)) return false;
	if (ActiveDefinition.FixedContent == FActiveChapterDefinition::EFixedContent::Pacifier
		&& (!Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAllocatorProofId())
			|| !Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetWardenProofId())))
	{
		Snapshot.State = EPRChapterLifecycleState::Rejected;
		Snapshot.FallbackReason = TEXT("Chapter.InvalidProofChain");
		PublishState();
		return false;
	}
	if (ActiveDefinition.FixedContent == FActiveChapterDefinition::EFixedContent::Auditor
		&& (!Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAllocatorProofId())
			|| !Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetWardenProofId())
			|| !Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetPacifierProofId())))
	{
		Snapshot.State = EPRChapterLifecycleState::Rejected;
		Snapshot.FallbackReason = TEXT("Chapter.InvalidProofChain");
		PublishState();
		return false;
	}
	if (ActiveDefinition.FixedContent == FActiveChapterDefinition::EFixedContent::Headmind
		&& (!Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAllocatorProofId())
			|| !Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetWardenProofId())
			|| !Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetPacifierProofId())
			|| !Current.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAuditorProofId())))
	{
		Snapshot.State = EPRChapterLifecycleState::Rejected;
		Snapshot.FallbackReason = TEXT("Chapter.InvalidProofChain");
		PublishState();
		return false;
	}
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
	Transaction.HeadmindEndingInput = FrozenHeadmindEndingInput;
	if (ActiveDefinition.FixedContent == FActiveChapterDefinition::EFixedContent::Headmind)
	{
		Transaction.Completion.HeadmindEnding = Snapshot.HeadmindEnding;
	}
	Transaction.bPending = true;
	PendingSettlement = MoveTemp(Transaction);
	return SubmitPendingSettlement();
}

void UPRChapterSubsystem::TryBeginSettlement()
{
	if (Snapshot.State != EPRChapterLifecycleState::RunActive
		|| !bRoomSequenceVerified
		|| !bBossVerified
		|| !bAccountDeletedVerified
		|| PendingSettlement.bPending
		|| bSettlementRequested)
	{
		return;
	}
	bSettlementRequested = true;
	if (BeginSettlement()) bSettlementRequested = false;
	else if (PendingSettlement.bPending && !PendingSettlement.SaveRequestId.IsValid()) ScheduleDeferredSettlementSubmit();
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

void UPRChapterSubsystem::ScheduleDeferredSettlementSubmit()
{
	if (SettlementRetryTickerHandle.IsValid()
		|| !bSettlementRequested
		|| !PendingSettlement.bPending
		|| PendingSettlement.SaveRequestId.IsValid())
	{
		return;
	}
	SettlementRetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPRChapterSubsystem::TickDeferredSettlementSubmit));
}

bool UPRChapterSubsystem::TickDeferredSettlementSubmit(const float DeltaSeconds)
{
	if (!bSettlementRequested
		|| !PendingSettlement.bPending
		|| PendingSettlement.SaveRequestId.IsValid())
	{
		SettlementRetryTickerHandle.Reset();
		return false;
	}
	if (SubmitPendingSettlement())
	{
		bSettlementRequested = false;
		SettlementRetryTickerHandle.Reset();
		return false;
	}
	return true;
}

void UPRChapterSubsystem::RefreshWardenStoryProjection()
{
	Snapshot.WardenStory = FPRWardenStoryProjection();
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Warden) return;
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

void UPRChapterSubsystem::RefreshPacifierStoryProjection()
{
	Snapshot.PacifierStory = FPRPacifierStoryProjection();
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Pacifier) return;
	const UPRPacifierChapterDataAsset* Pacifier = PRChapterSubsystemPrivate::LoadPacifierDefinition();
	UPRCompanionSubsystem* Companions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	UPRCompanionQuestSubsystem* Quests = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionQuestSubsystem>() : nullptr;
	UPRMemorySubsystem* Memory = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRMemorySubsystem>() : nullptr;
	FPRMemorySummary LatestSummary;
	FPRCompanionRelationshipRecord KindleRelationship;
	FPRCompanionQuestEntitlementSnapshot QuestEntitlements;
	const FGameplayTag KindleId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false);
	const bool bKindleRelationshipAvailable = Companions && Companions->GetRelationshipSnapshot(KindleId, KindleRelationship);
	const bool bQuestEntitlementsAvailable = Quests && Quests->GetEntitlementSnapshot(QuestEntitlements);
	const bool bDependencies = Pacifier && Companions && Quests && Memory && bKindleRelationshipAvailable
		&& bQuestEntitlementsAvailable && Memory->GetLatestSummary(LatestSummary);
	const bool bKindlePrimary = Companions && Companions->GetSyncState().PrimaryCompanionId == KindleId;
	const bool bNoRetreat = Quests && Quests->IsQuestCompleted(TEXT("Quest.Kindle.NoRetreatLine"));
	const bool bLearnToRetreat = Quests && Quests->IsQuestCompleted(TEXT("Quest.Kindle.LearnToRetreat"))
		&& QuestEntitlements.EntitlementIds.Contains(TEXT("Line:Kindle_LearnToRetreat"));
	if (Pacifier)
	{
		Snapshot.PacifierStory = Pacifier->BuildStoryProjection(
			bKindlePrimary,
			bNoRetreat,
			bLearnToRetreat,
			bDependencies);
	}
}

void UPRChapterSubsystem::RefreshAuditorStoryProjection()
{
	Snapshot.AuditorStory = FPRAuditorStoryProjection();
	Snapshot.TripleResonancePrerequisiteId = NAME_None;
	Snapshot.bHasTripleResonancePrerequisite = false;
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Auditor) return;
	const UPRAuditorChapterDataAsset* Auditor = PRChapterSubsystemPrivate::LoadAuditorDefinition();
	UPRCompanionSubsystem* Companions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	UPRCompanionQuestSubsystem* Quests = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionQuestSubsystem>() : nullptr;
	UPRMemorySubsystem* Memory = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRMemorySubsystem>() : nullptr;
	FPRMemorySnapshot MemorySnapshot;
	FPRCompanionRelationshipRecord NullRelationship;
	FPRCompanionQuestEntitlementSnapshot Entitlements;
	const FGameplayTag NullId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false);
	const bool bRelationship = Companions && Companions->GetRelationshipSnapshot(NullId, NullRelationship);
	const bool bEntitlements = Quests && Quests->GetEntitlementSnapshot(Entitlements);
	const bool bMemory = Memory && Memory->GetSnapshot(MemorySnapshot);
	const bool bDependencies = Auditor && Companions && Quests && Memory && bRelationship && bEntitlements && bMemory;
	const bool bNullPrimary = Companions && Companions->GetSyncState().PrimaryCompanionId == NullId;
	const bool bGarbageCollection = Quests && Quests->IsQuestCompleted(TEXT("Quest.Null.GarbageCollection"));
	const bool bRememberMe = Quests && Quests->IsQuestCompleted(TEXT("Quest.Null.RememberMe"))
		&& Entitlements.EntitlementIds.Contains(TEXT("Line:Null_RememberMe"));
	if (Auditor) Snapshot.AuditorStory = Auditor->BuildStoryProjection(bNullPrimary, bGarbageCollection, bRememberMe, bDependencies);
	FPRChapterPersistenceData Persistence;
	if (UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr)
	{
		if (Save->GetChapterPersistenceSnapshot(Persistence)
			&& Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAuditorProofId()))
		{
			Snapshot.TripleResonancePrerequisiteId = TEXT("TripleResonance.ChapterPrerequisite.Auditor");
			Snapshot.bHasTripleResonancePrerequisite = true;
		}
	}
}

void UPRChapterSubsystem::RefreshHeadmindPresentation()
{
	Snapshot.HeadmindBoss = FPRHeadmindBossRuntimeState();
	Snapshot.HeadmindEnding = FPRHeadmindEndingResult();
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Headmind) return;
	FPRChapterPersistenceData Persistence;
	if (UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr)
	{
		if (Save->GetChapterPersistenceSnapshot(Persistence)
			&& Persistence.HumanAnomalyProofIds.Contains(UPRChapterContentRegistryDataAsset::GetAuditorProofId()))
		{
			Snapshot.TripleResonancePrerequisiteId = TEXT("TripleResonance.ChapterPrerequisite.Auditor");
			Snapshot.bHasTripleResonancePrerequisite = true;
		}
	}
}

void UPRChapterSubsystem::PublishHeadmindBossRuntimeState(const FPRHeadmindBossRuntimeState& InRuntimeState)
{
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Headmind
		|| Snapshot.State != EPRChapterLifecycleState::RunActive) return;
	Snapshot.HeadmindBoss = InRuntimeState;
	PublishState();
}

void UPRChapterSubsystem::ClearWardenPresentation()
{
	if (UPRWardenChapterWidget* Overlay = WardenOverlay.Get()) Overlay->RemoveFromParent();
	WardenOverlay.Reset();
}

void UPRChapterSubsystem::ClearPacifierPresentation()
{
	if (UPRPacifierChapterWidget* Overlay = PacifierOverlay.Get()) Overlay->RemoveFromParent();
	PacifierOverlay.Reset();
}

void UPRChapterSubsystem::ClearAuditorPresentation()
{
	if (UPRAuditorChapterWidget* Overlay = AuditorOverlay.Get()) Overlay->RemoveFromParent();
	AuditorOverlay.Reset();
}

void UPRChapterSubsystem::ClearHeadmindPresentation()
{
	if (UPRHeadmindChapterWidget* Overlay = HeadmindOverlay.Get()) Overlay->RemoveFromParent();
	HeadmindOverlay.Reset();
}

void UPRChapterSubsystem::EnsureWardenPresentation()
{
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Warden || WardenOverlay.IsValid()) return;
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

void UPRChapterSubsystem::EnsurePacifierPresentation()
{
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Pacifier || PacifierOverlay.IsValid()) return;
	const UPRPacifierChapterDataAsset* Pacifier = PRChapterSubsystemPrivate::LoadPacifierDefinition();
	UClass* OverlayClass = Pacifier ? Pacifier->OverlayWidgetClass.LoadSynchronous() : nullptr;
	UWorld* World = BoundWorld.Get();
	APlayerController* Controller = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!OverlayClass || !Controller) return;
	if (UPRPacifierChapterWidget* Overlay = CreateWidget<UPRPacifierChapterWidget>(Controller, OverlayClass))
	{
		PacifierOverlay = Overlay;
		Overlay->AddToViewport(20);
	}
}

void UPRChapterSubsystem::EnsureAuditorPresentation()
{
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Auditor || AuditorOverlay.IsValid()) return;
	const UPRAuditorChapterDataAsset* Auditor = PRChapterSubsystemPrivate::LoadAuditorDefinition();
	UClass* OverlayClass = Auditor ? Auditor->OverlayWidgetClass.LoadSynchronous() : nullptr;
	UWorld* World = BoundWorld.Get();
	APlayerController* Controller = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!OverlayClass || !Controller) return;
	if (UPRAuditorChapterWidget* Overlay = CreateWidget<UPRAuditorChapterWidget>(Controller, OverlayClass))
	{
		AuditorOverlay = Overlay;
		Overlay->AddToViewport(20);
	}
}

void UPRChapterSubsystem::EnsureHeadmindPresentation()
{
	if (ActiveDefinition.FixedContent != FActiveChapterDefinition::EFixedContent::Headmind || HeadmindOverlay.IsValid()) return;
	const UPRHeadmindChapterDataAsset* Headmind = PRChapterSubsystemPrivate::LoadHeadmindDefinition();
	UClass* OverlayClass = Headmind ? Headmind->OverlayWidgetClass.LoadSynchronous() : nullptr;
	UWorld* World = BoundWorld.Get();
	APlayerController* Controller = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!OverlayClass || !Controller) return;
	if (UPRHeadmindChapterWidget* Overlay = CreateWidget<UPRHeadmindChapterWidget>(Controller, OverlayClass))
	{
		HeadmindOverlay = Overlay;
		Overlay->AddToViewport(20);
	}
}
