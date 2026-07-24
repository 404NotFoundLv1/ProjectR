// Copyright Epic Games, Inc. All Rights Reserved.

#include "Divergence/PRDivergenceDataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId UPRDivergenceDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ProjectRDivergence"), GetFName());
}

bool UPRDivergenceDataAsset::ValidateDefinition(FString& OutError) const
{
	OutError.Reset();
	if (MinimumTrust != FPRDivergenceContract::MinimumTrust
		|| MaximumOverloadExclusive != FPRDivergenceContract::MaximumOverloadExclusive
		|| !FMath::IsNearlyEqual(ChoiceWindowSeconds, FPRDivergenceContract::ChoiceWindowSeconds)
		|| !FMath::IsNearlyEqual(RescueHealthFraction, FPRDivergenceContract::RescueHealthFraction)
		|| !FMath::IsNearlyEqual(ChallengeHealthFraction, FPRDivergenceContract::ChallengeHealthFraction)
		|| !FMath::IsNearlyZero(ReviveShieldFraction))
	{
		OutError = TEXT("Divergence thresholds or revive fractions differ from the frozen v0.3.4 contract.");
		return false;
	}
	const TArray<FGameplayTag>& ExpectedCompanions = FPRCompanionContract::GetCanonicalCompanionIds();
	if (Presentations.Num() != ExpectedCompanions.Num())
	{
		OutError = TEXT("Divergence requires exactly the three canonical companion presentations.");
		return false;
	}
	const EPRDivergenceChoice ExpectedChoices[] = {
		EPRDivergenceChoice::Rescue,
		EPRDivergenceChoice::Leave,
		EPRDivergenceChoice::FaceChallenge };
	for (int32 PresentationIndex = 0; PresentationIndex < Presentations.Num(); ++PresentationIndex)
	{
		const FPRDivergencePresentationDefinition& Presentation = Presentations[PresentationIndex];
		if (!Presentation.CompanionId.MatchesTagExact(ExpectedCompanions[PresentationIndex])
			|| Presentation.SpeakerText.ToString() != FPRDivergenceContract::GetCompanionSpeaker(Presentation.CompanionId).ToString()
			|| Presentation.PromptText.ToString() != FPRDivergenceContract::GetCompanionPrompt(Presentation.CompanionId).ToString()
			|| Presentation.Choices.Num() != UE_ARRAY_COUNT(ExpectedChoices))
		{
			OutError = TEXT("Divergence companion presentation differs from the canonical order, speaker, prompt, or choice count.");
			return false;
		}
		for (int32 ChoiceIndex = 0; ChoiceIndex < Presentation.Choices.Num(); ++ChoiceIndex)
		{
			const FPRDivergenceChoiceDefinition& Choice = Presentation.Choices[ChoiceIndex];
			FPRRelationshipDelta ExpectedDelta;
			if (Choice.Choice != ExpectedChoices[ChoiceIndex]
				|| Choice.DisplayText.ToString() != FPRDivergenceContract::GetChoiceText(Choice.Choice).ToString()
				|| !FPRDivergenceContract::GetFixedRelationshipDelta(Presentation.CompanionId, Choice.Choice, ExpectedDelta)
				|| Choice.RelationshipDelta.CompanionId != ExpectedDelta.CompanionId
				|| Choice.RelationshipDelta.TrustDelta != ExpectedDelta.TrustDelta
				|| Choice.RelationshipDelta.AffectionDelta != ExpectedDelta.AffectionDelta
				|| Choice.RelationshipDelta.EvaluationDelta != ExpectedDelta.EvaluationDelta
				|| Choice.RelationshipDelta.OverloadDelta != ExpectedDelta.OverloadDelta
				|| Choice.RelationshipDelta.SourceId != ExpectedDelta.SourceId)
			{
				OutError = TEXT("Divergence choice definition differs from the frozen relationship contract.");
				return false;
			}
		}
	}
	return true;
}

const FPRDivergencePresentationDefinition* UPRDivergenceDataAsset::FindPresentation(const FGameplayTag CompanionId) const
{
	return Presentations.FindByPredicate([CompanionId](const FPRDivergencePresentationDefinition& Candidate)
	{
		return Candidate.CompanionId.MatchesTagExact(CompanionId);
	});
}

#if WITH_EDITOR
EDataValidationResult UPRDivergenceDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	FString Error;
	if (!ValidateDefinition(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}
	if (WidgetClass.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("Divergence WidgetClass must point to WBP_DivergenceCache.")));
		return EDataValidationResult::Invalid;
	}
	return EDataValidationResult::Valid;
}
#endif

void FPRDivergenceContract::ConfigureFixedDefinition(UPRDivergenceDataAsset& Asset)
{
	Asset.MinimumTrust = MinimumTrust;
	Asset.MaximumOverloadExclusive = MaximumOverloadExclusive;
	Asset.ChoiceWindowSeconds = ChoiceWindowSeconds;
	Asset.RescueHealthFraction = RescueHealthFraction;
	Asset.ChallengeHealthFraction = ChallengeHealthFraction;
	Asset.ReviveShieldFraction = ReviveShieldFraction;
	Asset.Presentations.Reset();
	for (const FGameplayTag& CompanionId : FPRCompanionContract::GetCanonicalCompanionIds())
	{
		FPRDivergencePresentationDefinition& Presentation = Asset.Presentations.AddDefaulted_GetRef();
		Presentation.CompanionId = CompanionId;
		Presentation.SpeakerText = GetCompanionSpeaker(CompanionId);
		Presentation.PromptText = GetCompanionPrompt(CompanionId);
		for (const EPRDivergenceChoice Choice : { EPRDivergenceChoice::Rescue, EPRDivergenceChoice::Leave, EPRDivergenceChoice::FaceChallenge })
		{
			FPRDivergenceChoiceDefinition& Definition = Presentation.Choices.AddDefaulted_GetRef();
			Definition.Choice = Choice;
			Definition.DisplayText = GetChoiceText(Choice);
			GetFixedRelationshipDelta(CompanionId, Choice, Definition.RelationshipDelta);
		}
	}
}
