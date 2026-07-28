// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Memory/PRMemoryTypes.h"

/** Rejects every provider candidate outside the fixed v0.5.2 five-field schema. */
class PROJECTR_API FPRPostRunDialogueValidator
{
public:
	static bool Validate(
		const FPRPostRunDialogueRequest& Request,
		const FPRPostRunDialogueCandidate& Candidate,
		const TArray<FName>& WireFieldNames,
		FPRPostRunDialogueResult& OutResult);

private:
	static bool HasExactWireFields(const TArray<FName>& WireFieldNames);
	static bool IsKnownCompanion(FName CompanionId);
	static bool IsKnownEmotion(FName CompanionId, FName EmotionId);
	static bool HasExactOptions(FName CompanionId, const TArray<FName>& OptionIds);
	static bool IsSafeSummary(const FString& Summary);
};
