// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Dialogue/PRDialogueTypes.h"

#include "PRCompanionDialogueWidget.generated.h"

class UTextBlock;

/** Presentation-only companion dialogue widget. Choice authority remains in UPRCompanionDialogueSubsystem. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRCompanionDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ApplyRuntimeState(const FPRDialogueRuntimeState& State);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DialogueText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceAcknowledgeText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceAnalyzeText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceHintText;
};
