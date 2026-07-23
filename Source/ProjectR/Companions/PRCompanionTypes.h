// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRRelationshipTypes.h"

#include "PRCompanionTypes.generated.h"

UENUM()
enum class EPRPrimaryCompanionSelectionResult : uint8
{
	Selected,
	AlreadySelected,
	RejectedRegistryUnavailable,
	RejectedUnknownCompanion,
	Invalid
};

UENUM()
enum class EPRRelationshipApplyResult : uint8
{
	Applied,
	AppliedClamped,
	RejectedNoLoadedProfile,
	RejectedUnknownCompanion,
	Invalid
};

UENUM(BlueprintType)
enum class EPRCompanionSupportType : uint8
{
	Shield,
	LightDamage,
	ControlMark
};

UENUM(BlueprintType)
enum class EPRCompanionSupportResult : uint8
{
	Applied,
	SkippedNoEligibleTarget,
	SkippedAtHealthFloor,
	SkippedBossPhaseFloor,
	RejectedInvalidState,
	RejectedByCombat
};

/** Value-only support fact for downstream consumers; it never owns gameplay state. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRCompanionSupportEvent
{
	GENERATED_BODY()

	FPRCompanionSupportEvent()
		: EventId(FGuid::NewGuid())
	{
	}

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion", meta=(IgnoreForMemberInitializationTest)) FGuid EventId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") FGameplayTag CompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") EPRCompanionSupportType SupportType = EPRCompanionSupportType::Shield;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") EPRCompanionSupportResult Result = EPRCompanionSupportResult::RejectedInvalidState;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") FName SourceId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") FName TargetId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") float RequestedMagnitude = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") float AppliedMagnitude = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") float HealthFloor = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") float Overload = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") float EffectiveIntervalSeconds = 0.0f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") FVector TargetLocation = FVector::ZeroVector;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion") double WorldTimeSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRCompanionSyncState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion")
	FGameplayTag PrimaryCompanionId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion")
	TArray<FGameplayTag> BackgroundCompanionIds;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion")
	int32 SelectionRevision = 0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRPrimaryCompanionSyncChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion")
	FPRCompanionSyncState PreviousState;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion")
	FPRCompanionSyncState CurrentState;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Companion")
	double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRRelationshipChangedNative, const FPRRelationshipChangedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRPrimaryCompanionSyncChangedNative, const FPRPrimaryCompanionSyncChangedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRCompanionSupportEventNative, const FPRCompanionSupportEvent&);
