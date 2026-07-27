// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRAccountSaveTypes.h"

#include "PRAccountRuntimeTypes.generated.h"

UENUM()
enum class EPRRunLifecycleState : uint8
{
	Idle,
	CreatingAccount,
	AccountReady,
	StartingRun,
	RunActive,
	AwaitingDivergence,
	Finalizing,
	PersistenceFailed,
	FinalizedTravelPending
};

UENUM()
enum class EPRAccountOperationType : uint8
{
	CreateAccount,
	StartRun,
	FinalizeAccount,
	RetryPersistence,
	ReturnToReality
};

UENUM()
enum class EPRAccountOperationResult : uint8
{
	Started,
	Succeeded,
	AlreadyPending,
	AlreadyCompleted,
	RejectedNoProfile,
	RejectedRegistryUnavailable,
	RejectedUnknownIdentity,
	RejectedActiveAccount,
	RejectedNoAccount,
	RejectedRunAlreadyStarted,
	RejectedBusy,
	RejectedInvalidState,
	RejectedNoPendingPersistence,
	RejectedNoPendingTravel,
	PersistenceFailed,
	RoomStartFailed,
	TravelFailed,
	Invalid
};

USTRUCT()
struct PROJECTR_API FPRRunRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient) EPRRunLifecycleState State = EPRRunLifecycleState::Idle;
	UPROPERTY(Transient) FGuid AccountId;
	UPROPERTY(Transient) FGuid RunId;
	UPROPERTY(Transient) FPrimaryAssetId IdentityId;
	UPROPERTY(Transient) int32 Seed = 0;
	UPROPERTY(Transient) bool bPersistencePending = false;
	UPROPERTY(Transient) bool bTravelPending = false;
};

USTRUCT()
struct PROJECTR_API FPRAccountOperationEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient) FGuid RequestId;
	UPROPERTY(Transient) EPRAccountOperationType Operation = EPRAccountOperationType::CreateAccount;
	UPROPERTY(Transient) EPRAccountOperationResult Result = EPRAccountOperationResult::Invalid;
	UPROPERTY(Transient) EPRRunLifecycleState State = EPRRunLifecycleState::Idle;
	UPROPERTY(Transient) FGuid AccountId;
	UPROPERTY(Transient) FGuid RunId;
	UPROPERTY(Transient) FGuid SaveRequestId;
	UPROPERTY(Transient) double WorldTimeSeconds = 0.0;
};

USTRUCT()
struct PROJECTR_API FPRAccountDeletedEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient) FGuid RequestId;
	UPROPERTY(Transient) FPRAccountRecord Record;
	UPROPERTY(Transient) double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRRunStateChangedNative, const FPRRunRuntimeState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRAccountOperationNative, const FPRAccountOperationEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRAccountDeletedNative, const FPRAccountDeletedEvent&);

struct PROJECTR_API FPRRunTerminationArbiter
{
	static EPRAccountTerminationReason Resolve(
		const bool bRoomSequenceCompleted,
		const bool bDeathPending,
		const EPRAccountTerminationReason Candidate)
	{
		if (bRoomSequenceCompleted) return EPRAccountTerminationReason::RoomSequenceCompleted;
		if (Candidate == EPRAccountTerminationReason::DivergenceEvacuation || Candidate == EPRAccountTerminationReason::DivergenceLeave)
		{
			return Candidate;
		}
		return bDeathPending ? EPRAccountTerminationReason::PlayerDeath : Candidate;
	}
};
