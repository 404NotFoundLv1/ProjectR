// Copyright Epic Games, Inc. All Rights Reserved.

#include "Save/PRSaveSubsystem.h"

#include "Save/PRSaveGame.h"
#include "Save/PRSaveStorage.h"

#include "ProjectR.h"
#include "UObject/UObjectGlobals.h"

namespace PRSaveSubsystemPrivate
{
bool IsSuccessfulRead(const FPRSaveGenerationRead& Read)
{
	return Read.Result == EPRSaveResult::Success && IsValid(Read.SaveGame);
}

int32 GetFailurePriority(const EPRSaveResult Result)
{
	switch (Result)
	{
	case EPRSaveResult::FutureEnvelopeVersion: return 100;
	case EPRSaveResult::FutureSchemaVersion: return 90;
	case EPRSaveResult::UnsupportedOldVersion: return 80;
	case EPRSaveResult::MigrationFailed: return 70;
	case EPRSaveResult::MissingSchemaVersion: return 60;
	case EPRSaveResult::WrongSaveClass: return 50;
	case EPRSaveResult::CorruptData: return 40;
	case EPRSaveResult::InvalidEnvelope: return 30;
	case EPRSaveResult::EmptyData: return 20;
	case EPRSaveResult::ReadFailed: return 10;
	case EPRSaveResult::NotFound: return 0;
	default: return -1;
	}
}

FString ResultName(const EPRSaveResult Result)
{
	if (const UEnum* Enum = StaticEnum<EPRSaveResult>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(Result));
	}
	return TEXT("InvalidEnum");
}
} // namespace PRSaveSubsystemPrivate

void UPRSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisterProjectRSaveMigrations(MigrationRegistry);
	Storage = FPRSaveStorage::CreateProduction();
	State = EPRSaveSubsystemState::Ready;
	LastResult = EPRSaveResult::InvalidRequest;
	UE_LOG(LogProjectR, Log, TEXT("ProjectR Save subsystem initialized without storage access."));
}

void UPRSaveSubsystem::Deinitialize()
{
	check(IsInGameThread());
	State = EPRSaveSubsystemState::ShuttingDown;

	if (TrailingSave)
	{
		PublishOperation(
			TrailingSave->RequestId,
			EPRSaveOperationType::Save,
			EPRSaveResult::CancelledOnShutdown,
			LoadedSave ? LoadedSave->SchemaVersion : 0,
			EPRSaveGeneration::None,
			false,
			bNeedsResave);
		TrailingSave.Reset();
	}

	ActiveSave.Reset();
	Storage.Reset();
	LoadedSave = nullptr;
	SaveOperationEvent.Clear();
	Super::Deinitialize();
}

EPRSaveResult UPRSaveSubsystem::LoadDefaultProfile(FGuid& OutRequestId)
{
	if (!IsCallableOnGameThread(OutRequestId))
	{
		return EPRSaveResult::InvalidRequest;
	}
	if (State == EPRSaveSubsystemState::Loading || State == EPRSaveSubsystemState::Saving)
	{
		return EPRSaveResult::Busy;
	}
	if (State != EPRSaveSubsystemState::Ready || !Storage)
	{
		return EPRSaveResult::InvalidRequest;
	}

	OutRequestId = FGuid::NewGuid();
	if (LoadedSave)
	{
		LastResult = EPRSaveResult::AlreadyLoaded;
		PublishOperation(
			OutRequestId,
			EPRSaveOperationType::Load,
			LastResult,
			LoadedSave->SchemaVersion,
			LoadedGeneration,
			false,
			bNeedsResave);
		return LastResult;
	}

	State = EPRSaveSubsystemState::Loading;
	const FPRSaveGenerationRead ReadA = Storage->ReadGenerationSync(EPRSaveGeneration::A, &MigrationRegistry);
	const FPRSaveGenerationRead ReadB = Storage->ReadGenerationSync(EPRSaveGeneration::B, &MigrationRegistry);
	ObservedA = ObserveGeneration(ReadA);
	ObservedB = ObserveGeneration(ReadB);

	UE_LOG(
		LogProjectR,
		Log,
		TEXT("ProjectR Save load diagnostics: A=%s B=%s."),
		*PRSaveSubsystemPrivate::ResultName(ReadA.Result),
		*PRSaveSubsystemPrivate::ResultName(ReadB.Result));

	const FLoadSelection Selection = SelectLoadedGeneration(ReadA, ReadB);
	if (Selection.SaveGame)
	{
		LoadedSave = DuplicateObject<UPRSaveGame>(Selection.SaveGame, this);
		LoadedGeneration = Selection.Generation;
		bNeedsResave = Selection.bNeedsResave;
	}

	State = EPRSaveSubsystemState::Ready;
	LastResult = Selection.Result;
	PublishOperation(
		OutRequestId,
		EPRSaveOperationType::Load,
		Selection.Result,
		Selection.SourceSchemaVersion,
		Selection.Generation,
		Selection.bUsedAlternateGeneration,
		Selection.bNeedsResave);
	return Selection.Result;
}

EPRSaveResult UPRSaveSubsystem::CreateNewDefaultProfile(FGuid& OutRequestId)
{
	if (!IsCallableOnGameThread(OutRequestId))
	{
		return EPRSaveResult::InvalidRequest;
	}
	if (State == EPRSaveSubsystemState::Loading || State == EPRSaveSubsystemState::Saving)
	{
		return EPRSaveResult::Busy;
	}
	if (State != EPRSaveSubsystemState::Ready || !Storage)
	{
		return EPRSaveResult::InvalidRequest;
	}

	OutRequestId = FGuid::NewGuid();
	if (LoadedSave)
	{
		LastResult = EPRSaveResult::AlreadyLoaded;
		PublishOperation(
			OutRequestId,
			EPRSaveOperationType::Create,
			LastResult,
			LoadedSave->SchemaVersion,
			LoadedGeneration,
			false,
			bNeedsResave);
		return LastResult;
	}

	const ISaveGameSystem::ESaveExistsResult ExistsA = Storage->CheckExists(EPRSaveGeneration::A);
	const ISaveGameSystem::ESaveExistsResult ExistsB = Storage->CheckExists(EPRSaveGeneration::B);
	ObservedA = FObservedGeneration();
	ObservedB = FObservedGeneration();
	ObservedA.Result = ExistsA == ISaveGameSystem::ESaveExistsResult::DoesNotExist
		? EPRSaveResult::NotFound
		: EPRSaveResult::ReadFailed;
	ObservedB.Result = ExistsB == ISaveGameSystem::ESaveExistsResult::DoesNotExist
		? EPRSaveResult::NotFound
		: EPRSaveResult::ReadFailed;

	EPRSaveResult Result = EPRSaveResult::Success;
	if (ExistsA == ISaveGameSystem::ESaveExistsResult::UnspecifiedError ||
		ExistsB == ISaveGameSystem::ESaveExistsResult::UnspecifiedError)
	{
		Result = EPRSaveResult::ReadFailed;
	}
	else if (ExistsA != ISaveGameSystem::ESaveExistsResult::DoesNotExist ||
		ExistsB != ISaveGameSystem::ESaveExistsResult::DoesNotExist)
	{
		Result = EPRSaveResult::SlotOccupied;
	}

	if (Result == EPRSaveResult::Success)
	{
		LoadedSave = NewObject<UPRSaveGame>(this);
		LoadedSave->SchemaVersion = UPRSaveGame::CurrentSchemaVersion;
		LoadedSave->SaveRevision = 0;
		LoadedSave->Profile.ProfileId = FGuid::NewGuid();
		LoadedSave->Profile.CompanionRelationships = FPRCompanionContract::BuildDefaultRelationshipRecords();
		LoadedGeneration = EPRSaveGeneration::None;
		bNeedsResave = true;
	}

	LastResult = Result;
	PublishOperation(
		OutRequestId,
		EPRSaveOperationType::Create,
		Result,
		0,
		EPRSaveGeneration::None,
		false,
		Result == EPRSaveResult::Success);
	return Result;
}

EPRSaveRequestStatus UPRSaveSubsystem::RequestSaveCurrentProfile(FGuid& OutRequestId)
{
	if (!IsCallableOnGameThread(OutRequestId))
	{
		return EPRSaveRequestStatus::Invalid;
	}
	if (State == EPRSaveSubsystemState::ShuttingDown || State == EPRSaveSubsystemState::Uninitialized)
	{
		return EPRSaveRequestStatus::RejectedShuttingDown;
	}
	if (!LoadedSave)
	{
		return EPRSaveRequestStatus::RejectedNoProfile;
	}
	if (State == EPRSaveSubsystemState::Loading || bPublishingCompletion)
	{
		return EPRSaveRequestStatus::RejectedBusy;
	}

	UPRSaveGame* Snapshot = CaptureCurrentSnapshot();
	if (!Snapshot)
	{
		return EPRSaveRequestStatus::Invalid;
	}

	if (State == EPRSaveSubsystemState::Saving)
	{
		if (!ActiveSave)
		{
			return EPRSaveRequestStatus::RejectedBusy;
		}
		if (!TrailingSave)
		{
			TrailingSave = MakeUnique<FSaveRequest>(Snapshot);
			TrailingSave->RequestId = FGuid::NewGuid();
		}
		else
		{
			TrailingSave->Snapshot = TStrongObjectPtr<UPRSaveGame>(Snapshot);
		}
		OutRequestId = TrailingSave->RequestId;
		return EPRSaveRequestStatus::Coalesced;
	}
	if (State != EPRSaveSubsystemState::Ready)
	{
		return EPRSaveRequestStatus::RejectedBusy;
	}

	ActiveSave = MakeUnique<FSaveRequest>(Snapshot);
	ActiveSave->RequestId = FGuid::NewGuid();
	OutRequestId = ActiveSave->RequestId;
	State = EPRSaveSubsystemState::Saving;
	StartActiveSave();
	return EPRSaveRequestStatus::Started;
}

FPRSaveRuntimeState UPRSaveSubsystem::GetSaveRuntimeState() const
{
	FPRSaveRuntimeState RuntimeState;
	RuntimeState.State = State;
	RuntimeState.bHasLoadedProfile = IsValid(LoadedSave);
	if (LoadedSave)
	{
		RuntimeState.ProfileId = LoadedSave->Profile.ProfileId;
		RuntimeState.SchemaVersion = LoadedSave->SchemaVersion;
		RuntimeState.SaveRevision = LoadedSave->SaveRevision;
	}
	RuntimeState.bNeedsResave = bNeedsResave;
	RuntimeState.bSaveRequestQueued = TrailingSave.IsValid();
	RuntimeState.LastResult = LastResult;
	RuntimeState.LoadedGeneration = LoadedGeneration;
	return RuntimeState;
}

FPRSaveOperationEventNative& UPRSaveSubsystem::OnSaveOperation()
{
	return SaveOperationEvent;
}

bool UPRSaveSubsystem::GetLoadedProfileSnapshot(FPRProfileSaveData& OutProfile) const
{
	OutProfile = FPRProfileSaveData();
	if (!IsInGameThread() || !LoadedSave)
	{
		return false;
	}
	OutProfile = LoadedSave->Profile;
	return true;
}

bool UPRSaveSubsystem::StageCompanionRelationships(const TArray<FPRCompanionRelationshipRecord>& Records)
{
	if (!IsInGameThread() || !LoadedSave || State != EPRSaveSubsystemState::Ready
		|| !FPRCompanionContract::AreCanonicalRelationshipRecords(Records))
	{
		return false;
	}
	LoadedSave->Profile.CompanionRelationships = Records;
	bNeedsResave = true;
	return true;
}

UPRSaveSubsystem::FObservedGeneration UPRSaveSubsystem::ObserveGeneration(
	const FPRSaveGenerationRead& Read)
{
	FObservedGeneration Observed;
	Observed.Result = Read.Result;
	Observed.RawEnvelope = Read.RawEnvelope;
	if (Read.SaveGame)
	{
		Observed.ProfileId = Read.SaveGame->Profile.ProfileId;
		Observed.SchemaVersion = Read.SaveGame->SchemaVersion;
		Observed.SaveRevision = Read.SaveGame->SaveRevision;
	}
	return Observed;
}

UPRSaveSubsystem::FLoadSelection UPRSaveSubsystem::SelectLoadedGeneration(
	const FPRSaveGenerationRead& ReadA,
	const FPRSaveGenerationRead& ReadB)
{
	FLoadSelection Selection;
	const bool bValidA = PRSaveSubsystemPrivate::IsSuccessfulRead(ReadA);
	const bool bValidB = PRSaveSubsystemPrivate::IsSuccessfulRead(ReadB);

	if (ReadA.Result == EPRSaveResult::FutureEnvelopeVersion ||
		ReadB.Result == EPRSaveResult::FutureEnvelopeVersion)
	{
		Selection.Result = EPRSaveResult::FutureEnvelopeVersion;
		return Selection;
	}
	if (ReadA.Result == EPRSaveResult::FutureSchemaVersion ||
		ReadB.Result == EPRSaveResult::FutureSchemaVersion)
	{
		Selection.Result = EPRSaveResult::FutureSchemaVersion;
		return Selection;
	}

	if (bValidA && bValidB)
	{
		if (ReadA.SaveGame->Profile.ProfileId != ReadB.SaveGame->Profile.ProfileId)
		{
			Selection.Result = EPRSaveResult::GenerationConflict;
			return Selection;
		}
		if (ReadA.SaveGame->SaveRevision == ReadB.SaveGame->SaveRevision)
		{
			if (ReadA.Payload != ReadB.Payload)
			{
				Selection.Result = EPRSaveResult::GenerationConflict;
				return Selection;
			}
			Selection.SaveGame = ReadA.SaveGame;
			Selection.Generation = EPRSaveGeneration::A;
			Selection.SourceSchemaVersion = ReadA.SourceSchemaVersion;
			Selection.bNeedsResave = ReadA.bNeedsResave || ReadB.bNeedsResave;
			Selection.Result = EPRSaveResult::Success;
			return Selection;
		}

		const FPRSaveGenerationRead& Newer = ReadA.SaveGame->SaveRevision > ReadB.SaveGame->SaveRevision
			? ReadA
			: ReadB;
		Selection.SaveGame = Newer.SaveGame;
		Selection.Generation = Newer.Generation;
		Selection.SourceSchemaVersion = Newer.SourceSchemaVersion;
		Selection.bNeedsResave = Newer.bNeedsResave;
		Selection.Result = EPRSaveResult::Success;
		return Selection;
	}

	if (bValidA || bValidB)
	{
		const FPRSaveGenerationRead& Valid = bValidA ? ReadA : ReadB;
		const FPRSaveGenerationRead& Alternate = bValidA ? ReadB : ReadA;
		Selection.SaveGame = Valid.SaveGame;
		Selection.Generation = Valid.Generation;
		Selection.SourceSchemaVersion = Valid.SourceSchemaVersion;
		Selection.bUsedAlternateGeneration = Alternate.Result != EPRSaveResult::NotFound;
		Selection.bNeedsResave = Valid.bNeedsResave || Selection.bUsedAlternateGeneration;
		Selection.Result = Selection.bUsedAlternateGeneration
			? EPRSaveResult::RecoveredFromAlternate
			: EPRSaveResult::Success;
		return Selection;
	}

	Selection.Result = PRSaveSubsystemPrivate::GetFailurePriority(ReadA.Result) >=
		PRSaveSubsystemPrivate::GetFailurePriority(ReadB.Result)
		? ReadA.Result
		: ReadB.Result;
	return Selection;
}

bool UPRSaveSubsystem::ObservationsMatch(
	const FObservedGeneration& Expected,
	const FObservedGeneration& Current)
{
	return Expected.Result == Current.Result &&
		Expected.ProfileId == Current.ProfileId &&
		Expected.SchemaVersion == Current.SchemaVersion &&
		Expected.SaveRevision == Current.SaveRevision &&
		Expected.RawEnvelope == Current.RawEnvelope;
}

void UPRSaveSubsystem::PublishOperation(
	const FGuid& RequestId,
	const EPRSaveOperationType Operation,
	const EPRSaveResult Result,
	const int32 SourceSchemaVersion,
	const EPRSaveGeneration Generation,
	const bool bUsedAlternateGeneration,
	const bool bEventNeedsResave)
{
	check(IsInGameThread());
	FPRSaveOperationEvent Event;
	Event.RequestId = RequestId;
	Event.Operation = Operation;
	Event.Result = Result;
	Event.SourceSchemaVersion = SourceSchemaVersion;
	Event.Generation = Generation;
	Event.bUsedAlternateGeneration = bUsedAlternateGeneration;
	Event.bNeedsResave = bEventNeedsResave;
	if (LoadedSave)
	{
		Event.ProfileId = LoadedSave->Profile.ProfileId;
		Event.FinalSchemaVersion = LoadedSave->SchemaVersion;
		Event.SaveRevision = LoadedSave->SaveRevision;
	}
	SaveOperationEvent.Broadcast(Event);
}

void UPRSaveSubsystem::StartActiveSave()
{
	check(IsInGameThread());
	check(ActiveSave && LoadedSave && Storage);

	if (LoadedSave->SaveRevision == MAX_int64 ||
		!LoadedSave->Profile.ProfileId.IsValid() ||
		LoadedSave->SchemaVersion != UPRSaveGame::CurrentSchemaVersion)
	{
		CompleteActiveSave(EPRSaveResult::InvalidRequest);
		return;
	}

	ActiveSave->Snapshot->SchemaVersion = UPRSaveGame::CurrentSchemaVersion;
	ActiveSave->Snapshot->SaveRevision = LoadedSave->SaveRevision + 1;
	const EPRSaveResult SerializeResult = Storage->SerializeEnvelope(
		*ActiveSave->Snapshot,
		ActiveSave->Envelope);
	if (SerializeResult != EPRSaveResult::Success)
	{
		CompleteActiveSave(SerializeResult);
		return;
	}

	const FPRSaveGenerationRead ReadA = Storage->ReadGenerationSync(EPRSaveGeneration::A, &MigrationRegistry);
	const FPRSaveGenerationRead ReadB = Storage->ReadGenerationSync(EPRSaveGeneration::B, &MigrationRegistry);
	ActiveSave->PrewriteA = ObserveGeneration(ReadA);
	ActiveSave->PrewriteB = ObserveGeneration(ReadB);

	if (ReadA.Result == EPRSaveResult::FutureEnvelopeVersion ||
		ReadB.Result == EPRSaveResult::FutureEnvelopeVersion)
	{
		CompleteActiveSave(EPRSaveResult::FutureEnvelopeVersion);
		return;
	}
	if (ReadA.Result == EPRSaveResult::FutureSchemaVersion ||
		ReadB.Result == EPRSaveResult::FutureSchemaVersion)
	{
		CompleteActiveSave(EPRSaveResult::FutureSchemaVersion);
		return;
	}
	if (!ObservationsMatch(ObservedA, ActiveSave->PrewriteA) ||
		!ObservationsMatch(ObservedB, ActiveSave->PrewriteB))
	{
		CompleteActiveSave(EPRSaveResult::GenerationConflict);
		return;
	}

	ActiveSave->TargetGeneration = LoadedGeneration == EPRSaveGeneration::A
		? EPRSaveGeneration::B
		: EPRSaveGeneration::A;
	TSharedRef<const TArray<uint8>> SharedEnvelope = MakeShared<TArray<uint8>>(ActiveSave->Envelope);
	const TWeakObjectPtr<UPRSaveSubsystem> WeakThis(this);
	Storage->SaveGenerationAsync(
		ActiveSave->TargetGeneration,
		SharedEnvelope,
		[WeakThis](const bool bSuccess)
		{
			if (UPRSaveSubsystem* Subsystem = WeakThis.Get())
			{
				if (Subsystem->State == EPRSaveSubsystemState::Saving)
				{
					Subsystem->HandleActiveWriteComplete(bSuccess);
				}
			}
		});
}

void UPRSaveSubsystem::HandleActiveWriteComplete(const bool bSuccess)
{
	check(IsInGameThread());
	if (!ActiveSave || !Storage)
	{
		return;
	}
	if (!bSuccess)
	{
		CompleteActiveSave(EPRSaveResult::WriteFailed);
		return;
	}

	const TWeakObjectPtr<UPRSaveSubsystem> WeakThis(this);
	Storage->LoadGenerationAsync(
		ActiveSave->TargetGeneration,
		[WeakThis](const bool bLoaded, TArray<uint8> Data)
		{
			if (UPRSaveSubsystem* Subsystem = WeakThis.Get())
			{
				if (Subsystem->State == EPRSaveSubsystemState::Saving)
				{
					Subsystem->HandleActiveVerificationComplete(bLoaded, MoveTemp(Data));
				}
			}
		});
}

void UPRSaveSubsystem::HandleActiveVerificationComplete(
	const bool bSuccess,
	TArray<uint8> Data)
{
	check(IsInGameThread());
	if (!ActiveSave)
	{
		return;
	}
	if (!bSuccess || Data != ActiveSave->Envelope)
	{
		CompleteActiveSave(EPRSaveResult::VerificationFailed);
		return;
	}

	FPRSaveDecodedEnvelope Decoded;
	if (!Storage || Storage->DeserializeEnvelope(Data, Decoded, &MigrationRegistry) != EPRSaveResult::Success ||
		!Decoded.SaveGame ||
		Decoded.SaveGame->Profile.ProfileId != ActiveSave->Snapshot->Profile.ProfileId ||
		Decoded.SaveGame->SchemaVersion != ActiveSave->Snapshot->SchemaVersion ||
		Decoded.SaveGame->SaveRevision != ActiveSave->Snapshot->SaveRevision)
	{
		CompleteActiveSave(EPRSaveResult::VerificationFailed);
		return;
	}

	LoadedSave = ActiveSave->Snapshot.Get();
	LoadedGeneration = ActiveSave->TargetGeneration;
	bNeedsResave = false;
	if (LoadedGeneration == EPRSaveGeneration::A)
	{
		ObservedA = ObserveGeneration(FPRSaveGenerationRead{
			EPRSaveGeneration::A,
			EPRSaveResult::Success,
			ISaveGameSystem::ESaveExistsResult::OK,
			LoadedSave,
			LoadedSave->SchemaVersion,
			false,
			Decoded.Payload,
			MoveTemp(Data)});
		ObservedB = ActiveSave->PrewriteB;
	}
	else
	{
		ObservedA = ActiveSave->PrewriteA;
		ObservedB = ObserveGeneration(FPRSaveGenerationRead{
			EPRSaveGeneration::B,
			EPRSaveResult::Success,
			ISaveGameSystem::ESaveExistsResult::OK,
			LoadedSave,
			LoadedSave->SchemaVersion,
			false,
			Decoded.Payload,
			MoveTemp(Data)});
	}
	CompleteActiveSave(EPRSaveResult::Success);
}

void UPRSaveSubsystem::CompleteActiveSave(const EPRSaveResult Result)
{
	check(IsInGameThread());
	if (!ActiveSave)
	{
		return;
	}

	bPublishingCompletion = true;
	LastResult = Result;
	const FGuid CompletedRequestId = ActiveSave->RequestId;
	const EPRSaveGeneration CompletedGeneration = Result == EPRSaveResult::Success
		? LoadedGeneration
		: EPRSaveGeneration::None;
	PublishOperation(
		CompletedRequestId,
		EPRSaveOperationType::Save,
		Result,
		LoadedSave ? LoadedSave->SchemaVersion : 0,
		CompletedGeneration,
		false,
		bNeedsResave);
	bPublishingCompletion = false;
	ActiveSave.Reset();

	if (Result != EPRSaveResult::Success)
	{
		if (TrailingSave)
		{
			PublishOperation(
				TrailingSave->RequestId,
				EPRSaveOperationType::Save,
				EPRSaveResult::CancelledAfterPriorFailure,
				LoadedSave ? LoadedSave->SchemaVersion : 0,
				EPRSaveGeneration::None,
				false,
				bNeedsResave);
			TrailingSave.Reset();
		}
		State = EPRSaveSubsystemState::Ready;
		return;
	}

	if (TrailingSave)
	{
		ActiveSave = MoveTemp(TrailingSave);
		StartActiveSave();
		return;
	}
	State = EPRSaveSubsystemState::Ready;
}

UPRSaveGame* UPRSaveSubsystem::CaptureCurrentSnapshot() const
{
	check(IsInGameThread());
	return LoadedSave ? DuplicateObject<UPRSaveGame>(LoadedSave, GetTransientPackage()) : nullptr;
}

bool UPRSaveSubsystem::IsCallableOnGameThread(FGuid& OutRequestId) const
{
	OutRequestId.Invalidate();
	if (IsInGameThread())
	{
		return true;
	}
	UE_LOG(LogProjectR, Error, TEXT("ProjectR Save API rejected a non-Game-Thread call."));
	return false;
}
