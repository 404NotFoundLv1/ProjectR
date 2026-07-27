// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRAccountSaveTypes.h"

/** Collects only stable values; it deliberately stores no actor, UObject, handle, timer or delegate. */
class PROJECTR_API FPRRunSummaryBuilder
{
public:
	void Reset(FGuid InRunId, FGuid InAccountId, int32 InSeed, FPrimaryAssetId InIdentityId, int64 InStartedUtc);
	void RecordRoom(FPrimaryAssetId RoomId);
	void RecordReward(FPrimaryAssetId RewardId);
	void RecordDirectorRule(FGameplayTag RuleId, int32 Level);
	void RecordQTEResult(FPrimaryAssetId QTEId, FGameplayTag CompanionId, FGameplayTag ResultTag, FName TimingGradeId);
	void RecordSkill(FGameplayTag AbilityTag, bool bCommitted);
	void RecordDamage(double DamageDealt, double DamageTaken, double ShieldAbsorbed, float HealthRatio);
	void SetCompanionSnapshot(FGameplayTag PrimaryCompanionId, const TArray<FPRCompanionRelationshipRecord>& Relationships);
	void SetBossResult(FGameplayTag BossId, bool bCompleted);
	void SetDeathCause(const FPRRunDeathCause& Cause);
	FPRRunSummary Build(EPRAccountTerminationReason Reason, int64 EndedUtc) const;

private:
	FPRRunSummary Summary;
};
