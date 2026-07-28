// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRMemorySaveTypes.h"

#include "PRMemoryTypes.generated.h"

UENUM()
enum class EPRMemoryState : uint8
{
	Uninitialized,
	Ready,
	WaitingForUpstream,
	ProviderPending,
	PersistencePending,
	ReadyToRetry,
	ShuttingDown
};

UENUM()
enum class EPRMemoryOperationResult : uint8
{
	Started,
	Succeeded,
	UsedFallback,
	NotReady,
	Busy,
	NoSummary,
	NoRetry,
	InvalidOption,
	PersistenceFailed,
	PersistenceConflict,
	Cancelled,
	InvalidData
};

UENUM()
enum class EPRMemoryPlayerOptionSlot : uint8
{
	First,
	Second,
	Third
};

USTRUCT()
struct PROJECTR_API FPRPostRunDialogueRequest
{
	GENERATED_BODY()

	FGuid RequestId;
	FGuid SummaryId;
	FName SceneId;
	FName CompanionId;
	int32 DurationSeconds = 0;
	int32 DeathCount = 0;
	int32 RuleLevel = 1;
	int32 QTECount = 1;
	TArray<FName> KeyEventIds;
};

USTRUCT()
struct PROJECTR_API FPRPostRunDialogueCandidate
{
	GENERATED_BODY()

	FName SceneId;
	FName CompanionId;
	FName EmotionId;
	FString Summary;
	TArray<FName> PlayerOptionIds;
};

USTRUCT()
struct PROJECTR_API FPRPostRunDialogueResult
{
	GENERATED_BODY()

	FGuid RequestId;
	FName SceneId;
	FName CompanionId;
	FName EmotionId;
	FString Summary;
	TArray<FName> PlayerOptionIds;
	bool bUsedFallback = false;
	FName FallbackReasonId;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRMemorySnapshot
{
	GENERATED_BODY()
	UPROPERTY(Transient) EPRMemoryState State = EPRMemoryState::Uninitialized;
	UPROPERTY(Transient) bool bRegistryReady = false;
	UPROPERTY(Transient) bool bProfileLoaded = false;
	UPROPERTY(Transient) bool bHasLatestSummary = false;
	UPROPERTY(Transient) FPRMemorySummary LatestSummary;
	/** Fixed local Persona DataAsset text projected for display only; never persisted or submitted. */
	UPROPERTY(Transient) TArray<FText> LatestOptionDisplayTexts;
};

USTRUCT()
struct PROJECTR_API FPRMemoryOperationEvent
{
	GENERATED_BODY()
	UPROPERTY(Transient) FGuid RequestId;
	UPROPERTY(Transient) EPRMemoryOperationResult Result = EPRMemoryOperationResult::InvalidData;
	UPROPERTY(Transient) FName ReasonId;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRMemoryStateChangedNative, const FPRMemorySnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRMemorySummaryReadyNative, const FPRMemorySummary&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRMemoryOperationNative, const FPRMemoryOperationEvent&);
