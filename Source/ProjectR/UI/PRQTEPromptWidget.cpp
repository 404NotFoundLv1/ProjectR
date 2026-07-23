// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRQTEPromptWidget.h"

#include "Components/TextBlock.h"
#include "Core/PRTagLibrary.h"

#define LOCTEXT_NAMESPACE "PRQTEPromptWidget"

FText UPRQTEPromptWidget::FormatAcceptedInputTags(const FGameplayTagContainer& InputTags)
{
	TArray<FString> Prompts;
	for (const FGameplayTag& InputTag : InputTags)
	{
		if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputAttackTag())) Prompts.Add(TEXT("J / LMB"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputDodgeTag())) Prompts.Add(TEXT("L / Left Shift"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputInteractTag())) Prompts.Add(TEXT("E"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputExecuteTag())) Prompts.Add(TEXT("F"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputSkillShadowThrustTag())) Prompts.Add(TEXT("U"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputSkillFireSlashTag())) Prompts.Add(TEXT("I"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputSkillThunderDropTag())) Prompts.Add(TEXT("O"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputSkillVectorHookTag())) Prompts.Add(TEXT("H"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputSkillCounterProofWallTag())) Prompts.Add(TEXT("Semicolon"));
		else if (InputTag.MatchesTagExact(UPRTagLibrary::GetInputQTERejectTag())) Prompts.Add(TEXT("R"));
		else Prompts.Add(InputTag.ToString());
	}
	return FText::FromString(FString::Join(Prompts, TEXT(" / ")));
}

void UPRQTEPromptWidget::ApplyRuntimeState(const FPRQTERuntimeState& State)
{
	if (QTENameText) QTENameText->SetText(State.DisplayName.IsEmpty() ? FText::FromName(State.QTEId.PrimaryAssetName) : State.DisplayName);
	if (PromptText) PromptText->SetText(State.PromptText.IsEmpty() ? FText::FromString(TEXT("QTE")) : State.PromptText);
	if (InputText) InputText->SetText(FText::Format(LOCTEXT("PressPrompt", "Press {0}"), FormatAcceptedInputTags(State.AcceptedInputTags)));
	if (TimeText) TimeText->SetText(FText::AsNumber(FMath::Max(0.0f, State.RemainingSeconds)));
}

#undef LOCTEXT_NAMESPACE
