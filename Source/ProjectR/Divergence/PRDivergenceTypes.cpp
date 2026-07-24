// Copyright Epic Games, Inc. All Rights Reserved.

#include "Divergence/PRDivergenceTypes.h"

namespace
{
bool SetDelta(
	const FGameplayTag CompanionId,
	const EPRDivergenceChoice Choice,
	const int32 Trust,
	const int32 Affection,
	const int32 Evaluation,
	const int32 Overload,
	FPRRelationshipDelta& OutDelta)
{
	if (!CompanionId.IsValid() || Choice == EPRDivergenceChoice::None)
	{
		return false;
	}
	OutDelta = FPRRelationshipDelta();
	OutDelta.CompanionId = CompanionId;
	OutDelta.TrustDelta = Trust;
	OutDelta.AffectionDelta = Affection;
	OutDelta.EvaluationDelta = Evaluation;
	OutDelta.OverloadDelta = Overload;
	switch (Choice)
	{
	case EPRDivergenceChoice::Rescue: OutDelta.SourceId = TEXT("Divergence.Rescue"); break;
	case EPRDivergenceChoice::Leave: OutDelta.SourceId = TEXT("Divergence.Leave"); break;
	case EPRDivergenceChoice::FaceChallenge: OutDelta.SourceId = TEXT("Divergence.FaceChallenge"); break;
	default: return false;
	}
	return true;
}
}

bool FPRDivergenceContract::IsEligible(const FPRDivergenceEligibilityInput& Input)
{
	return Input.bAuthority
		&& Input.bCurrentPlayerDead
		&& Input.bHasPrimaryCompanion
		&& Input.bHasLoadedProfile
		&& !Input.bRunProtectionConsumed
		&& Input.Trust >= MinimumTrust
		&& Input.Overload < MaximumOverloadExclusive;
}

bool FPRDivergenceContract::GetFixedRelationshipDelta(
	const FGameplayTag CompanionId,
	const EPRDivergenceChoice Choice,
	FPRRelationshipDelta& OutDelta)
{
	OutDelta = FPRRelationshipDelta();
	if (CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag()))
	{
		switch (Choice)
		{
		case EPRDivergenceChoice::Rescue: return SetDelta(CompanionId, Choice, 2, 1, 1, 20, OutDelta);
		case EPRDivergenceChoice::Leave: return SetDelta(CompanionId, Choice, -1, -1, 1, 0, OutDelta);
		case EPRDivergenceChoice::FaceChallenge: return SetDelta(CompanionId, Choice, 0, 0, 2, 0, OutDelta);
		default: return false;
		}
	}
	if (CompanionId.MatchesTagExact(FPRCompanionContract::KindleTag()))
	{
		switch (Choice)
		{
		case EPRDivergenceChoice::Rescue: return SetDelta(CompanionId, Choice, 2, 2, 0, 25, OutDelta);
		case EPRDivergenceChoice::Leave: return SetDelta(CompanionId, Choice, -2, -1, 0, 0, OutDelta);
		case EPRDivergenceChoice::FaceChallenge: return SetDelta(CompanionId, Choice, 2, 1, 2, 0, OutDelta);
		default: return false;
		}
	}
	if (CompanionId.MatchesTagExact(FPRCompanionContract::NullTag()))
	{
		switch (Choice)
		{
		case EPRDivergenceChoice::Rescue: return SetDelta(CompanionId, Choice, 1, 2, 1, 20, OutDelta);
		case EPRDivergenceChoice::Leave: return SetDelta(CompanionId, Choice, -1, -2, 1, 0, OutDelta);
		case EPRDivergenceChoice::FaceChallenge: return SetDelta(CompanionId, Choice, 1, 1, 2, 0, OutDelta);
		default: return false;
		}
	}
	return false;
}

FText FPRDivergenceContract::GetCompanionSpeaker(const FGameplayTag CompanionId)
{
	if (CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag())) return FText::FromString(TEXT("Axiom"));
	if (CompanionId.MatchesTagExact(FPRCompanionContract::KindleTag())) return FText::FromString(TEXT("Kindle"));
	if (CompanionId.MatchesTagExact(FPRCompanionContract::NullTag())) return FText::FromString(TEXT("Null"));
	return FText::GetEmpty();
}

FText FPRDivergenceContract::GetCompanionPrompt(const FGameplayTag CompanionId)
{
	if (CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag())) return FText::FromString(TEXT("存续概率仍可修正。接受挽救、主动离开，或继续证明。"));
	if (CompanionId.MatchesTagExact(FPRCompanionContract::KindleTag())) return FText::FromString(TEXT("还没结束！让我拉你回来、现在撤，还是带着这点血继续冲？"));
	if (CompanionId.MatchesTagExact(FPRCompanionContract::NullTag())) return FText::FromString(TEXT("删除流程卡住了。让我救、自己走，还是拿这点命继续赌？"));
	return FText::GetEmpty();
}

FText FPRDivergenceContract::GetChoiceText(const EPRDivergenceChoice Choice)
{
	switch (Choice)
	{
	case EPRDivergenceChoice::Rescue: return FText::FromString(TEXT("挽救"));
	case EPRDivergenceChoice::Leave: return FText::FromString(TEXT("直接离开"));
	case EPRDivergenceChoice::FaceChallenge: return FText::FromString(TEXT("直面挑战"));
	default: return FText::GetEmpty();
	}
}

FText FPRDivergenceContract::GetChoiceInputHint(const EPRDivergenceChoice Choice)
{
	switch (Choice)
	{
	case EPRDivergenceChoice::Rescue: return FText::FromString(TEXT("E / Interact"));
	case EPRDivergenceChoice::Leave: return FText::FromString(TEXT("R / QTE Reject"));
	case EPRDivergenceChoice::FaceChallenge: return FText::FromString(TEXT("F / Execute"));
	default: return FText::GetEmpty();
	}
}

EPRDivergenceFutureDisposition FPRDivergenceContract::GetFutureDisposition(const EPRDivergenceChoice Choice)
{
	switch (Choice)
	{
	case EPRDivergenceChoice::Rescue: return EPRDivergenceFutureDisposition::RescueEvacuationRequested;
	case EPRDivergenceChoice::Leave: return EPRDivergenceFutureDisposition::LeaveRunRequested;
	case EPRDivergenceChoice::FaceChallenge: return EPRDivergenceFutureDisposition::ChallengeContinues;
	default: return EPRDivergenceFutureDisposition::None;
	}
}
