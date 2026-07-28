// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memory/PRMemorySummaryBuilder.h"

#include "Dialogue/PRDialogueTypes.h"
#include "Divergence/PRDivergenceTypes.h"
#include "Roguelike/Account/PRAccountRuntimeTypes.h"
#include "Roguelike/PRRoomTypes.h"

namespace PRMemorySummaryBuilderPrivate
{
	static FName AssetName(const FPrimaryAssetId& AssetId)
	{
		return AssetId.IsValid() ? AssetId.PrimaryAssetName : NAME_None;
	}

	static FGameplayTag SelectCompanion(const FPRAccountRecord& Record)
	{
		const FGameplayTag Primary = Record.Summary.PrimaryCompanionId;
		if (Primary == FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false) ||
			Primary == FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false) ||
			Primary == FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false)) return Primary;
		return FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false);
	}
}

void FPRMemorySummaryBuilder::Reset()
{
	ChoiceRefs.Reset();
	KeyEventIds.Reset();
	CompletedQuestIds.Reset();
}

void FPRMemorySummaryBuilder::AddBoundedUnique(TArray<FName>& Values, const FName Value, const int32 Maximum)
{
	if (!Value.IsNone() && !Values.Contains(Value) && Values.Num() < Maximum) Values.Add(Value);
}

void FPRMemorySummaryBuilder::AddChoice(TArray<FPRMemoryChoiceRef>& Values, const FName SourceId, const FName ContextId, const FName ChoiceId)
{
	if (SourceId.IsNone() || ContextId.IsNone() || ChoiceId.IsNone() || Values.Num() >= FPRMemoryPersistenceContract::MaxChoiceRefs) return;
	for (const FPRMemoryChoiceRef& Existing : Values)
	{
		if (Existing.SourceId == SourceId && Existing.ContextId == ContextId && Existing.ChoiceId == ChoiceId) return;
	}
	FPRMemoryChoiceRef& Added = Values.AddDefaulted_GetRef();
	Added.SourceId = SourceId;
	Added.ContextId = ContextId;
	Added.ChoiceId = ChoiceId;
}

void FPRMemorySummaryBuilder::RecordRoomEvent(const FPRRoomEventResult& Result)
{
	if (!Result.bChoiceApplied || !Result.ResolutionId.IsValid()) return;
	const FName Context = PRMemorySummaryBuilderPrivate::AssetName(Result.EventId);
	AddChoice(ChoiceRefs, TEXT("Room"), Context, Result.ChoiceId);
	AddBoundedUnique(KeyEventIds, TEXT("RoomEventApplied"), FPRMemoryPersistenceContract::MaxKeyEventIds);
}

void FPRMemorySummaryBuilder::RecordDialogueResult(const FPRDialogueResult& Result)
{
	if (Result.Resolution != EPRDialogueChoiceResolution::Applied || !Result.ResultId.IsValid()) return;
	AddChoice(ChoiceRefs, TEXT("Dialogue"), Result.CompanionId.GetTagName(), Result.ChoiceId);
	AddBoundedUnique(KeyEventIds, TEXT("DialogueChoiceApplied"), FPRMemoryPersistenceContract::MaxKeyEventIds);
}

void FPRMemorySummaryBuilder::RecordDivergenceResult(const FPRDivergenceResult& Result)
{
	if (Result.Resolution != EPRDivergenceResolution::Applied || !Result.ResultId.IsValid()) return;
	AddChoice(ChoiceRefs, TEXT("Divergence"), Result.CompanionId.GetTagName(), Result.DialogueChoiceId);
	AddBoundedUnique(KeyEventIds, TEXT("DivergenceApplied"), FPRMemoryPersistenceContract::MaxKeyEventIds);
}

void FPRMemorySummaryBuilder::SetCompletedQuestIds(const TArray<FName>& InCompletedQuestIds)
{
	CompletedQuestIds.Reset();
	for (const FName QuestId : InCompletedQuestIds) AddBoundedUnique(CompletedQuestIds, QuestId, FPRMemoryPersistenceContract::MaxCompletedQuestIds);
}

bool FPRMemorySummaryBuilder::Build(const FPRAccountRecord& Record, FPRMemorySummary& OutSummary) const
{
	if (!Record.RecordId.IsValid() || Record.GraveyardOrdinal <= 0) return false;
	OutSummary = FPRMemorySummary();
	OutSummary.SummaryId = Record.RecordId;
	OutSummary.TerminationReason = Record.TerminationReason;
	OutSummary.DeathCauseId = Record.Summary.DeathCause.SourceId;
	OutSummary.DirectorRules = Record.Summary.DirectorRules;
	OutSummary.DirectorRules.SetNum(FMath::Min(OutSummary.DirectorRules.Num(), FPRMemoryPersistenceContract::MaxDirectorRules));
	OutSummary.QTEResults = Record.Summary.QTEResults;
	OutSummary.QTEResults.SetNum(FMath::Min(OutSummary.QTEResults.Num(), FPRMemoryPersistenceContract::MaxQTEResults));
	OutSummary.ChoiceRefs = ChoiceRefs;
	OutSummary.CompletedQuestIds = CompletedQuestIds;
	OutSummary.KeyEventIds = KeyEventIds;
	AddBoundedUnique(OutSummary.KeyEventIds, FName(*FString::Printf(TEXT("Termination.%s"), *StaticEnum<EPRAccountTerminationReason>()->GetNameStringByValue(static_cast<int64>(Record.TerminationReason)))), FPRMemoryPersistenceContract::MaxKeyEventIds);
	OutSummary.CompanionId = PRMemorySummaryBuilderPrivate::SelectCompanion(Record);
	OutSummary.SceneId = TEXT("post_run_summary");
	OutSummary.GraveyardOrdinal = Record.GraveyardOrdinal;
	return true;
}
