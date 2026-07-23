// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/PRRelationshipTypes.h"

namespace PRRelationshipPrivate
{
const FGameplayTag Axiom = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false);
const FGameplayTag Kindle = FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false);
const FGameplayTag Null = FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false);

const TArray<FGameplayTag> CanonicalIds = { Axiom, Kindle, Null };

int32 ClampValue(const int64 Value)
{
	return static_cast<int32>(FMath::Clamp<int64>(
		Value,
		FPRCompanionContract::MinimumRelationshipValue,
		FPRCompanionContract::MaximumRelationshipValue));
}
}

const FGameplayTag& FPRCompanionContract::AxiomTag()
{
	return PRRelationshipPrivate::Axiom;
}

const FGameplayTag& FPRCompanionContract::KindleTag()
{
	return PRRelationshipPrivate::Kindle;
}

const FGameplayTag& FPRCompanionContract::NullTag()
{
	return PRRelationshipPrivate::Null;
}

const TArray<FGameplayTag>& FPRCompanionContract::GetCanonicalCompanionIds()
{
	return PRRelationshipPrivate::CanonicalIds;
}

TArray<FPRCompanionRelationshipRecord> FPRCompanionContract::BuildDefaultRelationshipRecords()
{
	TArray<FPRCompanionRelationshipRecord> Records;
	for (const FGameplayTag& Id : GetCanonicalCompanionIds())
	{
		FPRCompanionRelationshipRecord& Record = Records.AddDefaulted_GetRef();
		Record.CompanionId = Id;
	}
	return Records;
}

bool FPRCompanionContract::AreCanonicalRelationshipRecords(const TArray<FPRCompanionRelationshipRecord>& Records)
{
	const TArray<FGameplayTag>& CanonicalIds = GetCanonicalCompanionIds();
	if (Records.Num() != CanonicalIds.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < CanonicalIds.Num(); ++Index)
	{
		if (!Records[Index].CompanionId.MatchesTagExact(CanonicalIds[Index]) || !IsStateInRange(Records[Index].State))
		{
			return false;
		}
	}
	return true;
}

bool FPRCompanionContract::ApplyDelta(FPRRelationshipState& InOutState, const FPRRelationshipDelta& Delta)
{
	if (!Delta.CompanionId.IsValid() || Delta.SourceId.IsNone())
	{
		return false;
	}
	InOutState.Trust = PRRelationshipPrivate::ClampValue(static_cast<int64>(InOutState.Trust) + Delta.TrustDelta);
	InOutState.Affection = PRRelationshipPrivate::ClampValue(static_cast<int64>(InOutState.Affection) + Delta.AffectionDelta);
	InOutState.Evaluation = PRRelationshipPrivate::ClampValue(static_cast<int64>(InOutState.Evaluation) + Delta.EvaluationDelta);
	InOutState.Overload = PRRelationshipPrivate::ClampValue(static_cast<int64>(InOutState.Overload) + Delta.OverloadDelta);
	return true;
}

bool FPRCompanionContract::IsStateInRange(const FPRRelationshipState& State)
{
	return State.Trust >= MinimumRelationshipValue && State.Trust <= MaximumRelationshipValue
		&& State.Affection >= MinimumRelationshipValue && State.Affection <= MaximumRelationshipValue
		&& State.Evaluation >= MinimumRelationshipValue && State.Evaluation <= MaximumRelationshipValue
		&& State.Overload >= MinimumRelationshipValue && State.Overload <= MaximumRelationshipValue;
}
