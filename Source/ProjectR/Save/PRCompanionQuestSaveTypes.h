// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "PRCompanionQuestSaveTypes.generated.h"

UENUM()
enum class EPRCompanionQuestPersistentState : uint8
{
	Locked,
	Available,
	Active,
	Completed
};

USTRUCT()
struct PROJECTR_API FPRCompanionQuestRecord
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FName QuestId;
	UPROPERTY(SaveGame) EPRCompanionQuestPersistentState State = EPRCompanionQuestPersistentState::Locked;
	UPROPERTY(SaveGame) int32 Stage = 0;
	UPROPERTY(SaveGame) int32 Progress = 0;
	UPROPERTY(SaveGame) TArray<FGuid> EvidenceIds;
	UPROPERTY(SaveGame) FGuid LastAccountId;
	UPROPERTY(SaveGame) int32 ArchivedAccountCount = 0;
	UPROPERTY(SaveGame) int64 CompletionSequence = 0;
};

USTRUCT()
struct PROJECTR_API FPRCompanionQuestPersistenceData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) TArray<FPRCompanionQuestRecord> Records;
};

struct PROJECTR_API FPRCompanionQuestPersistenceContract
{
	static constexpr int32 MaxRecords = 6;
	static constexpr int32 MaxEvidenceIds = 5;
	static constexpr int32 MaxArchivedAccountCount = 32;
	static bool IsKnownQuestId(const FName QuestId)
	{
		return QuestId == TEXT("Quest.Axiom.ImperfectOptimum") || QuestId == TEXT("Quest.Axiom.LowProbabilitySample")
			|| QuestId == TEXT("Quest.Kindle.LearnToRetreat") || QuestId == TEXT("Quest.Kindle.NoRetreatLine")
			|| QuestId == TEXT("Quest.Null.GarbageCollection") || QuestId == TEXT("Quest.Null.RememberMe");
	}

	static FPRCompanionQuestPersistenceData MakeDefault() { return FPRCompanionQuestPersistenceData(); }
	static void Normalize(FPRCompanionQuestPersistenceData& Value)
	{
		Value.Records.RemoveAll([](const FPRCompanionQuestRecord& Record) { return !IsKnownQuestId(Record.QuestId); });
		Value.Records.Sort([](const FPRCompanionQuestRecord& A, const FPRCompanionQuestRecord& B) { return A.QuestId.LexicalLess(B.QuestId); });
		for (int32 Index = Value.Records.Num() - 1; Index > 0; --Index)
		{
			if (Value.Records[Index].QuestId == Value.Records[Index - 1].QuestId) Value.Records.RemoveAt(Index);
		}
		Value.Records.SetNum(FMath::Min(Value.Records.Num(), MaxRecords));
		for (FPRCompanionQuestRecord& Record : Value.Records)
		{
			Record.Stage = FMath::Max(0, Record.Stage);
			Record.Progress = FMath::Max(0, Record.Progress);
			Record.ArchivedAccountCount = FMath::Clamp(Record.ArchivedAccountCount, 0, MaxArchivedAccountCount);
			Record.CompletionSequence = FMath::Max<int64>(0, Record.CompletionSequence);
			Record.EvidenceIds.RemoveAll([](const FGuid& Id) { return !Id.IsValid(); });
			Record.EvidenceIds.Sort([](const FGuid& A, const FGuid& B) { return A.ToString() < B.ToString(); });
			for (int32 EvidenceIndex = Record.EvidenceIds.Num() - 1; EvidenceIndex > 0; --EvidenceIndex)
			{
				if (Record.EvidenceIds[EvidenceIndex] == Record.EvidenceIds[EvidenceIndex - 1]) Record.EvidenceIds.RemoveAt(EvidenceIndex);
			}
			Record.EvidenceIds.SetNum(FMath::Min(Record.EvidenceIds.Num(), MaxEvidenceIds));
		}
	}
	static bool IsCanonical(const FPRCompanionQuestPersistenceData& Value)
	{
		if (Value.Records.Num() > MaxRecords) return false;
		FName Previous;
		for (const FPRCompanionQuestRecord& Record : Value.Records)
		{
			if (!IsKnownQuestId(Record.QuestId) || (!Previous.IsNone() && !Previous.LexicalLess(Record.QuestId)) || Record.Stage < 0 || Record.Progress < 0 || Record.ArchivedAccountCount < 0 || Record.ArchivedAccountCount > MaxArchivedAccountCount || Record.CompletionSequence < 0 || Record.EvidenceIds.Num() > MaxEvidenceIds) return false;
			FGuid PreviousEvidence;
			for (const FGuid& EvidenceId : Record.EvidenceIds)
			{
				if (!EvidenceId.IsValid() || (PreviousEvidence.IsValid() && PreviousEvidence.ToString() >= EvidenceId.ToString())) return false;
				PreviousEvidence = EvidenceId;
			}
			Previous = Record.QuestId;
		}
		return true;
	}
};
