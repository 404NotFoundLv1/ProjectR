// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRAccountSaveTypes.h"

#include "PRMemorySaveTypes.generated.h"

USTRUCT()
struct PROJECTR_API FPRMemoryChoiceRef
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FName SourceId;
	UPROPERTY(SaveGame) FName ContextId;
	UPROPERTY(SaveGame) FName ChoiceId;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRMemorySummary
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) FGuid SummaryId;
	UPROPERTY(SaveGame) EPRAccountTerminationReason TerminationReason = EPRAccountTerminationReason::InterruptedRecovery;
	UPROPERTY(SaveGame) FGameplayTag DeathCauseId;
	UPROPERTY(SaveGame) TArray<FPRRunDirectorRuleSummary> DirectorRules;
	UPROPERTY(SaveGame) TArray<FPRRunQTESummary> QTEResults;
	UPROPERTY(SaveGame) TArray<FPRMemoryChoiceRef> ChoiceRefs;
	UPROPERTY(SaveGame) TArray<FName> CompletedQuestIds;
	UPROPERTY(SaveGame) TArray<FName> KeyEventIds;
	UPROPERTY(SaveGame) FGameplayTag CompanionId;
	UPROPERTY(SaveGame) FName SceneId;
	UPROPERTY(SaveGame) FName EmotionId;
	UPROPERTY(SaveGame) FString SummaryText;
	UPROPERTY(SaveGame) TArray<FName> PlayerOptionIds;
	UPROPERTY(SaveGame) bool bUsedFallback = false;
	UPROPERTY(SaveGame) FName FallbackReasonId;
	UPROPERTY(SaveGame) int64 SummarySequence = 0;
	UPROPERTY(SaveGame) int64 GraveyardOrdinal = 0;
	UPROPERTY(SaveGame) int32 MemoryFragmentsAwarded = 0;
};

USTRUCT()
struct PROJECTR_API FPRMemoryPersistenceData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame) TArray<FPRMemorySummary> Summaries;
	UPROPERTY(SaveGame) int64 LastProcessedGraveyardOrdinal = 0;
	UPROPERTY(SaveGame) int64 LifetimeSummaryCount = 0;
	UPROPERTY(SaveGame) int64 LifetimeMemoryFragmentsAwarded = 0;
	UPROPERTY(SaveGame) int64 SummarySequence = 0;
};

/** Save-layer-only canonical form for the bounded v0.5.2 Memory partition. */
struct PROJECTR_API FPRMemoryPersistenceContract
{
	static constexpr int32 MaxSummaries = 32;
	static constexpr int32 MaxDirectorRules = 12;
	static constexpr int32 MaxQTEResults = 16;
	static constexpr int32 MaxChoiceRefs = 8;
	static constexpr int32 MaxCompletedQuestIds = 6;
	static constexpr int32 MaxKeyEventIds = 16;
	static constexpr int32 MaxSummaryCodePoints = 240;

	static FPRMemoryPersistenceData MakeDefault() { return FPRMemoryPersistenceData(); }
	static void Normalize(FPRMemoryPersistenceData& Value)
	{
		Value.LastProcessedGraveyardOrdinal = FMath::Max<int64>(0, Value.LastProcessedGraveyardOrdinal);
		Value.LifetimeSummaryCount = FMath::Max<int64>(0, Value.LifetimeSummaryCount);
		Value.LifetimeMemoryFragmentsAwarded = FMath::Max<int64>(0, Value.LifetimeMemoryFragmentsAwarded);
		Value.SummarySequence = FMath::Max<int64>(0, Value.SummarySequence);
		for (FPRMemorySummary& Summary : Value.Summaries) NormalizeSummary(Summary);
		Value.Summaries.RemoveAll([](const FPRMemorySummary& Summary) { return !IsSummaryCanonical(Summary); });
		Value.Summaries.Sort([](const FPRMemorySummary& A, const FPRMemorySummary& B) { return SummaryKey(A) < SummaryKey(B); });
		for (int32 Index = Value.Summaries.Num() - 1; Index > 0; --Index)
		{
			if (Value.Summaries[Index].SummaryId == Value.Summaries[Index - 1].SummaryId || Value.Summaries[Index].GraveyardOrdinal == Value.Summaries[Index - 1].GraveyardOrdinal) Value.Summaries.RemoveAt(Index);
		}
		if (Value.Summaries.Num() > MaxSummaries) Value.Summaries.RemoveAt(0, Value.Summaries.Num() - MaxSummaries);
		for (const FPRMemorySummary& Summary : Value.Summaries)
		{
			Value.LastProcessedGraveyardOrdinal = FMath::Max(Value.LastProcessedGraveyardOrdinal, Summary.GraveyardOrdinal);
			Value.SummarySequence = FMath::Max(Value.SummarySequence, Summary.SummarySequence);
		}
		Value.LifetimeSummaryCount = FMath::Max<int64>(Value.LifetimeSummaryCount, Value.Summaries.Num());
		int64 PersistedFragmentCount = 0;
		for (const FPRMemorySummary& Summary : Value.Summaries) PersistedFragmentCount += Summary.MemoryFragmentsAwarded;
		Value.LifetimeMemoryFragmentsAwarded = FMath::Max(Value.LifetimeMemoryFragmentsAwarded, PersistedFragmentCount);
	}

	static bool IsCanonical(const FPRMemoryPersistenceData& Value)
	{
		if (Value.Summaries.Num() > MaxSummaries || Value.LastProcessedGraveyardOrdinal < 0 || Value.LifetimeSummaryCount < Value.Summaries.Num() || Value.LifetimeMemoryFragmentsAwarded < 0 || Value.SummarySequence < 0) return false;
		FString PreviousKey;
		TSet<FGuid> SummaryIds;
		int64 PreviousOrdinal = 0;
		int64 PersistedFragmentCount = 0;
		for (const FPRMemorySummary& Summary : Value.Summaries)
		{
			if (!IsSummaryCanonical(Summary) || SummaryIds.Contains(Summary.SummaryId) || Summary.GraveyardOrdinal <= PreviousOrdinal) return false;
			const FString CurrentKey = SummaryKey(Summary);
			if (!PreviousKey.IsEmpty() && PreviousKey >= CurrentKey) return false;
			PreviousKey = CurrentKey;
			PreviousOrdinal = Summary.GraveyardOrdinal;
			SummaryIds.Add(Summary.SummaryId);
			PersistedFragmentCount += Summary.MemoryFragmentsAwarded;
		}
		return Value.LifetimeMemoryFragmentsAwarded >= PersistedFragmentCount && (Value.Summaries.IsEmpty() || Value.LastProcessedGraveyardOrdinal >= Value.Summaries.Last().GraveyardOrdinal);
	}

private:
	static FString SummaryKey(const FPRMemorySummary& Value)
	{
		return FString::Printf(TEXT("%020lld:%s"), Value.GraveyardOrdinal, *Value.SummaryId.ToString(EGuidFormats::Digits));
	}

	static bool IsKnownCompanion(const FGameplayTag Id)
	{
		return Id == FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false) || Id == FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false) || Id == FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false);
	}

	static bool IsKnownEmotion(const FGameplayTag CompanionId, const FName EmotionId)
	{
		if (CompanionId == FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false)) return EmotionId == TEXT("analytical") || EmotionId == TEXT("concerned") || EmotionId == TEXT("quietly_proud");
		if (CompanionId == FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false)) return EmotionId == TEXT("fired_up") || EmotionId == TEXT("frustrated") || EmotionId == TEXT("relieved");
		return CompanionId == FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false) && (EmotionId == TEXT("sarcastic") || EmotionId == TEXT("sarcastic_worried") || EmotionId == TEXT("sincere"));
	}

	static void NormalizeSummary(FPRMemorySummary& Value)
	{
		Value.SummarySequence = FMath::Max<int64>(0, Value.SummarySequence);
		Value.GraveyardOrdinal = FMath::Max<int64>(0, Value.GraveyardOrdinal);
		Value.MemoryFragmentsAwarded = FMath::Clamp(Value.MemoryFragmentsAwarded, 0, 1);
		Value.CompletedQuestIds.RemoveAll([](const FName Id) { return Id.IsNone(); });
		Value.CompletedQuestIds.Sort(FNameLexicalLess());
		RemoveDuplicateNames(Value.CompletedQuestIds);
		Value.CompletedQuestIds.SetNum(FMath::Min(Value.CompletedQuestIds.Num(), MaxCompletedQuestIds));
		Value.KeyEventIds.RemoveAll([](const FName Id) { return Id.IsNone(); });
		Value.KeyEventIds.Sort(FNameLexicalLess());
		RemoveDuplicateNames(Value.KeyEventIds);
		Value.KeyEventIds.SetNum(FMath::Min(Value.KeyEventIds.Num(), MaxKeyEventIds));
		Value.PlayerOptionIds.RemoveAll([](const FName Id) { return Id.IsNone(); });
		Value.ChoiceRefs.RemoveAll([](const FPRMemoryChoiceRef& Ref) { return Ref.SourceId.IsNone() || Ref.ContextId.IsNone() || Ref.ChoiceId.IsNone(); });
		Value.ChoiceRefs.Sort([](const FPRMemoryChoiceRef& Left, const FPRMemoryChoiceRef& Right)
		{
			return ChoiceRefKey(Left) < ChoiceRefKey(Right);
		});
		for (int32 Index = Value.ChoiceRefs.Num() - 1; Index > 0; --Index)
		{
			if (ChoiceRefKey(Value.ChoiceRefs[Index]) == ChoiceRefKey(Value.ChoiceRefs[Index - 1])) Value.ChoiceRefs.RemoveAt(Index);
		}
		Value.ChoiceRefs.SetNum(FMath::Min(Value.ChoiceRefs.Num(), MaxChoiceRefs));
	}

	static bool IsSummaryCanonical(const FPRMemorySummary& Value)
	{
		if (!Value.SummaryId.IsValid() || Value.GraveyardOrdinal <= 0 || Value.SummarySequence <= 0 || Value.SceneId != TEXT("post_run_summary") || !IsKnownCompanion(Value.CompanionId) || !IsKnownEmotion(Value.CompanionId, Value.EmotionId) || Value.SummaryText.IsEmpty() || CountUnicodeCodePoints(Value.SummaryText) > MaxSummaryCodePoints || !HasExactOptions(Value.CompanionId, Value.PlayerOptionIds) || Value.KeyEventIds.IsEmpty() || Value.KeyEventIds.Num() > MaxKeyEventIds || Value.CompletedQuestIds.Num() > MaxCompletedQuestIds || Value.DirectorRules.Num() > MaxDirectorRules || Value.QTEResults.Num() > MaxQTEResults || Value.ChoiceRefs.Num() > MaxChoiceRefs || (Value.MemoryFragmentsAwarded != 0 && Value.MemoryFragmentsAwarded != 1) || (Value.TerminationReason == EPRAccountTerminationReason::InterruptedRecovery && Value.MemoryFragmentsAwarded != 0) || (Value.bUsedFallback && Value.FallbackReasonId.IsNone())) return false;
		FString Previous;
		for (const FName Key : Value.KeyEventIds)
		{
			if (Key.IsNone() || (!Previous.IsEmpty() && Previous >= Key.ToString())) return false;
			Previous = Key.ToString();
		}
		FString PreviousQuest;
		for (const FName QuestId : Value.CompletedQuestIds)
		{
			if (QuestId.IsNone() || (!PreviousQuest.IsEmpty() && PreviousQuest >= QuestId.ToString())) return false;
			PreviousQuest = QuestId.ToString();
		}
		FString PreviousChoice;
		for (const FPRMemoryChoiceRef& Choice : Value.ChoiceRefs)
		{
			const FString CurrentKey = ChoiceRefKey(Choice);
			if (CurrentKey.IsEmpty() || (!PreviousChoice.IsEmpty() && PreviousChoice >= CurrentKey)) return false;
			PreviousChoice = CurrentKey;
		}
		return true;
	}

	static FString ChoiceRefKey(const FPRMemoryChoiceRef& Value)
	{
		return Value.SourceId.IsNone() || Value.ContextId.IsNone() || Value.ChoiceId.IsNone()
			? FString()
			: Value.SourceId.ToString() + TEXT("|") + Value.ContextId.ToString() + TEXT("|") + Value.ChoiceId.ToString();
	}

	static bool HasExactOptions(const FGameplayTag CompanionId, const TArray<FName>& Options)
	{
		if (Options.Num() != 3) return false;
		static const FName Axiom[] = { TEXT("axiom_reflect"), TEXT("axiom_challenge"), TEXT("axiom_deflect") };
		static const FName Kindle[] = { TEXT("kindle_steady"), TEXT("kindle_critique"), TEXT("kindle_thank") };
		static const FName Null[] = { TEXT("null_promise"), TEXT("null_callout"), TEXT("null_analyze") };
		const FName* Expected = CompanionId == FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false) ? Axiom : CompanionId == FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false) ? Kindle : Null;
		return Options[0] == Expected[0] && Options[1] == Expected[1] && Options[2] == Expected[2];
	}

	static int32 CountUnicodeCodePoints(const FString& Value)
	{
		int32 Count = 0;
		for (int32 Index = 0; Index < Value.Len(); ++Index)
		{
			const uint16 CodeUnit = Value[Index];
			if (CodeUnit >= 0xD800 && CodeUnit <= 0xDBFF && Index + 1 < Value.Len())
			{
				const uint16 Next = Value[Index + 1];
				if (Next >= 0xDC00 && Next <= 0xDFFF) ++Index;
			}
			++Count;
		}
		return Count;
	}

	static void RemoveDuplicateNames(TArray<FName>& Values)
	{
		for (int32 Index = Values.Num() - 1; Index > 0; --Index)
		{
			if (Values[Index] == Values[Index - 1]) Values.RemoveAt(Index);
		}
	}
};
