// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PRRelationshipTypes.h"
#include "Engine/AssetManagerTypes.h"
#include "GameplayTagContainer.h"

#include "PRAccountSaveTypes.generated.h"

UENUM()
enum class EPRAccountTerminationReason : uint8
{
	PlayerDeath,
	DivergenceEvacuation,
	DivergenceLeave,
	RoomSequenceCompleted,
	InterruptedRecovery
};

USTRUCT()
struct PROJECTR_API FPRRunDirectorRuleSummary
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FGameplayTag RuleId;
	UPROPERTY(SaveGame) int32 Level = 0;
};

USTRUCT()
struct PROJECTR_API FPRRunQTESummary
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FPrimaryAssetId QTEId;
	UPROPERTY(SaveGame) FGameplayTag CompanionId;
	UPROPERTY(SaveGame) FGameplayTag ResultTag;
	UPROPERTY(SaveGame) FName TimingGradeId;
	UPROPERTY(SaveGame) int32 Count = 0;
};

USTRUCT()
struct PROJECTR_API FPRRunSkillSummary
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FGameplayTag AbilityTag;
	UPROPERTY(SaveGame) int32 UseCount = 0;
	UPROPERTY(SaveGame) int32 CommitCount = 0;
};

USTRUCT()
struct PROJECTR_API FPRRunDeathCause
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FGuid CombatEventId;
	UPROPERTY(SaveGame) FGameplayTag SourceId;
	UPROPERTY(SaveGame) FGameplayTag AbilityTag;
	UPROPERTY(SaveGame) FGameplayTagContainer DamageTags;
	UPROPERTY(SaveGame) FGameplayTagContainer ResponseTags;
};

USTRUCT()
struct PROJECTR_API FPRRunSummary
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FGuid RunId;
	UPROPERTY(SaveGame) FGuid AccountId;
	UPROPERTY(SaveGame) FPrimaryAssetId IdentityId;
	UPROPERTY(SaveGame) int32 Seed = 0;
	UPROPERTY(SaveGame) int64 StartedUtc = 0;
	UPROPERTY(SaveGame) int64 EndedUtc = 0;
	UPROPERTY(SaveGame) int32 DurationSeconds = 0;
	UPROPERTY(SaveGame) EPRAccountTerminationReason TerminationReason = EPRAccountTerminationReason::InterruptedRecovery;
	UPROPERTY(SaveGame) TArray<FPrimaryAssetId> RoomIds;
	UPROPERTY(SaveGame) TArray<FPrimaryAssetId> RewardIds;
	UPROPERTY(SaveGame) TArray<FPRRunDirectorRuleSummary> DirectorRules;
	UPROPERTY(SaveGame) FGameplayTag PrimaryCompanionId;
	UPROPERTY(SaveGame) TArray<FPRCompanionRelationshipRecord> CompanionRelationships;
	UPROPERTY(SaveGame) TArray<FPRRunQTESummary> QTEResults;
	UPROPERTY(SaveGame) TArray<FPRRunSkillSummary> SkillSummaries;
	UPROPERTY(SaveGame) double DamageDealt = 0.0;
	UPROPERTY(SaveGame) double DamageTaken = 0.0;
	UPROPERTY(SaveGame) double ShieldAbsorbed = 0.0;
	UPROPERTY(SaveGame) float MinimumHealthRatio = 1.0f;
	UPROPERTY(SaveGame) int32 DeathCount = 0;
	UPROPERTY(SaveGame) FGameplayTag BossId;
	UPROPERTY(SaveGame) bool bBossCompleted = false;
	UPROPERTY(SaveGame) FPRRunDeathCause DeathCause;
	UPROPERTY(SaveGame) int32 CounterproofFragmentsAwarded = 0;
};

USTRUCT()
struct PROJECTR_API FPRActiveAccountSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FGuid AccountId;
	UPROPERTY(SaveGame) FPrimaryAssetId IdentityId;
	UPROPERTY(SaveGame) FGuid RunId;
	UPROPERTY(SaveGame) int32 Seed = 0;
	UPROPERTY(SaveGame) bool bRunStarted = false;
	UPROPERTY(SaveGame) int64 CreatedUtc = 0;
	UPROPERTY(SaveGame) int64 StartedUtc = 0;
};

USTRUCT()
struct PROJECTR_API FPRAccountRecord
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FGuid RecordId;
	UPROPERTY(SaveGame) FGuid AccountId;
	UPROPERTY(SaveGame) FPrimaryAssetId IdentityId;
	UPROPERTY(SaveGame) int64 EndedUtc = 0;
	UPROPERTY(SaveGame) EPRAccountTerminationReason TerminationReason = EPRAccountTerminationReason::InterruptedRecovery;
	UPROPERTY(SaveGame) FPRRunSummary Summary;
	UPROPERTY(SaveGame) int64 GraveyardOrdinal = 0;
};

USTRUCT()
struct PROJECTR_API FPRAccountPersistenceData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) bool bHasActiveAccount = false;
	UPROPERTY(SaveGame) FPRActiveAccountSaveData ActiveAccount;
	UPROPERTY(SaveGame) TArray<FPRAccountRecord> Graveyard;
	UPROPERTY(SaveGame) int64 LifetimeDeletedAccountCount = 0;
	UPROPERTY(SaveGame) int32 CounterproofFragments = 0;
};

/** Canonical, bounded persisted account values. It never accesses runtime state or user slots. */
struct PROJECTR_API FPRAccountPersistenceContract
{
	static constexpr int32 MaxGraveyardRecords = 32;
	static constexpr int32 MaxRooms = 10;
	static constexpr int32 MaxRewards = 10;
	static constexpr int32 MaxDirectorRules = 12;
	static constexpr int32 MaxQTEResults = 16;
	static constexpr int32 MaxSkillSummaries = 16;

	static FPRAccountPersistenceData MakeDefault()
	{
		return FPRAccountPersistenceData();
	}

	static void Normalize(FPRAccountPersistenceData& Value)
	{
		Value.LifetimeDeletedAccountCount = FMath::Max<int64>(0, Value.LifetimeDeletedAccountCount);
		Value.CounterproofFragments = FMath::Max(0, Value.CounterproofFragments);
		if (!Value.bHasActiveAccount || !IsActiveAccountValid(Value.ActiveAccount))
		{
			Value.bHasActiveAccount = false;
			Value.ActiveAccount = FPRActiveAccountSaveData();
		}
		for (FPRAccountRecord& Record : Value.Graveyard)
		{
			NormalizeSummary(Record.Summary);
		}
		Value.Graveyard.RemoveAll([](const FPRAccountRecord& Record) { return !IsRecordValid(Record); });
		Value.Graveyard.Sort([](const FPRAccountRecord& A, const FPRAccountRecord& B)
		{
			return A.EndedUtc != B.EndedUtc
				? A.EndedUtc < B.EndedUtc
				: A.RecordId.ToString(EGuidFormats::Digits) < B.RecordId.ToString(EGuidFormats::Digits);
		});
		Value.Graveyard.SetNum(FMath::Min(Value.Graveyard.Num(), MaxGraveyardRecords));
		Value.LifetimeDeletedAccountCount = FMath::Max<int64>(Value.LifetimeDeletedAccountCount, Value.Graveyard.Num());
	}

	static bool IsCanonical(const FPRAccountPersistenceData& Value)
	{
		if (Value.LifetimeDeletedAccountCount < 0 || Value.CounterproofFragments < 0 || Value.Graveyard.Num() > MaxGraveyardRecords)
		{
			return false;
		}
		if (Value.bHasActiveAccount != IsActiveAccountValid(Value.ActiveAccount))
		{
			return false;
		}
		FString Previous;
		TSet<FGuid> AccountIds;
		for (const FPRAccountRecord& Record : Value.Graveyard)
		{
			if (!IsRecordValid(Record) || AccountIds.Contains(Record.AccountId)) return false;
			const FString Current = FString::Printf(TEXT("%020lld:%s"), Record.EndedUtc, *Record.RecordId.ToString(EGuidFormats::Digits));
			if (!Previous.IsEmpty() && Previous >= Current) return false;
			Previous = Current;
			AccountIds.Add(Record.AccountId);
		}
		return Value.LifetimeDeletedAccountCount >= Value.Graveyard.Num()
			&& (!Value.bHasActiveAccount || !AccountIds.Contains(Value.ActiveAccount.AccountId));
	}

	static bool IsActiveAccountValid(const FPRActiveAccountSaveData& Value)
	{
		return Value.AccountId.IsValid() && Value.IdentityId.IsValid() && Value.CreatedUtc > 0
			&& (!Value.bRunStarted || (Value.RunId.IsValid() && Value.StartedUtc >= Value.CreatedUtc));
	}

	static bool IsRecordValid(const FPRAccountRecord& Value)
	{
		return Value.RecordId.IsValid() && Value.AccountId.IsValid() && Value.IdentityId.IsValid()
			&& Value.EndedUtc > 0 && Value.Summary.RunId.IsValid()
			&& Value.Summary.AccountId == Value.AccountId && Value.Summary.IdentityId == Value.IdentityId
			&& Value.Summary.EndedUtc == Value.EndedUtc && IsSummaryCanonical(Value.Summary);
	}

	static bool IsSummaryCanonical(const FPRRunSummary& Value)
	{
		if (!(Value.RunId.IsValid() && Value.AccountId.IsValid() && Value.IdentityId.IsValid()
			&& Value.StartedUtc > 0 && Value.EndedUtc >= Value.StartedUtc && Value.DurationSeconds >= 0
			&& Value.RoomIds.Num() <= MaxRooms && Value.RewardIds.Num() <= MaxRewards
			&& Value.DirectorRules.Num() <= MaxDirectorRules && Value.QTEResults.Num() <= MaxQTEResults
			&& Value.SkillSummaries.Num() <= MaxSkillSummaries && Value.CounterproofFragmentsAwarded >= 0
			&& FMath::IsFinite(Value.DamageDealt) && FMath::IsFinite(Value.DamageTaken)
			&& FMath::IsFinite(Value.ShieldAbsorbed) && FMath::IsFinite(Value.MinimumHealthRatio)))
		{
			return false;
		}
		TSet<FPrimaryAssetId> Rooms;
		TSet<FPrimaryAssetId> Rewards;
		TSet<FGameplayTag> Rules;
		TSet<FGameplayTag> Skills;
		TSet<FString> QTEKeys;
		for (const FPrimaryAssetId& Room : Value.RoomIds) { if (!Room.IsValid() || Rooms.Contains(Room)) return false; Rooms.Add(Room); }
		for (const FPrimaryAssetId& Reward : Value.RewardIds) { if (!Reward.IsValid() || Rewards.Contains(Reward)) return false; Rewards.Add(Reward); }
		for (const FPRRunDirectorRuleSummary& Rule : Value.DirectorRules) { if (!Rule.RuleId.IsValid() || Rule.Level <= 0 || Rules.Contains(Rule.RuleId)) return false; Rules.Add(Rule.RuleId); }
		for (const FPRRunSkillSummary& Skill : Value.SkillSummaries) { if (!Skill.AbilityTag.IsValid() || Skill.UseCount <= 0 || Skill.CommitCount < 0 || Skill.CommitCount > Skill.UseCount || Skills.Contains(Skill.AbilityTag)) return false; Skills.Add(Skill.AbilityTag); }
		for (const FPRRunQTESummary& QTE : Value.QTEResults)
		{
			const FString Key = QTE.QTEId.ToString() + TEXT("|") + QTE.CompanionId.ToString() + TEXT("|") + QTE.ResultTag.ToString() + TEXT("|") + QTE.TimingGradeId.ToString();
			if (!QTE.QTEId.IsValid() || QTE.Count <= 0 || QTEKeys.Contains(Key)) return false;
			QTEKeys.Add(Key);
		}
		return FPRCompanionContract::AreCanonicalRelationshipRecords(Value.CompanionRelationships);
	}

	static void NormalizeSummary(FPRRunSummary& Value)
	{
		Value.DurationSeconds = FMath::Max(0, Value.DurationSeconds);
		Value.EndedUtc = FMath::Max(Value.StartedUtc, Value.EndedUtc);
		Value.DamageDealt = FMath::Max(0.0, FMath::IsFinite(Value.DamageDealt) ? Value.DamageDealt : 0.0);
		Value.DamageTaken = FMath::Max(0.0, FMath::IsFinite(Value.DamageTaken) ? Value.DamageTaken : 0.0);
		Value.ShieldAbsorbed = FMath::Max(0.0, FMath::IsFinite(Value.ShieldAbsorbed) ? Value.ShieldAbsorbed : 0.0);
		Value.MinimumHealthRatio = FMath::Clamp(FMath::IsFinite(Value.MinimumHealthRatio) ? Value.MinimumHealthRatio : 1.0f, 0.0f, 1.0f);
		Value.DeathCount = FMath::Max(0, Value.DeathCount);
		Value.CounterproofFragmentsAwarded = FMath::Max(0, Value.CounterproofFragmentsAwarded);
		NormalizeIds(Value.RoomIds, MaxRooms);
		NormalizeIds(Value.RewardIds, MaxRewards);
		Value.DirectorRules.RemoveAll([](const FPRRunDirectorRuleSummary& Rule) { return !Rule.RuleId.IsValid() || Rule.Level <= 0; });
		Value.DirectorRules.Sort([](const FPRRunDirectorRuleSummary& A, const FPRRunDirectorRuleSummary& B) { return A.RuleId.ToString() < B.RuleId.ToString(); });
		for (int32 Index = Value.DirectorRules.Num() - 1; Index > 0; --Index)
		{
			if (Value.DirectorRules[Index].RuleId == Value.DirectorRules[Index - 1].RuleId)
			{
				Value.DirectorRules[Index - 1].Level = FMath::Max(Value.DirectorRules[Index - 1].Level, Value.DirectorRules[Index].Level);
				Value.DirectorRules.RemoveAt(Index);
			}
		}
		Value.DirectorRules.SetNum(FMath::Min(Value.DirectorRules.Num(), MaxDirectorRules));
		Value.QTEResults.RemoveAll([](const FPRRunQTESummary& QTE) { return !QTE.QTEId.IsValid() || QTE.Count <= 0; });
		Value.QTEResults.Sort([](const FPRRunQTESummary& A, const FPRRunQTESummary& B) { return A.QTEId.ToString() < B.QTEId.ToString(); });
		for (int32 Index = Value.QTEResults.Num() - 1; Index > 0; --Index)
		{
			FPRRunQTESummary& Current = Value.QTEResults[Index];
			FPRRunQTESummary& Previous = Value.QTEResults[Index - 1];
			if (Current.QTEId == Previous.QTEId && Current.CompanionId == Previous.CompanionId
				&& Current.ResultTag == Previous.ResultTag && Current.TimingGradeId == Previous.TimingGradeId)
			{
				Previous.Count = FMath::Min(MAX_int32, Previous.Count + Current.Count);
				Value.QTEResults.RemoveAt(Index);
			}
		}
		Value.QTEResults.SetNum(FMath::Min(Value.QTEResults.Num(), MaxQTEResults));
		Value.SkillSummaries.RemoveAll([](const FPRRunSkillSummary& Skill) { return !Skill.AbilityTag.IsValid() || (Skill.UseCount <= 0 && Skill.CommitCount <= 0); });
		Value.SkillSummaries.Sort([](const FPRRunSkillSummary& A, const FPRRunSkillSummary& B) { return A.AbilityTag.ToString() < B.AbilityTag.ToString(); });
		for (int32 Index = Value.SkillSummaries.Num() - 1; Index > 0; --Index)
		{
			if (Value.SkillSummaries[Index].AbilityTag == Value.SkillSummaries[Index - 1].AbilityTag)
			{
				Value.SkillSummaries[Index - 1].UseCount = FMath::Min(MAX_int32, Value.SkillSummaries[Index - 1].UseCount + Value.SkillSummaries[Index].UseCount);
				Value.SkillSummaries[Index - 1].CommitCount = FMath::Min(MAX_int32, Value.SkillSummaries[Index - 1].CommitCount + Value.SkillSummaries[Index].CommitCount);
				Value.SkillSummaries.RemoveAt(Index);
			}
		}
		Value.SkillSummaries.SetNum(FMath::Min(Value.SkillSummaries.Num(), MaxSkillSummaries));
		if (!FPRCompanionContract::AreCanonicalRelationshipRecords(Value.CompanionRelationships))
		{
			Value.CompanionRelationships = FPRCompanionContract::BuildDefaultRelationshipRecords();
		}
	}

private:
	static void NormalizeIds(TArray<FPrimaryAssetId>& Values, const int32 Maximum)
	{
		Values.RemoveAll([](const FPrimaryAssetId& Id) { return !Id.IsValid(); });
		Values.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.ToString() < B.ToString(); });
		for (int32 Index = Values.Num() - 1; Index > 0; --Index)
		{
			if (Values[Index] == Values[Index - 1]) Values.RemoveAt(Index);
		}
		Values.SetNum(FMath::Min(Values.Num(), Maximum));
	}
};
