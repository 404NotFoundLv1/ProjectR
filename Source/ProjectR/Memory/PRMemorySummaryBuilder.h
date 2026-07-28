// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRMemorySaveTypes.h"

struct FPRAccountRecord;
struct FPRRoomEventResult;
struct FPRDialogueResult;
struct FPRDivergenceResult;

/** Reduces public, already-validated gameplay results to the bounded Memory summary contract. */
class PROJECTR_API FPRMemorySummaryBuilder
{
public:
	void Reset();
	void RecordRoomEvent(const FPRRoomEventResult& Result);
	void RecordDialogueResult(const FPRDialogueResult& Result);
	void RecordDivergenceResult(const FPRDivergenceResult& Result);
	void SetCompletedQuestIds(const TArray<FName>& InCompletedQuestIds);
	bool Build(const FPRAccountRecord& Record, FPRMemorySummary& OutSummary) const;

private:
	static void AddBoundedUnique(TArray<FName>& Values, FName Value, int32 Maximum);
	static void AddChoice(TArray<FPRMemoryChoiceRef>& Values, FName SourceId, FName ContextId, FName ChoiceId);
	TArray<FPRMemoryChoiceRef> ChoiceRefs;
	TArray<FName> KeyEventIds;
	TArray<FName> CompletedQuestIds;
};
