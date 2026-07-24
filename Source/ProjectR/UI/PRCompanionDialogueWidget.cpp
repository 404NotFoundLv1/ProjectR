// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRCompanionDialogueWidget.h"

#include "Components/TextBlock.h"

void UPRCompanionDialogueWidget::ApplyRuntimeState(const FPRDialogueRuntimeState& State)
{
	if (SpeakerText) SpeakerText->SetText(State.SpeakerText);
	if (DialogueText) DialogueText->SetText(State.DialogueText);
	const bool bChoice = State.PresentationState == EPRDialoguePresentationState::Choice && State.Choices.Num() == 2;
	if (ChoiceAcknowledgeText) ChoiceAcknowledgeText->SetText(bChoice ? State.Choices[0].Text : FText::GetEmpty());
	if (ChoiceAnalyzeText) ChoiceAnalyzeText->SetText(bChoice ? State.Choices[1].Text : FText::GetEmpty());
	if (ChoiceHintText) ChoiceHintText->SetText(bChoice ? FText::FromString(TEXT("E: acknowledge   F: analyze")) : FText::GetEmpty());
}
