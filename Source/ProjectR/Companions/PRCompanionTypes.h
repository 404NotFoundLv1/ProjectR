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

