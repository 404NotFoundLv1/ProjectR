// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/Account/PRRunSummaryBuilder.h"

void FPRRunSummaryBuilder::Reset(const FGuid InRunId, const FGuid InAccountId, const int32 InSeed, const FPrimaryAssetId InIdentityId, const int64 InStartedUtc)
{
	Summary = FPRRunSummary();
	Summary.RunId = InRunId;
	Summary.AccountId = InAccountId;
	Summary.Seed = InSeed;
	Summary.IdentityId = InIdentityId;
	Summary.StartedUtc = InStartedUtc;
	Summary.MinimumHealthRatio = 1.0f;
}

void FPRRunSummaryBuilder::RecordRoom(const FPrimaryAssetId RoomId) { Summary.RoomIds.Add(RoomId); }
void FPRRunSummaryBuilder::RecordReward(const FPrimaryAssetId RewardId) { Summary.RewardIds.Add(RewardId); }

void FPRRunSummaryBuilder::RecordDirectorRule(const FGameplayTag RuleId, const int32 Level)
{
	for (FPRRunDirectorRuleSummary& Existing : Summary.DirectorRules)
	{
		if (Existing.RuleId == RuleId) { Existing.Level = FMath::Max(Existing.Level, Level); return; }
	}
	FPRRunDirectorRuleSummary& Added = Summary.DirectorRules.AddDefaulted_GetRef();
	Added.RuleId = RuleId;
	Added.Level = Level;
}

void FPRRunSummaryBuilder::RecordQTEResult(const FPrimaryAssetId QTEId, const FGameplayTag CompanionId, const FGameplayTag ResultTag, const FName TimingGradeId)
{
	for (FPRRunQTESummary& Existing : Summary.QTEResults)
	{
		if (Existing.QTEId == QTEId && Existing.CompanionId == CompanionId && Existing.ResultTag == ResultTag && Existing.TimingGradeId == TimingGradeId)
		{
			++Existing.Count;
			return;
		}
	}
	FPRRunQTESummary& Added = Summary.QTEResults.AddDefaulted_GetRef();
	Added.QTEId = QTEId;
	Added.CompanionId = CompanionId;
	Added.ResultTag = ResultTag;
	Added.TimingGradeId = TimingGradeId;
	Added.Count = 1;
}

void FPRRunSummaryBuilder::RecordSkill(const FGameplayTag AbilityTag, const bool bCommitted)
{
	for (FPRRunSkillSummary& Existing : Summary.SkillSummaries)
	{
		if (Existing.AbilityTag == AbilityTag) { ++Existing.UseCount; Existing.CommitCount += bCommitted ? 1 : 0; return; }
	}
	FPRRunSkillSummary& Added = Summary.SkillSummaries.AddDefaulted_GetRef();
	Added.AbilityTag = AbilityTag;
	Added.UseCount = 1;
	Added.CommitCount = bCommitted ? 1 : 0;
}

void FPRRunSummaryBuilder::RecordDamage(const double DamageDealt, const double DamageTaken, const double ShieldAbsorbed, const float HealthRatio)
{
	Summary.DamageDealt += FMath::Max(0.0, DamageDealt);
	Summary.DamageTaken += FMath::Max(0.0, DamageTaken);
	Summary.ShieldAbsorbed += FMath::Max(0.0, ShieldAbsorbed);
	Summary.MinimumHealthRatio = FMath::Min(Summary.MinimumHealthRatio, FMath::Clamp(HealthRatio, 0.0f, 1.0f));
}

void FPRRunSummaryBuilder::SetCompanionSnapshot(const FGameplayTag PrimaryCompanionId, const TArray<FPRCompanionRelationshipRecord>& Relationships)
{
	Summary.PrimaryCompanionId = PrimaryCompanionId;
	Summary.CompanionRelationships = Relationships;
}

void FPRRunSummaryBuilder::SetBossResult(const FGameplayTag BossId, const bool bCompleted) { Summary.BossId = BossId; Summary.bBossCompleted = bCompleted; }
void FPRRunSummaryBuilder::SetDeathCause(const FPRRunDeathCause& Cause) { Summary.DeathCause = Cause; }

FPRRunSummary FPRRunSummaryBuilder::Build(const EPRAccountTerminationReason Reason, const int64 EndedUtc) const
{
	FPRRunSummary Result = Summary;
	Result.TerminationReason = Reason;
	Result.EndedUtc = FMath::Max(Result.StartedUtc, EndedUtc);
	Result.DurationSeconds = static_cast<int32>(FMath::Min<int64>(TNumericLimits<int32>::Max(), Result.EndedUtc - Result.StartedUtc));
	Result.CounterproofFragmentsAwarded = Reason == EPRAccountTerminationReason::RoomSequenceCompleted ? 1 : 0;
	FPRAccountPersistenceContract::NormalizeSummary(Result);
	return Result;
}
