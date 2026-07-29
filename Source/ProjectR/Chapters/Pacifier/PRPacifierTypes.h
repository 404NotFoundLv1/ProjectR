// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "PRPacifierTypes.generated.h"

UENUM(BlueprintType)
enum class EPRPacifierBossPhase : uint8
{
	Dormant,
	IllusionSplit,
	LowRiskRewardLure,
	AdventureYieldSuppression,
	Defeated
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRPacifierIllusionProjectionState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FVector RelativeOffset = FVector::ZeroVector;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bActive = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRPacifierBossRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") EPRPacifierBossPhase Phase = EPRPacifierBossPhase::Dormant;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 ComfortPressure = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 ActiveProjectionCount = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") TArray<FPRPacifierIllusionProjectionState> Projections;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 SuppressionLayers = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bIllusionSplitCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bLowRiskLureCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bYieldSuppressionCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bDegradedNoOp = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRPacifierStoryProjection
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FName StoryBeatId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FText Text;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bAvailable = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FName FallbackReason;
};
