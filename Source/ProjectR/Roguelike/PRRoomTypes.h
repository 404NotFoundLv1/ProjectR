// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "GameplayTagContainer.h"

#include "PRRoomTypes.generated.h"

UENUM(BlueprintType)
enum class EPRRoomFlowStatus : uint8
{
	Idle,
	SelectingRoom,
	Travelling,
	EncounterActive,
	SelectingEvent,
	SelectingReward,
	Completed,
	Cancelled
};

UENUM(BlueprintType)
enum class EPRRoomOperationResult : uint8
{
	Succeeded,
	AlreadyStarted,
	NotReady,
	Invalid,
	NotFound,
	Rejected,
	Busy,
	NoEligibleContent,
	TravelFailed,
	CompletionAlreadyPublished
};

/** Result of configuring the closed content registry/context seam. */
UENUM(BlueprintType)
enum class EPRRoomContentResult : uint8
{
	Succeeded,
	Busy,
	NotFound,
	InvalidRegistry,
	RejectedContext
};

UENUM(BlueprintType)
enum class EPRRoomConditionKind : uint8
{
	Always,
	MinClearedRooms,
	PrimaryCompanion,
	DirectorRule,
	QTEResult,
	RoomType,
	HealthRatio,
	EnergyRatio,
	EncounterComplete
};

UENUM(BlueprintType)
enum class EPRRoomEncounterKind : uint8
{
	None,
	Combat,
	Elite,
	Boss
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRoomCondition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") EPRRoomConditionKind Kind = EPRRoomConditionKind::Always;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTag Tag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FName Name;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 IntegerValue = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") float FloatValue = 0.0f;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPREncounterSpawnDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTag PrototypeTag;
	/** Chapter content uses this stable identity; exactly one prototype selector is valid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId PrototypeId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FVector RelativeLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRoomPathStep
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 StepIndex = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPrimaryAssetId> CandidateRoomIds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId SelectedRoomId;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRoomRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FGuid SessionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 Seed = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") EPRRoomFlowStatus FlowStatus = EPRRoomFlowStatus::Idle;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 CurrentStepIndex = INDEX_NONE;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 PathLength = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPRRoomPathStep> Path;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId ActiveRoomId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FGuid ActiveRewardOfferId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") bool bEncounterComplete = false;
	/** Chapter-local, non-persistent explanation when a constrained offer used its deterministic fallback. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FName ChapterOfferFallbackReason;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRoomEventResult
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FGuid ResolutionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId RoomId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId EventId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FName ChoiceId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") bool bChoiceApplied = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") bool bEpicWeightBoosted = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRoomSequenceCompleted
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FGuid CompletionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") FGuid SessionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") int32 Seed = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPRRoomPathStep> CompletedPath;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPrimaryAssetId> RewardIds;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FGameplayTag> DirectorRuleIds;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Roguelike") double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRRoomStateChangedNative, const FPRRoomRuntimeState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRRoomEventResolvedNative, const FPRRoomEventResult&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRRoomSequenceCompletedNative, const FPRRoomSequenceCompleted&);
