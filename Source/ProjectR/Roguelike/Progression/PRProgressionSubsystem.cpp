// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/Progression/PRProgressionSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Roguelike/Account/PRAccountRuntimeTypes.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/Progression/PRProgressionNodeDataAsset.h"
#include "Roguelike/Progression/PRProgressionRegistryDataAsset.h"
#include "Save/PRSaveSubsystem.h"

#include "UObject/UObjectGlobals.h"

namespace PRProgressionSubsystemPrivate
{
const FPrimaryAssetType NodeType(TEXT("ProgressionNode"));
const FName SupportSourceId(TEXT("Progression.AISupport"));
const FSoftObjectPath RegistryPath(TEXT("/Game/ProjectR/Data/Progression/DA_ProgressionRegistry.DA_ProgressionRegistry"));
const TCHAR* const HealthEffectPath = TEXT("/Game/ProjectR/Effects/Progression/GE_Progression_PlayerMaxHealth.GE_Progression_PlayerMaxHealth_C");
const TCHAR* const EnergyEffectPath = TEXT("/Game/ProjectR/Effects/Progression/GE_Progression_PlayerMaxEnergy.GE_Progression_PlayerMaxEnergy_C");

bool IsSuccessfulSaveResult(const EPRSaveResult Result)
{
	return Result == EPRSaveResult::Success || Result == EPRSaveResult::RecoveredFromAlternate;
}

bool IsActiveRunState(const EPRRunLifecycleState State)
{
	return State == EPRRunLifecycleState::RunActive || State == EPRRunLifecycleState::AwaitingDivergence;
}
}

void UPRProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveSubsystem = GetGameInstance()->GetSubsystem<UPRSaveSubsystem>();
	RunStateSubsystem = GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>();
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get())
	{
		SaveOperationHandle = Save->OnSaveOperation().AddUObject(this, &UPRProgressionSubsystem::HandleSaveOperation);
	}
	if (UPRRunStateSubsystem* RunState = RunStateSubsystem.Get())
	{
		AccountOperationHandle = RunState->OnAccountOperation().AddUObject(this, &UPRProgressionSubsystem::HandleAccountOperation);
		RunStateHandle = RunState->OnRunStateChanged().AddUObject(this, &UPRProgressionSubsystem::HandleRunStateChanged);
	}
	PostWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UPRProgressionSubsystem::HandlePostWorldInitialization);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UPRProgressionSubsystem::HandleWorldCleanup);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPRProgressionSubsystem::HandlePostLoadMap);
	RefreshSnapshotFromProfile();
	if (GetWorld())
	{
		BoundWorld = GetWorld();
		RebindRuntimeEffects();
	}
}

void UPRProgressionSubsystem::Deinitialize()
{
	ClearRuntimeEffects();
	ClearRunSnapshot();
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get()) Save->OnSaveOperation().Remove(SaveOperationHandle);
	if (UPRRunStateSubsystem* RunState = RunStateSubsystem.Get())
	{
		RunState->OnAccountOperation().Remove(AccountOperationHandle);
		RunState->OnRunStateChanged().Remove(RunStateHandle);
	}
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitializationHandle);
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	PendingUnlock.Reset();
	SaveSubsystem.Reset();
	RunStateSubsystem.Reset();
	BoundWorld.Reset();
	ProgressionChanged.Clear();
	UnlockCompleted.Clear();
	RunSnapshotChanged.Clear();
	Super::Deinitialize();
}

EPRProgressionOperationResult UPRProgressionSubsystem::RequestUnlockNode(const FPrimaryAssetId NodeId, FGuid& OutRequestId)
{
	OutRequestId.Invalidate();
	if (PendingUnlock.IsSet()) return EPRProgressionOperationResult::Pending;
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	UPRRunStateSubsystem* RunState = RunStateSubsystem.Get();
	if (!Save || !RunState || !bHasLoadedProfile || !LoadRegistry()) return EPRProgressionOperationResult::NotReady;
	const EPRRunLifecycleState RunStateValue = RunState->GetRunRuntimeState().State;
	if (RunStateValue != EPRRunLifecycleState::Idle && RunStateValue != EPRRunLifecycleState::AccountReady)
	{
		return EPRProgressionOperationResult::InvalidState;
	}

	FPRProfileSaveData Profile;
	if (!Save->GetLoadedProfileSnapshot(Profile)) return EPRProgressionOperationResult::NotReady;
	const UPRProgressionNodeDataAsset* Node = LoadRegistry()->FindNode(NodeId);
	if (!Node) return EPRProgressionOperationResult::UnknownNode;
	const EPRProgressionOperationResult Validation = ValidateUnlock(*Node, Profile);
	if (Validation != EPRProgressionOperationResult::Success) return Validation;
	if (Profile.ProgressionPersistence.UnlockSequence == MAX_int64) return EPRProgressionOperationResult::InvalidState;

	FPendingUnlock Pending;
	Pending.RequestId = FGuid::NewGuid();
	Pending.NodeId = NodeId;
	Pending.AccountPersistence = Profile.AccountPersistence;
	Pending.ProgressionPersistence = Profile.ProgressionPersistence;
	Pending.AccountPersistence.CounterproofFragments -= Node->CounterproofCost;
	Pending.ProgressionPersistence.MemoryFragments -= Node->MemoryFragmentCost;
	Pending.ProgressionPersistence.UnlockedNodeIds.Add(NodeId);
	++Pending.ProgressionPersistence.UnlockSequence;
	FPRAccountPersistenceContract::Normalize(Pending.AccountPersistence);
	FPRProgressionPersistenceContract::Normalize(Pending.ProgressionPersistence);
	if (!Save->StageProgressionTransaction(Pending.AccountPersistence, Pending.ProgressionPersistence))
	{
		return EPRProgressionOperationResult::PersistenceFailed;
	}
	const EPRSaveRequestStatus SaveStatus = Save->RequestSaveCurrentProfile(Pending.SaveRequestId);
	if (SaveStatus != EPRSaveRequestStatus::Started)
	{
		return EPRProgressionOperationResult::PersistenceFailed;
	}
	OutRequestId = Pending.RequestId;
	PendingUnlock = MoveTemp(Pending);
	return EPRProgressionOperationResult::Pending;
}

EPRProgressionOperationResult UPRProgressionSubsystem::RetryPendingUnlock(FGuid& OutRequestId)
{
	OutRequestId.Invalidate();
	if (!PendingUnlock.IsSet()) return EPRProgressionOperationResult::RetryNotAvailable;
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	if (!Save) return EPRProgressionOperationResult::NotReady;
	FPendingUnlock& Pending = PendingUnlock.GetValue();
	if (!Save->StageProgressionTransaction(Pending.AccountPersistence, Pending.ProgressionPersistence))
	{
		return EPRProgressionOperationResult::PersistenceFailed;
	}
	if (Save->RequestSaveCurrentProfile(Pending.SaveRequestId) != EPRSaveRequestStatus::Started)
	{
		return EPRProgressionOperationResult::PersistenceFailed;
	}
	OutRequestId = Pending.RequestId;
	return EPRProgressionOperationResult::Pending;
}

bool UPRProgressionSubsystem::GetProgressionSnapshot(FPRProgressionSnapshot& OutSnapshot) const
{
	OutSnapshot = bHasLoadedProfile ? Snapshot : FPRProgressionSnapshot();
	return bHasLoadedProfile;
}

bool UPRProgressionSubsystem::GetRunSnapshot(FPRProgressionRunSnapshot& OutSnapshot) const
{
	OutSnapshot = bHasRunSnapshot ? RunSnapshot : FPRProgressionRunSnapshot();
	return bHasRunSnapshot;
}

bool UPRProgressionSubsystem::IsNodeUnlocked(const FPrimaryAssetId NodeId) const
{
	return Snapshot.UnlockedNodeIds.Contains(NodeId);
}

FPRProgressionChangedNative& UPRProgressionSubsystem::OnProgressionChanged() { return ProgressionChanged; }
FPRProgressionUnlockCompletedNative& UPRProgressionSubsystem::OnUnlockCompleted() { return UnlockCompleted; }
FPRProgressionRunSnapshotChangedNative& UPRProgressionSubsystem::OnRunSnapshotChanged() { return RunSnapshotChanged; }

void UPRProgressionSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if (Event.Operation == EPRSaveOperationType::Load || Event.Operation == EPRSaveOperationType::Create)
	{
		if (PRProgressionSubsystemPrivate::IsSuccessfulSaveResult(Event.Result))
		{
			ClearRuntimeEffects();
			ClearRunSnapshot();
			PendingUnlock.Reset();
			RefreshSnapshotFromProfile();
			PublishChanged(FGuid());
		}
		return;
	}
	if (Event.Operation != EPRSaveOperationType::Save) return;
	if (PendingUnlock.IsSet() && Event.RequestId == PendingUnlock->SaveRequestId)
	{
		if (Event.Result == EPRSaveResult::Success)
		{
			const FGuid CompletedId = PendingUnlock->RequestId;
			RefreshSnapshotFromProfile();
			PublishUnlockCompleted(EPRProgressionOperationResult::Success);
			PendingUnlock.Reset();
			PublishChanged(CompletedId);
		}
		else
		{
			PublishUnlockCompleted(EPRProgressionOperationResult::PersistenceFailed);
		}
		return;
	}
	if (Event.Result == EPRSaveResult::Success && RefreshSnapshotFromProfile()) PublishChanged(FGuid());
}

void UPRProgressionSubsystem::HandleAccountOperation(const FPRAccountOperationEvent& Event)
{
	if (Event.Operation == EPRAccountOperationType::StartRun && Event.Result == EPRAccountOperationResult::Succeeded)
	{
		FreezeRunSnapshot();
		RebindRuntimeEffects();
	}
}

void UPRProgressionSubsystem::HandleRunStateChanged(const FPRRunRuntimeState& State)
{
	if (!PRProgressionSubsystemPrivate::IsActiveRunState(State.State))
	{
		ClearRuntimeEffects();
		ClearRunSnapshot();
	}
}

void UPRProgressionSubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues)
{
	if (World)
	{
		BoundWorld = World;
		RebindRuntimeEffects();
	}
}

void UPRProgressionSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (BoundWorld.Get() == World)
	{
		ClearRuntimeEffects();
		BoundWorld.Reset();
	}
}

void UPRProgressionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld)
	{
		BoundWorld = LoadedWorld;
		RebindRuntimeEffects();
	}
}

bool UPRProgressionSubsystem::RefreshSnapshotFromProfile()
{
	FPRProfileSaveData Profile;
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	if (!Save || !Save->GetLoadedProfileSnapshot(Profile)
		|| !FPRProgressionPersistenceContract::IsCanonical(Profile.ProgressionPersistence))
	{
		bHasLoadedProfile = false;
		Snapshot = FPRProgressionSnapshot();
		return false;
	}
	const UPRProgressionRegistryDataAsset* Registry = LoadRegistry();
	if (!Registry)
	{
		bHasLoadedProfile = false;
		Snapshot = FPRProgressionSnapshot();
		return false;
	}
	for (const FPrimaryAssetId& NodeId : Profile.ProgressionPersistence.UnlockedNodeIds)
	{
		if (!Registry->FindNode(NodeId))
		{
			bHasLoadedProfile = false;
			Snapshot = FPRProgressionSnapshot();
			return false;
		}
	}
	Snapshot.CounterproofFragments = Profile.AccountPersistence.CounterproofFragments;
	Snapshot.MemoryFragments = Profile.ProgressionPersistence.MemoryFragments;
	Snapshot.UnlockedNodeIds = Profile.ProgressionPersistence.UnlockedNodeIds;
	Snapshot.UnlockSequence = Profile.ProgressionPersistence.UnlockSequence;
	bHasLoadedProfile = true;
	return true;
}

const UPRProgressionRegistryDataAsset* UPRProgressionSubsystem::LoadRegistry() const
{
	UPRProgressionRegistryDataAsset* Registry = Cast<UPRProgressionRegistryDataAsset>(PRProgressionSubsystemPrivate::RegistryPath.TryLoad());
	return Registry && Registry->IsRegistryReady() ? Registry : nullptr;
}

EPRProgressionOperationResult UPRProgressionSubsystem::ValidateUnlock(
	const UPRProgressionNodeDataAsset& Node,
	const FPRProfileSaveData& Profile) const
{
	const FPrimaryAssetId NodeId = Node.GetPrimaryAssetId();
	if (Profile.ProgressionPersistence.UnlockedNodeIds.Contains(NodeId)) return EPRProgressionOperationResult::AlreadyUnlocked;
	for (const FPrimaryAssetId& Prerequisite : Node.PrerequisiteNodeIds)
	{
		if (!Profile.ProgressionPersistence.UnlockedNodeIds.Contains(Prerequisite)) return EPRProgressionOperationResult::PrerequisiteNotMet;
	}
	if (!DoesRelationshipRequirementPass(Node.RelationshipRequirement)) return EPRProgressionOperationResult::RelationshipRequirementNotMet;
	if (Profile.AccountPersistence.CounterproofFragments < Node.CounterproofCost) return EPRProgressionOperationResult::InsufficientCounterproofFragments;
	if (Profile.ProgressionPersistence.MemoryFragments < Node.MemoryFragmentCost) return EPRProgressionOperationResult::InsufficientMemoryFragments;
	return EPRProgressionOperationResult::Success;
}

bool UPRProgressionSubsystem::DoesRelationshipRequirementPass(const FPRProgressionRelationshipRequirement& Requirement) const
{
	if (Requirement.Metric == EPRProgressionRelationshipMetric::None) return true;
	UPRCompanionSubsystem* Companions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	if (!Companions) return false;
	TArray<FPRCompanionRelationshipRecord> Records;
	Companions->GetAllRelationshipSnapshots(Records);
	auto MeetsRequirement = [&Requirement](const FPRCompanionRelationshipRecord& Record)
	{
		const int32 Value = Requirement.Metric == EPRProgressionRelationshipMetric::Trust ? Record.State.Trust
			: Requirement.Metric == EPRProgressionRelationshipMetric::Affection ? Record.State.Affection
			: Record.State.Evaluation;
		return Value >= Requirement.MinimumValue;
	};
	if (Requirement.Scope == EPRProgressionRelationshipScope::AnyCompanion)
	{
		return Records.ContainsByPredicate(MeetsRequirement);
	}
	if (Requirement.Scope == EPRProgressionRelationshipScope::AllCompanions)
	{
		return Records.Num() == FPRCompanionContract::GetCanonicalCompanionIds().Num()
			&& Records.FilterByPredicate([&MeetsRequirement](const FPRCompanionRelationshipRecord& Record) { return MeetsRequirement(Record); }).Num() == Records.Num();
	}
	if (Requirement.Scope == EPRProgressionRelationshipScope::PrimaryCompanion)
	{
		const FGameplayTag PrimaryId = Companions->GetSyncState().PrimaryCompanionId;
		const FPRCompanionRelationshipRecord* Primary = Records.FindByPredicate([&PrimaryId](const FPRCompanionRelationshipRecord& Record) { return Record.CompanionId == PrimaryId; });
		return Primary && MeetsRequirement(*Primary);
	}
	return false;
}

void UPRProgressionSubsystem::FreezeRunSnapshot()
{
	if (!bHasLoadedProfile || !LoadRegistry()) return;
	RunSnapshot = FPRProgressionRunSnapshot();
	RunSnapshot.SnapshotSequence = Snapshot.UnlockSequence;
	for (const FPrimaryAssetId& NodeId : Snapshot.UnlockedNodeIds)
	{
		const UPRProgressionNodeDataAsset* Node = LoadRegistry()->FindNode(NodeId);
		if (!Node) continue;
		switch (Node->EffectKind)
		{
		case EPRProgressionEffectKind::PlayerMaxHealth: RunSnapshot.PlayerMaxHealthBonus += 10; break;
		case EPRProgressionEffectKind::PlayerMaxEnergy: RunSnapshot.PlayerMaxEnergyBonus += 10; break;
		case EPRProgressionEffectKind::CompanionSupportInterval: RunSnapshot.CompanionSupportIntervalMultiplier *= 0.90f; break;
		case EPRProgressionEffectKind::EntitlementOnly: RunSnapshot.EntitlementIds.Add(NodeId); break;
		default: break;
		}
	}
	RunSnapshot.EntitlementIds.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.ToString() < B.ToString(); });
	RunSnapshot.CompanionSupportIntervalMultiplier = FMath::Clamp(RunSnapshot.CompanionSupportIntervalMultiplier, 0.90f, 1.0f);
	bHasRunSnapshot = true;
	PublishRunSnapshotChanged();
}

void UPRProgressionSubsystem::ClearRunSnapshot()
{
	if (!bHasRunSnapshot) return;
	bHasRunSnapshot = false;
	RunSnapshot = FPRProgressionRunSnapshot();
	PublishRunSnapshotChanged();
}

UAbilitySystemComponent* UPRProgressionSubsystem::ResolvePlayerAbilitySystem(UWorld* World) const
{
	APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	IAbilitySystemInterface* Interface = Pawn ? Cast<IAbilitySystemInterface>(Pawn) : nullptr;
	return Interface ? Interface->GetAbilitySystemComponent() : nullptr;
}

void UPRProgressionSubsystem::RebindRuntimeEffects()
{
	if (!bHasRunSnapshot) return;
	UWorld* World = BoundWorld.Get();
	UAbilitySystemComponent* ASC = ResolvePlayerAbilitySystem(World);
	if (AppliedAbilitySystem.Get() != ASC)
	{
		ClearRuntimeEffects();
	}
	if (ASC)
	{
		AppliedAbilitySystem = ASC;
		if (RunSnapshot.PlayerMaxHealthBonus > 0 && !HealthEffectHandle.IsValid())
		{
			if (UClass* EffectClass = LoadClass<UGameplayEffect>(nullptr, PRProgressionSubsystemPrivate::HealthEffectPath))
			{
				HealthEffectHandle = ASC->ApplyGameplayEffectToSelf(EffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
			}
		}
		if (RunSnapshot.PlayerMaxEnergyBonus > 0 && !EnergyEffectHandle.IsValid())
		{
			if (UClass* EffectClass = LoadClass<UGameplayEffect>(nullptr, PRProgressionSubsystemPrivate::EnergyEffectPath))
			{
				EnergyEffectHandle = ASC->ApplyGameplayEffectToSelf(EffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
			}
		}
	}
	if (UPRCompanionRuntimeSubsystem* Runtime = World ? World->GetSubsystem<UPRCompanionRuntimeSubsystem>() : nullptr)
	{
		if (RunSnapshot.CompanionSupportIntervalMultiplier < 1.0f)
		{
			Runtime->SetSupportPolicy(PRProgressionSubsystemPrivate::SupportSourceId, RunSnapshot.CompanionSupportIntervalMultiplier, 1);
		}
	}
}

void UPRProgressionSubsystem::ClearRuntimeEffects()
{
	if (UAbilitySystemComponent* ASC = AppliedAbilitySystem.Get())
	{
		if (HealthEffectHandle.IsValid()) ASC->RemoveActiveGameplayEffect(HealthEffectHandle);
		if (EnergyEffectHandle.IsValid()) ASC->RemoveActiveGameplayEffect(EnergyEffectHandle);
	}
	HealthEffectHandle.Invalidate();
	EnergyEffectHandle.Invalidate();
	AppliedAbilitySystem.Reset();
	if (UWorld* World = BoundWorld.Get())
	{
		if (UPRCompanionRuntimeSubsystem* Runtime = World->GetSubsystem<UPRCompanionRuntimeSubsystem>())
		{
			Runtime->ClearSupportPolicy(PRProgressionSubsystemPrivate::SupportSourceId);
		}
	}
}

void UPRProgressionSubsystem::PublishChanged(const FGuid& RequestId)
{
	FPRProgressionChangedEvent Event;
	Event.RequestId = RequestId;
	Event.Snapshot = Snapshot;
	ProgressionChanged.Broadcast(Event);
}

void UPRProgressionSubsystem::PublishUnlockCompleted(const EPRProgressionOperationResult Result)
{
	if (!PendingUnlock.IsSet()) return;
	FPRProgressionUnlockCompletedEvent Event;
	Event.RequestId = PendingUnlock->RequestId;
	Event.SaveRequestId = PendingUnlock->SaveRequestId;
	Event.NodeId = PendingUnlock->NodeId;
	Event.Result = Result;
	UnlockCompleted.Broadcast(Event);
}

void UPRProgressionSubsystem::PublishRunSnapshotChanged()
{
	FPRProgressionRunSnapshotChangedEvent Event;
	Event.bHasActiveRunSnapshot = bHasRunSnapshot;
	Event.Snapshot = RunSnapshot;
	RunSnapshotChanged.Broadcast(Event);
}
