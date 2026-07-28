// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memory/PRPostRunDialogueValidator.h"

namespace PRPostRunDialogueValidatorPrivate
{
	static const FName SceneId(TEXT("post_run_summary"));
	static const FName WireFields[] = {
		TEXT("scene"), TEXT("companion_id"), TEXT("emotion"), TEXT("summary"), TEXT("player_options") };

	static bool IsDisallowedSummaryText(const FString& Value)
	{
		const FString Lower = Value.ToLower();
		static const TCHAR* Disallowed[] = {
			TEXT("http://"), TEXT("https://"), TEXT("www."), TEXT("<"), TEXT(">"), TEXT("{"), TEXT("}"),
			TEXT("system"), TEXT("assistant"), TEXT("ignore previous"), TEXT("prompt"), TEXT("instruction"),
			TEXT("api_key"), TEXT("token"), TEXT("profile"), TEXT("account"), TEXT("slot") };
		for (const TCHAR* Token : Disallowed)
		{
			if (Lower.Contains(Token)) return true;
		}
		return false;
	}

	static bool IsForbiddenCodeUnit(const TCHAR Character)
	{
		const uint32 CodeUnit = static_cast<uint32>(Character);
		return CodeUnit <= 0x1f || CodeUnit == 0x7f ||
			(CodeUnit >= 0x202a && CodeUnit <= 0x202e) ||
			(CodeUnit >= 0x2066 && CodeUnit <= 0x2069);
	}
}

bool FPRPostRunDialogueValidator::Validate(
	const FPRPostRunDialogueRequest& Request,
	const FPRPostRunDialogueCandidate& Candidate,
	const TArray<FName>& WireFieldNames,
	FPRPostRunDialogueResult& OutResult)
{
	OutResult = FPRPostRunDialogueResult();
	if (!Request.RequestId.IsValid() || !Request.SummaryId.IsValid() ||
		!HasExactWireFields(WireFieldNames) ||
		Candidate.SceneId != PRPostRunDialogueValidatorPrivate::SceneId ||
		Candidate.SceneId != Request.SceneId ||
		Candidate.CompanionId != Request.CompanionId ||
		!IsKnownCompanion(Candidate.CompanionId) ||
		!IsKnownEmotion(Candidate.CompanionId, Candidate.EmotionId) ||
		!HasExactOptions(Candidate.CompanionId, Candidate.PlayerOptionIds) ||
		!IsSafeSummary(Candidate.Summary))
	{
		return false;
	}

	OutResult.RequestId = Request.RequestId;
	OutResult.SceneId = Candidate.SceneId;
	OutResult.CompanionId = Candidate.CompanionId;
	OutResult.EmotionId = Candidate.EmotionId;
	OutResult.Summary = Candidate.Summary;
	OutResult.PlayerOptionIds = Candidate.PlayerOptionIds;
	return true;
}

bool FPRPostRunDialogueValidator::HasExactWireFields(const TArray<FName>& WireFieldNames)
{
	if (WireFieldNames.Num() != UE_ARRAY_COUNT(PRPostRunDialogueValidatorPrivate::WireFields)) return false;
	TSet<FName> Seen;
	for (const FName Field : WireFieldNames)
	{
		if (Seen.Contains(Field) || !TArrayView<const FName>(PRPostRunDialogueValidatorPrivate::WireFields).Contains(Field)) return false;
		Seen.Add(Field);
	}
	return true;
}

bool FPRPostRunDialogueValidator::IsKnownCompanion(const FName CompanionId)
{
	return CompanionId == TEXT("Axiom") || CompanionId == TEXT("Kindle") || CompanionId == TEXT("Null");
}

bool FPRPostRunDialogueValidator::IsKnownEmotion(const FName CompanionId, const FName EmotionId)
{
	if (CompanionId == TEXT("Axiom")) return EmotionId == TEXT("analytical") || EmotionId == TEXT("concerned") || EmotionId == TEXT("quietly_proud");
	if (CompanionId == TEXT("Kindle")) return EmotionId == TEXT("fired_up") || EmotionId == TEXT("frustrated") || EmotionId == TEXT("relieved");
	if (CompanionId == TEXT("Null")) return EmotionId == TEXT("sarcastic") || EmotionId == TEXT("sarcastic_worried") || EmotionId == TEXT("sincere");
	return false;
}

bool FPRPostRunDialogueValidator::HasExactOptions(const FName CompanionId, const TArray<FName>& OptionIds)
{
	static const FName Axiom[] = { TEXT("axiom_reflect"), TEXT("axiom_challenge"), TEXT("axiom_deflect") };
	static const FName Kindle[] = { TEXT("kindle_steady"), TEXT("kindle_critique"), TEXT("kindle_thank") };
	static const FName Null[] = { TEXT("null_promise"), TEXT("null_callout"), TEXT("null_analyze") };
	const TArrayView<const FName> Expected = CompanionId == TEXT("Axiom") ? TArrayView<const FName>(Axiom) :
		CompanionId == TEXT("Kindle") ? TArrayView<const FName>(Kindle) : TArrayView<const FName>(Null);
	if (OptionIds.Num() != Expected.Num()) return false;
	for (int32 Index = 0; Index < OptionIds.Num(); ++Index)
	{
		if (OptionIds[Index] != Expected[Index]) return false;
	}
	return true;
}

bool FPRPostRunDialogueValidator::IsSafeSummary(const FString& Summary)
{
	if (Summary.IsEmpty() || PRPostRunDialogueValidatorPrivate::IsDisallowedSummaryText(Summary)) return false;
	int32 CodePointCount = 0;
	for (int32 Index = 0; Index < Summary.Len(); ++Index)
	{
		const TCHAR Character = Summary[Index];
		if (PRPostRunDialogueValidatorPrivate::IsForbiddenCodeUnit(Character)) return false;
		const uint32 CodeUnit = static_cast<uint32>(Character);
		if (CodeUnit >= 0xd800 && CodeUnit <= 0xdbff)
		{
			if (++Index >= Summary.Len()) return false;
			const uint32 LowSurrogate = static_cast<uint32>(Summary[Index]);
			if (LowSurrogate < 0xdc00 || LowSurrogate > 0xdfff) return false;
		}
		else if (CodeUnit >= 0xdc00 && CodeUnit <= 0xdfff)
		{
			return false;
		}
		++CodePointCount;
	}
	return CodePointCount <= 240;
}
