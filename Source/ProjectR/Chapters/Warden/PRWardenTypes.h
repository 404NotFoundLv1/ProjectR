// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PRWardenTypes.generated.h"

UENUM(BlueprintType)
enum class EPRWardenBossPhase : uint8
{
	Dormant,
	PredictiveAttack,
	PlatformLockdown,
	RiskMark,
	Defeated
};

UENUM(BlueprintType)
enum class EPRWardenArenaLane : uint8
{
	None,
	Left,
	Center,
	Right
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRWardenBossRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") EPRWardenBossPhase Phase = EPRWardenBossPhase::Dormant;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") EPRWardenArenaLane LockedLane = EPRWardenArenaLane::None;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FGameplayTag PredictedSkillTag;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 RiskPressure = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") int32 RiskLayers = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bPredictiveAttackCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bPlatformLockdownCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bRiskMarkCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bDegradedNoOp = false;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRWardenStoryProjection
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FName StoryBeatId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FText Text;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") bool bAvailable = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Chapter") FName FallbackReason;
};
