// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PRAuditorChapterTypes.generated.h"

UENUM(BlueprintType)
enum class EPRAuditorChapterBossPhase : uint8
{
	Dormant,
	HabitReplication,
	RepeatedBuildAudit,
	VerdictEscalation,
	Defeated
};

UENUM(BlueprintType)
enum class EPRAuditorDistanceBand : uint8
{
	Near,
	Mid,
	Far,
	Unavailable
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRAuditorHabitProjection
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") FGameplayTag DominantSkillTag;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") EPRAuditorDistanceBand DistanceBand = EPRAuditorDistanceBand::Unavailable;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") bool bDodgeHeavy = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") bool bAvailable = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") FName FallbackReason;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRAuditorChapterBossRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") EPRAuditorChapterBossPhase Phase = EPRAuditorChapterBossPhase::Dormant;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") int32 AuditPressure = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") int32 RemainingAuditUnits = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") int32 RemainingVerdictSkills = 0;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") bool bRepeatedBuildCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") bool bVerdictCountered = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") bool bDegradedNoOp = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") FPRAuditorHabitProjection HabitProjection;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRAuditorStoryProjection
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") FName StoryBeatId;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") FText Text;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") bool bAvailable = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Auditor") FName FallbackReason;
};
