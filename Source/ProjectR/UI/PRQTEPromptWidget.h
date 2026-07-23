// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "QTE/PRQTETypes.h"

#include "PRQTEPromptWidget.generated.h"

class UTextBlock;

/** Read-only QTE presentation. It has no combat, relationship, timer, or Save authority. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRQTEPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ApplyRuntimeState(const FPRQTERuntimeState& State);
	/** Maps only the frozen player semantic inputs to a concise local keyboard prompt. */
	static FText FormatAcceptedInputTags(const FGameplayTagContainer& InputTags);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> QTENameText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> InputText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TimeText;
};
