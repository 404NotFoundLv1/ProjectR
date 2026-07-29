// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"

#include "PRChapterSaveTypes.generated.h"

/** Bounded Profile-local chapter completion/proof persistence. */
USTRUCT()
struct PROJECTR_API FPRChapterPersistenceData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) TArray<FPrimaryAssetId> CompletedChapterIds;
	UPROPERTY(SaveGame) TArray<FName> HumanAnomalyProofIds;
	UPROPERTY(SaveGame) int64 SettlementSequence = 0;
};

struct PROJECTR_API FPRChapterPersistenceContract
{
	static constexpr int32 MaxEntries = 5;

	static FPRChapterPersistenceData MakeDefault() { return FPRChapterPersistenceData(); }

	static void Normalize(FPRChapterPersistenceData& Value)
	{
		Value.SettlementSequence = FMath::Max<int64>(0, Value.SettlementSequence);
		Value.CompletedChapterIds.RemoveAll([](const FPrimaryAssetId& Id) { return !Id.IsValid() || Id.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectRChapter")); });
		Value.CompletedChapterIds.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.ToString() < B.ToString(); });
		for (int32 Index = Value.CompletedChapterIds.Num() - 1; Index > 0; --Index) if (Value.CompletedChapterIds[Index] == Value.CompletedChapterIds[Index - 1]) Value.CompletedChapterIds.RemoveAt(Index);
		Value.CompletedChapterIds.SetNum(FMath::Min(Value.CompletedChapterIds.Num(), MaxEntries));
		Value.HumanAnomalyProofIds.RemoveAll([](const FName Id) { return Id.IsNone() || !Id.ToString().StartsWith(TEXT("HumanAnomalyProof.")); });
		Value.HumanAnomalyProofIds.Sort([](const FName A, const FName B) { return A.LexicalLess(B); });
		for (int32 Index = Value.HumanAnomalyProofIds.Num() - 1; Index > 0; --Index) if (Value.HumanAnomalyProofIds[Index] == Value.HumanAnomalyProofIds[Index - 1]) Value.HumanAnomalyProofIds.RemoveAt(Index);
		Value.HumanAnomalyProofIds.SetNum(FMath::Min(Value.HumanAnomalyProofIds.Num(), MaxEntries));
	}

	static bool IsCanonical(const FPRChapterPersistenceData& Value)
	{
		if (Value.SettlementSequence < 0 || Value.CompletedChapterIds.Num() > MaxEntries || Value.HumanAnomalyProofIds.Num() > MaxEntries) return false;
		FString PreviousChapter;
		for (const FPrimaryAssetId& Id : Value.CompletedChapterIds)
		{
			if (!Id.IsValid() || Id.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectRChapter")) || (!PreviousChapter.IsEmpty() && PreviousChapter >= Id.ToString())) return false;
			PreviousChapter = Id.ToString();
		}
		FString PreviousProof;
		for (const FName Id : Value.HumanAnomalyProofIds)
		{
			const FString Current = Id.ToString();
			if (Id.IsNone() || !Current.StartsWith(TEXT("HumanAnomalyProof.")) || (!PreviousProof.IsEmpty() && PreviousProof >= Current)) return false;
			PreviousProof = Current;
		}
		return true;
	}
};
