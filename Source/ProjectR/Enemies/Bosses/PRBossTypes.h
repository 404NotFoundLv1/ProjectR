// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PRBossTypes.generated.h"

UENUM(BlueprintType)
enum class EPRAuditorBossPhase : uint8
{
	Dormant = 0,
	Sampling = 1,
	RuleAudit = 2,
	PredictionShield = 3,
	Defeated = 4
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRBossRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") FName BossId;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") FGuid SpawnId;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") EPRAuditorBossPhase Phase = EPRAuditorBossPhase::Dormant;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") float Health = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") float MaxHealth = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") float Shield = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") float MaxShield = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") FGameplayTag ActiveRuleId;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") FGameplayTag PredictedAbilityTag;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") FGameplayTag ActiveAttackTag;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") uint8 AttackPhase = 0;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") int32 PhaseSequence = 0;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") bool bDefeated = false;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") double WorldTimeSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRPrototypeRunResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") FGuid CompletionId;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") FName BossId;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") FGuid BossSpawnId;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") int32 CounterproofFragmentsAwarded = 1;
	UPROPERTY(BlueprintReadOnly, Category="ProjectR|Boss") double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRBossStateChangedNative, const FPRBossRuntimeState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPRPrototypeRunCompletedNative, const FPRPrototypeRunResult&);

/**
 * Bounded, value-only observation retained by the Auditor while it samples the
 * player. It is deliberately not replicated or persisted.
 */
USTRUCT()
struct PROJECTR_API FPRAuditorBossSample
{
	GENERATED_BODY()

	FGameplayTag AbilityTag;
	float Distance = 0.0f;
	bool bPerfectTiming = false;
	bool bDodgeOutcome = false;
	double WorldTimeSeconds = 0.0;
};
