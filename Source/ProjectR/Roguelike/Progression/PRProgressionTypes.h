// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"

#include "PRProgressionTypes.generated.h"

UENUM(BlueprintType)
enum class EPRProgressionTree : uint8
{
	Player,
	CompanionAI,
	Bond
};

UENUM(BlueprintType)
enum class EPRProgressionEffectKind : uint8
{
	PlayerMaxHealth,
	PlayerMaxEnergy,
	CompanionSupportInterval,
	EntitlementOnly
};

UENUM(BlueprintType)
enum class EPRProgressionRelationshipMetric : uint8
{
	None,
	Trust,
	Affection,
	Evaluation
};

UENUM(BlueprintType)
enum class EPRProgressionRelationshipScope : uint8
{
	None,
	PrimaryCompanion,
	AnyCompanion,
	AllCompanions
};

UENUM(BlueprintType)
enum class EPRProgressionOperationResult : uint8
{
	Success,
	Pending,
	NotReady,
	UnknownNode,
	AlreadyUnlocked,
	PrerequisiteNotMet,
	RelationshipRequirementNotMet,
	InsufficientCounterproofFragments,
	InsufficientMemoryFragments,
	PersistenceFailed,
	RetryNotAvailable,
	InvalidState
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRProgressionRelationshipRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression")
	EPRProgressionRelationshipMetric Metric = EPRProgressionRelationshipMetric::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression")
	EPRProgressionRelationshipScope Scope = EPRProgressionRelationshipScope::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression")
	int32 MinimumValue = 0;
};

USTRUCT()
struct PROJECTR_API FPRProgressionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(Transient) int32 CounterproofFragments = 0;
	UPROPERTY(Transient) int32 MemoryFragments = 0;
	UPROPERTY(Transient) TArray<FPrimaryAssetId> UnlockedNodeIds;
	UPROPERTY(Transient) int64 UnlockSequence = 0;
};

USTRUCT()
struct PROJECTR_API FPRProgressionRunSnapshot
{
	GENERATED_BODY()

	UPROPERTY(Transient) int32 PlayerMaxHealthBonus = 0;
	UPROPERTY(Transient) int32 PlayerMaxEnergyBonus = 0;
	UPROPERTY(Transient) float CompanionSupportIntervalMultiplier = 1.0f;
	UPROPERTY(Transient) TArray<FPrimaryAssetId> EntitlementIds;
	UPROPERTY(Transient) int64 SnapshotSequence = 0;
};

USTRUCT()
struct PROJECTR_API FPRProgressionChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient) FPRProgressionSnapshot Snapshot;
	UPROPERTY(Transient) FGuid RequestId;
};

USTRUCT()
struct PROJECTR_API FPRProgressionUnlockCompletedEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient) FGuid RequestId;
	UPROPERTY(Transient) FGuid SaveRequestId;
	UPROPERTY(Transient) FPrimaryAssetId NodeId;
	UPROPERTY(Transient) EPRProgressionOperationResult Result = EPRProgressionOperationResult::NotReady;
};

USTRUCT()
struct PROJECTR_API FPRProgressionRunSnapshotChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient) bool bHasActiveRunSnapshot = false;
	UPROPERTY(Transient) FPRProgressionRunSnapshot Snapshot;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRProgressionChangedNative, const FPRProgressionChangedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRProgressionUnlockCompletedNative, const FPRProgressionUnlockCompletedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRProgressionRunSnapshotChangedNative, const FPRProgressionRunSnapshotChangedEvent&);
