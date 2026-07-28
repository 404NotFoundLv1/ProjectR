// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRRelationshipTypes.h"
#include "Save/PRAccountSaveTypes.h"
#include "Save/PRCompanionQuestSaveTypes.h"

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
struct PROJECTR_API FPRProgressionPersistenceData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) int32 MemoryFragments = 0;
	UPROPERTY(SaveGame) TArray<FPrimaryAssetId> UnlockedNodeIds;
	UPROPERTY(SaveGame) int64 UnlockSequence = 0;
};

/** Canonical, bounded progression values persisted by Save without depending on Roguelike runtime types. */
struct PROJECTR_API FPRProgressionPersistenceContract
{
	static constexpr int32 MaxUnlockedNodeIds = 12;

	static FPRProgressionPersistenceData MakeDefault() { return FPRProgressionPersistenceData(); }

	static void Normalize(FPRProgressionPersistenceData& Value)
	{
		Value.MemoryFragments = FMath::Max(0, Value.MemoryFragments);
		Value.UnlockSequence = FMath::Max<int64>(0, Value.UnlockSequence);
		Value.UnlockedNodeIds.RemoveAll([](const FPrimaryAssetId& Id)
		{
			return !Id.IsValid() || Id.PrimaryAssetType != FPrimaryAssetType(TEXT("ProgressionNode"));
		});
		Value.UnlockedNodeIds.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.ToString() < B.ToString(); });
		for (int32 Index = Value.UnlockedNodeIds.Num() - 1; Index > 0; --Index)
		{
			if (Value.UnlockedNodeIds[Index] == Value.UnlockedNodeIds[Index - 1]) Value.UnlockedNodeIds.RemoveAt(Index);
		}
		Value.UnlockedNodeIds.SetNum(FMath::Min(Value.UnlockedNodeIds.Num(), MaxUnlockedNodeIds));
	}

	static bool IsCanonical(const FPRProgressionPersistenceData& Value)
	{
		if (Value.MemoryFragments < 0 || Value.UnlockSequence < 0 || Value.UnlockedNodeIds.Num() > MaxUnlockedNodeIds) return false;
		FString Previous;
		for (const FPrimaryAssetId& Id : Value.UnlockedNodeIds)
		{
			if (!Id.IsValid() || Id.PrimaryAssetType != FPrimaryAssetType(TEXT("ProgressionNode"))) return false;
			const FString Current = Id.ToString();
			if (!Previous.IsEmpty() && Previous >= Current) return false;
			Previous = Current;
		}
		return true;
	}
};

USTRUCT()
struct PROJECTR_API FPRProfileSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FGuid ProfileId;

	UPROPERTY(SaveGame)
	TArray<FPRCompanionRelationshipRecord> CompanionRelationships;

	UPROPERTY(SaveGame)
	FPRAccountPersistenceData AccountPersistence;

	UPROPERTY(SaveGame)
	FPRProgressionPersistenceData ProgressionPersistence;

	UPROPERTY(SaveGame)
	FPRCompanionQuestPersistenceData CompanionQuestPersistence;
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
