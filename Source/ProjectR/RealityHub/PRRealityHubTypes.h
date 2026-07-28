// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRMapId.h"
#include "GameplayTagContainer.h"

#include "PRRealityHubTypes.generated.h"

/** Fixed, registry-backed Reality Hub terminal identities. */
UENUM(BlueprintType)
enum class EPRRealityHubTerminal : uint8
{
	None,
	CassetteSlot,
	Companion,
	Graveyard,
	TrainingSimulator,
	DirectorForecaster
};

/** The five registry-backed account identities; callers cannot supply arbitrary asset ids. */
UENUM(BlueprintType)
enum class EPRRealityHubIdentity : uint8
{
	Technician,
	Security,
	Exile,
	Observer,
	Blank
};

/** A user-facing result only; it never represents a direct Save or map operation. */
UENUM(BlueprintType)
enum class EPRRealityHubOperationResult : uint8
{
	None,
	Succeeded,
	Busy,
	ProfileUnavailable,
	ProfileCreationRequired,
	IdentityUnavailable,
	RunStateUnavailable,
	PersistenceFailed,
	OperationUnavailable,
	TravelFailed,
	Cancelled
};

UENUM(BlueprintType)
enum class EPRRealityHubForecastResult : uint8
{
	UnavailableProfile,
	UnavailableRegistry,
	Available
};

/** Bounded read-only terminal state rendered by the Hub widgets. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRRealityHubTerminalSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") EPRRealityHubTerminal Terminal = EPRRealityHubTerminal::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") bool bAvailable = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") FText DisplayName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") FText StatusText;
};

/** Value-only Hub snapshot. No SaveGame, Actor, UObject, timer, delegate, or handle is retained. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRRealityHubSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") int64 Sequence = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") bool bProfileLoaded = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") bool bOperationPending = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") EPRRealityHubTerminal ActiveTerminal = EPRRealityHubTerminal::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") TArray<FPRRealityHubTerminalSnapshot> Terminals;
};

/** Bounded operation notification emitted only after an existing subsystem reaches a terminal result. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRRealityHubOperationEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") int64 Sequence = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") EPRRealityHubTerminal Terminal = EPRRealityHubTerminal::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") EPRRealityHubOperationResult Result = EPRRealityHubOperationResult::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") FText Message;
};

/** Local deterministic preview. It is not a Director response and cannot apply a rule. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRRealityHubForecast
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") EPRRealityHubForecastResult Result = EPRRealityHubForecastResult::UnavailableProfile;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") FGameplayTag RuleId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") int32 Level = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") FText Explanation;
};

/** Reserved value extension point. v0.5.0 provides no quest content or progress semantics. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRRealityHubQuestEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") FName QuestId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|RealityHub") FText StatusText;
};
