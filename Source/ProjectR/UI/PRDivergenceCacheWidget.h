// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Divergence/PRDivergenceTypes.h"

#include "PRDivergenceCacheWidget.generated.h"

class UTextBlock;

/** Presentation-only near-death choice widget. Gameplay remains in UPRDivergenceSubsystem. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRDivergenceCacheWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ApplyRuntimeState(const FPRDivergenceRuntimeState& State);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RescueText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> LeaveText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChallengeText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> InputHintText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CountdownText;
};
