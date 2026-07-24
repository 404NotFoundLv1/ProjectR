// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRDivergenceCacheWidget.h"

#include "Components/TextBlock.h"

void UPRDivergenceCacheWidget::ApplyRuntimeState(const FPRDivergenceRuntimeState& State)
{
	if (SpeakerText) SpeakerText->SetText(State.SpeakerText);
	if (PromptText) PromptText->SetText(State.PromptText);
	const auto FindChoiceText = [&State](const EPRDivergenceChoice Choice)
	{
		const FPRDivergenceChoicePresentation* Presentation = State.Choices.FindByPredicate([Choice](const FPRDivergenceChoicePresentation& Candidate)
		{
			return Candidate.Choice == Choice;
		});
		return Presentation ? Presentation->DisplayText : FText::GetEmpty();
	};
	if (RescueText) RescueText->SetText(FindChoiceText(EPRDivergenceChoice::Rescue));
	if (LeaveText) LeaveText->SetText(FindChoiceText(EPRDivergenceChoice::Leave));
	if (ChallengeText) ChallengeText->SetText(FindChoiceText(EPRDivergenceChoice::FaceChallenge));
	if (InputHintText) InputHintText->SetText(FText::FromString(TEXT("E: 挽救   R: 直接离开   F: 直面挑战")));
	if (CountdownText)
	{
		CountdownText->SetText(State.State == EPRDivergenceState::AwaitingChoice
			? FText::Format(NSLOCTEXT("ProjectR", "DivergenceWindow", "选择窗口：{0} 秒"), FText::AsNumber(FMath::RoundToInt(FPRDivergenceContract::ChoiceWindowSeconds)))
			: FText::GetEmpty());
	}
}
