// Copyright Epic Games, Inc. All Rights Reserved.

#include "Companions/PRCompanionSubsystem.h"

#include "Companions/PRCompanionDataAsset.h"
#include "Companions/PRCompanionRegistryDataAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "ProjectR.h"
#include "Save/PRSaveSubsystem.h"

namespace PRCompanionRegistry
{
const FSoftObjectPath RegistryPath(TEXT("/Game/ProjectR/Companions/DA_CompanionRegistry.DA_CompanionRegistry"));
}

void UPRCompanionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UPRSaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRSaveSubsystem>() : nullptr;
	SaveSubsystem = Save;
	if (Save)
	{
		SaveOperationHandle = Save->OnSaveOperation().AddUObject(this, &UPRCompanionSubsystem::HandleSaveOperation);
		LoadRelationshipProjection();
	}
	RegistryAsset = TSoftObjectPtr<UPRCompanionRegistryDataAsset>(PRCompanionRegistry::RegistryPath);
	LoadFixedRegistry();
}

void UPRCompanionSubsystem::Deinitialize()
{
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get(); Save && SaveOperationHandle.IsValid())
	{
		Save->OnSaveOperation().Remove(SaveOperationHandle);
	}
	SaveOperationHandle.Reset();
	SaveSubsystem.Reset();
	LoadedCompanions.Empty();
	RelationshipRecords.Empty();
	SyncState = FPRCompanionSyncState();
	bRegistryReady = false;
	RelationshipChanged.Clear();
	PrimarySyncChanged.Clear();
	Super::Deinitialize();
}

EPRPrimaryCompanionSelectionResult UPRCompanionSubsystem::SelectPrimaryCompanion(const FGameplayTag CompanionId)
{
	if (!CompanionId.IsValid()) return EPRPrimaryCompanionSelectionResult::Invalid;
	if (!bRegistryReady) return EPRPrimaryCompanionSelectionResult::RejectedRegistryUnavailable;
	if (!IsKnownCompanion(CompanionId)) return EPRPrimaryCompanionSelectionResult::RejectedUnknownCompanion;
	if (SyncState.PrimaryCompanionId.MatchesTagExact(CompanionId)) return EPRPrimaryCompanionSelectionResult::AlreadySelected;

	FPRPrimaryCompanionSyncChangedEvent Event;
	Event.PreviousState = SyncState;
	SyncState.PrimaryCompanionId = CompanionId;
	SyncState.BackgroundCompanionIds.Reset();
	for (const FGameplayTag& Candidate : FPRCompanionContract::GetCanonicalCompanionIds())
	{
		if (!Candidate.MatchesTagExact(CompanionId)) SyncState.BackgroundCompanionIds.Add(Candidate);
	}
	++SyncState.SelectionRevision;
	Event.CurrentState = SyncState;
	Event.WorldTimeSeconds = GetWorldTimeSeconds();
	PrimarySyncChanged.Broadcast(Event);
	return EPRPrimaryCompanionSelectionResult::Selected;
}

FPRCompanionSyncState UPRCompanionSubsystem::GetSyncState() const
{
	return SyncState;
}

bool UPRCompanionSubsystem::GetRelationshipSnapshot(const FGameplayTag CompanionId, FPRCompanionRelationshipRecord& OutRecord) const
{
	OutRecord = FPRCompanionRelationshipRecord();
	const FPRCompanionRelationshipRecord* Record = RelationshipRecords.FindByPredicate(
		[CompanionId](const FPRCompanionRelationshipRecord& Candidate)
		{
			return Candidate.CompanionId.MatchesTagExact(CompanionId);
		});
	if (!Record) return false;
	OutRecord = *Record;
	return true;
}

void UPRCompanionSubsystem::GetAllRelationshipSnapshots(TArray<FPRCompanionRelationshipRecord>& OutRecords) const
{
	OutRecords = RelationshipRecords;
}

EPRRelationshipApplyResult UPRCompanionSubsystem::ApplyRelationshipDelta(const FPRRelationshipDelta& Delta)
{
	if (!Delta.CompanionId.IsValid() || Delta.SourceId.IsNone()) return EPRRelationshipApplyResult::Invalid;
	if (!bRegistryReady || !IsKnownCompanion(Delta.CompanionId)) return EPRRelationshipApplyResult::RejectedUnknownCompanion;
	UPRSaveSubsystem* Save = SaveSubsystem.Get();
	if (!Save || !Save->GetSaveRuntimeState().bHasLoadedProfile) return EPRRelationshipApplyResult::RejectedNoLoadedProfile;

	FPRCompanionRelationshipRecord* Record = RelationshipRecords.FindByPredicate(
		[&Delta](const FPRCompanionRelationshipRecord& Candidate)
		{
			return Candidate.CompanionId.MatchesTagExact(Delta.CompanionId);
		});
	if (!Record) return EPRRelationshipApplyResult::Invalid;
	const FPRRelationshipState Previous = Record->State;
	const auto WouldClamp = [](const int32 Value, const int32 Delta)
	{
		const int64 Candidate = static_cast<int64>(Value) + static_cast<int64>(Delta);
		return Candidate < FPRCompanionContract::MinimumRelationshipValue || Candidate > FPRCompanionContract::MaximumRelationshipValue;
	};
	const bool bClamped = WouldClamp(Previous.Trust, Delta.TrustDelta)
		|| WouldClamp(Previous.Affection, Delta.AffectionDelta)
		|| WouldClamp(Previous.Evaluation, Delta.EvaluationDelta)
		|| WouldClamp(Previous.Overload, Delta.OverloadDelta);
	if (!FPRCompanionContract::ApplyDelta(Record->State, Delta)) return EPRRelationshipApplyResult::Invalid;
	if (!Save->StageCompanionRelationships(RelationshipRecords))
	{
		Record->State = Previous;
		return EPRRelationshipApplyResult::RejectedNoLoadedProfile;
	}
	if (Previous.Trust == Record->State.Trust && Previous.Affection == Record->State.Affection
		&& Previous.Evaluation == Record->State.Evaluation && Previous.Overload == Record->State.Overload)
	{
		return EPRRelationshipApplyResult::Applied;
	}
	FPRRelationshipChangedEvent Event;
	Event.CompanionId = Delta.CompanionId;
	Event.PreviousState = Previous;
	Event.CurrentState = Record->State;
	Event.AppliedDelta = Delta;
	Event.WorldTimeSeconds = GetWorldTimeSeconds();
	RelationshipChanged.Broadcast(Event);
	return bClamped ? EPRRelationshipApplyResult::AppliedClamped : EPRRelationshipApplyResult::Applied;
}

FPRRelationshipChangedNative& UPRCompanionSubsystem::OnRelationshipChanged()
{
	return RelationshipChanged;
}

FPRPrimaryCompanionSyncChangedNative& UPRCompanionSubsystem::OnPrimarySyncChanged()
{
	return PrimarySyncChanged;
}

bool UPRCompanionSubsystem::IsRegistryReady() const
{
	return bRegistryReady;
}

void UPRCompanionSubsystem::LoadFixedRegistry()
{
	if (!RegistryAsset.IsNull())
	{
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			RegistryAsset.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UPRCompanionSubsystem::FinishRegistryLoad));
	}
}

void UPRCompanionSubsystem::FinishRegistryLoad()
{
	UPRCompanionRegistryDataAsset* Registry = RegistryAsset.Get();
	if (!Registry) return;
	const TArray<TSoftObjectPtr<UPRCompanionDataAsset>>& Entries = Registry->GetCompanions();
	const TArray<FGameplayTag>& CanonicalIds = FPRCompanionContract::GetCanonicalCompanionIds();
	if (Entries.Num() != CanonicalIds.Num()) return;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		UPRCompanionDataAsset* Asset = Entries[Index].LoadSynchronous();
		if (!Asset || !Asset->CompanionId.MatchesTagExact(CanonicalIds[Index]))
		{
			LoadedCompanions.Empty();
			return;
		}
		LoadedCompanions.Add(Asset->CompanionId, Asset);
	}
	bRegistryReady = LoadedCompanions.Num() == CanonicalIds.Num();
}

void UPRCompanionSubsystem::HandleSaveOperation(const FPRSaveOperationEvent& Event)
{
	if ((Event.Operation == EPRSaveOperationType::Create || Event.Operation == EPRSaveOperationType::Load)
		&& (Event.Result == EPRSaveResult::Success || Event.Result == EPRSaveResult::RecoveredFromAlternate))
	{
		LoadRelationshipProjection();
		ClearSyncState();
	}
}

void UPRCompanionSubsystem::LoadRelationshipProjection()
{
	FPRProfileSaveData Profile;
	if (UPRSaveSubsystem* Save = SaveSubsystem.Get(); Save && Save->GetLoadedProfileSnapshot(Profile)
		&& FPRCompanionContract::AreCanonicalRelationshipRecords(Profile.CompanionRelationships))
	{
		RelationshipRecords = Profile.CompanionRelationships;
	}
	else
	{
		RelationshipRecords.Empty();
	}
}

void UPRCompanionSubsystem::ClearSyncState()
{
	if (!SyncState.PrimaryCompanionId.IsValid()) return;
	FPRPrimaryCompanionSyncChangedEvent Event;
	Event.PreviousState = SyncState;
	SyncState = FPRCompanionSyncState();
	Event.CurrentState = SyncState;
	Event.WorldTimeSeconds = GetWorldTimeSeconds();
	PrimarySyncChanged.Broadcast(Event);
}

bool UPRCompanionSubsystem::IsKnownCompanion(const FGameplayTag CompanionId) const
{
	return LoadedCompanions.Contains(CompanionId);
}

double UPRCompanionSubsystem::GetWorldTimeSeconds() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}
