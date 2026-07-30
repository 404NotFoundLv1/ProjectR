// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Save/PRAccountSaveTypes.h"

#include "PRTripleResonanceSaveTypes.generated.h"

USTRUCT()
struct PROJECTR_API FPRSkillMemoryFragment
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FGuid SourceSummaryId;
	UPROPERTY(SaveGame) FGameplayTag AbilityTag;
	UPROPERTY(SaveGame) int64 GraveyardOrdinal = 0;
	UPROPERTY(SaveGame) int64 LegacySequence = 0;

	bool IsValid() const
	{
		return SourceSummaryId.IsValid() && AbilityTag.IsValid() && GraveyardOrdinal > 0 && LegacySequence > 0;
	}
};

USTRUCT()
struct PROJECTR_API FPRTripleResonancePersistenceData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) int64 LastProcessedGraveyardOrdinal = 0;
	UPROPERTY(SaveGame) FPRSkillMemoryFragment SkillMemory;
	UPROPERTY(SaveGame) bool bHasHighRiskProof = false;
	UPROPERTY(SaveGame) int64 LegacySequence = 0;
	UPROPERTY(SaveGame) int64 HighRiskProofSequence = 0;
};

struct PROJECTR_API FPRTripleResonancePersistenceContract
{
	static FPRTripleResonancePersistenceData MakeDefault() { return FPRTripleResonancePersistenceData(); }

	static FGameplayTag SelectLegacySkillMemory(const TArray<FPRRunSkillSummary>& SkillSummaries)
	{
		static const TArray<FGameplayTag> ReleasedP0Skills = {
			FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust")), FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash")),
			FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop")), FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge")),
			FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook")), FGameplayTag::RequestGameplayTag(TEXT("Skill.CounterProofWall"))};
		TArray<FPRRunSkillSummary> Candidates;
		for (const FPRRunSkillSummary& Entry : SkillSummaries)
		{
			if (Entry.CommitCount > 0 && ReleasedP0Skills.Contains(Entry.AbilityTag)) Candidates.Add(Entry);
		}
		Candidates.Sort([](const FPRRunSkillSummary& A, const FPRRunSkillSummary& B)
		{
			if (A.CommitCount != B.CommitCount) return A.CommitCount > B.CommitCount;
			if (A.UseCount != B.UseCount) return A.UseCount > B.UseCount;
			return A.AbilityTag.ToString() < B.AbilityTag.ToString();
		});
		return Candidates.IsEmpty() ? FGameplayTag() : Candidates[0].AbilityTag;
	}

	static void Normalize(FPRTripleResonancePersistenceData& Value)
	{
		Value.LastProcessedGraveyardOrdinal = FMath::Max<int64>(0, Value.LastProcessedGraveyardOrdinal);
		Value.LegacySequence = FMath::Max<int64>(0, Value.LegacySequence);
		Value.HighRiskProofSequence = FMath::Max<int64>(0, Value.HighRiskProofSequence);
		if (!Value.SkillMemory.IsValid()
			|| Value.SkillMemory.GraveyardOrdinal > Value.LastProcessedGraveyardOrdinal
			|| Value.SkillMemory.LegacySequence > Value.LegacySequence) Value.SkillMemory = FPRSkillMemoryFragment();
		if (!Value.bHasHighRiskProof) Value.HighRiskProofSequence = 0;
	}

	static bool IsCanonical(const FPRTripleResonancePersistenceData& Value)
	{
		return Value.LastProcessedGraveyardOrdinal >= 0
			&& Value.LegacySequence >= 0
			&& Value.HighRiskProofSequence >= 0
			&& (!Value.SkillMemory.IsValid() || (Value.SkillMemory.GraveyardOrdinal <= Value.LastProcessedGraveyardOrdinal && Value.SkillMemory.LegacySequence <= Value.LegacySequence))
			&& (!Value.bHasHighRiskProof || Value.HighRiskProofSequence > 0);
	}
};
