// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRRelationshipTypes.h"

#include "PRSaveTypes.generated.h"

UENUM()
enum class EPRSaveSubsystemState : uint8
{
	Uninitialized,
	Ready,
	Loading,
	Saving,
	ShuttingDown
};

UENUM()
enum class EPRSaveGeneration : uint8
{
	None,
	A,
	B
};

UENUM()
enum class EPRSaveOperationType : uint8
{
	Create,
	Load,
	Save
};

UENUM()
enum class EPRSaveRequestStatus : uint8
{
	Started,
	Coalesced,
	RejectedNoProfile,
	RejectedBusy,
	RejectedShuttingDown,
	Invalid
};

UENUM()
enum class EPRSaveResult : uint8
{
	Success,
	RecoveredFromAlternate,
	NotFound,
	AlreadyLoaded,
	SlotOccupied,
	NoLoadedProfile,
	InvalidRequest,
	Busy,
	ReadFailed,
	EmptyData,
	InvalidEnvelope,
	FutureEnvelopeVersion,
	CorruptData,
	WrongSaveClass,
	MissingSchemaVersion,
	UnsupportedOldVersion,
	FutureSchemaVersion,
	MigrationFailed,
	GenerationConflict,
	SerializationFailed,
	WriteFailed,
	VerificationFailed,
	CancelledAfterPriorFailure,
	CancelledOnShutdown
};

USTRUCT()
struct PROJECTR_API FPRProfileSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FGuid ProfileId;

	UPROPERTY(SaveGame)
	TArray<FPRCompanionRelationshipRecord> CompanionRelationships;
};

USTRUCT()
struct PROJECTR_API FPRSaveRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	EPRSaveSubsystemState State = EPRSaveSubsystemState::Uninitialized;

	UPROPERTY(Transient)
	bool bHasLoadedProfile = false;

	UPROPERTY(Transient)
	FGuid ProfileId;

	UPROPERTY(Transient)
	int32 SchemaVersion = 0;

	UPROPERTY(Transient)
	int64 SaveRevision = 0;

	UPROPERTY(Transient)
	bool bNeedsResave = false;

	UPROPERTY(Transient)
	bool bSaveRequestQueued = false;

	UPROPERTY(Transient)
	EPRSaveResult LastResult = EPRSaveResult::InvalidRequest;

	UPROPERTY(Transient)
	EPRSaveGeneration LoadedGeneration = EPRSaveGeneration::None;
};

USTRUCT()
struct PROJECTR_API FPRSaveOperationEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FGuid RequestId;

	UPROPERTY(Transient)
	EPRSaveOperationType Operation = EPRSaveOperationType::Load;

	UPROPERTY(Transient)
	EPRSaveResult Result = EPRSaveResult::InvalidRequest;

	UPROPERTY(Transient)
	FGuid ProfileId;

	UPROPERTY(Transient)
	int32 SourceSchemaVersion = 0;

	UPROPERTY(Transient)
	int32 FinalSchemaVersion = 0;

	UPROPERTY(Transient)
	int64 SaveRevision = 0;

	UPROPERTY(Transient)
	EPRSaveGeneration Generation = EPRSaveGeneration::None;

	UPROPERTY(Transient)
	bool bUsedAlternateGeneration = false;

	UPROPERTY(Transient)
	bool bNeedsResave = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FPRSaveOperationEventNative,
	const FPRSaveOperationEvent&);
