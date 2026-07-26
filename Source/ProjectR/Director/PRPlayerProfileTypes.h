// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRRelationshipTypes.h"
#include "Divergence/PRDivergenceTypes.h"
#include "GameplayTagContainer.h"

struct PROJECTR_API FPRPlayerProfileTaggedCount
{
	FGameplayTag Tag;
	int32 Count = 0;
};

struct PROJECTR_API FPRPlayerProfileSkillMetric
{
	FGameplayTag SkillTag;
	int32 UseCount = 0;
	int32 CommitCount = 0;
	int32 FailureCount = 0;
};

struct PROJECTR_API FPRPlayerProfileDistanceMetric
{
	int32 SampleCount = 0;
	float MinimumDistanceCm = 0.0f;
	float MaximumDistanceCm = 0.0f;
	float AverageDistanceCm = 0.0f;
};

struct PROJECTR_API FPRPlayerProfileResourceMetric
{
	float DamageDealt = 0.0f;
	float DamageTaken = 0.0f;
	float ShieldAbsorbed = 0.0f;
	float EnergySpent = 0.0f;
	float MinimumHealthRatio = 1.0f;
};

struct PROJECTR_API FPRPlayerProfileDivergenceSummary
{
	FGuid ResultId;
	FGameplayTag CompanionId;
	EPRDivergenceChoice Choice = EPRDivergenceChoice::None;
	EPRDivergenceResolution Resolution = EPRDivergenceResolution::RejectedInvalid;
	EPRDivergenceFutureDisposition FutureDisposition = EPRDivergenceFutureDisposition::None;
};

/** Value-only, bounded runtime profile. It is never a SaveGame schema. */
struct PROJECTR_API FPRPlayerProfileSnapshot
{
	int32 SchemaVersion = 1;
	FGuid SnapshotId;
	FGuid ProfileSessionId;
	int64 Sequence = 0;
	TArray<FPRPlayerProfileSkillMetric> SkillMetrics;
	FPRPlayerProfileDistanceMetric CombatDistance;
	TArray<FPRPlayerProfileTaggedCount> QTEResultCounts;
	TArray<FPRPlayerProfileTaggedCount> QTEProfileSampleCounts;
	int32 QTEPerfectTimingCount = 0;
	int32 DeathCount = 0;
	FPRPlayerProfileResourceMetric Resources;
	FGameplayTag PrimaryCompanionId;
	TArray<FPRCompanionRelationshipRecord> Relationships;
	FPRPlayerProfileDivergenceSummary LastDivergence;
	TArray<FPRPlayerProfileTaggedCount> RuleCounterCounts;
	double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRPlayerProfileChangedNative, const FPRPlayerProfileSnapshot&);
