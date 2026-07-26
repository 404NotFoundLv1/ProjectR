// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Director/PRDirectorTypes.h"

#include "PRDirectorRulePanelWidget.generated.h"

class UTextBlock;

/** Read-only presentation of validated Director Rule runtime state. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRDirectorRulePanelWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RuleNameText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RuleReasonText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RuleEffectText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RuleCounterText;

private:
	void HandleRuleRuntimeChanged(const FPRDirectorRuleRuntimeState& State);
	void Refresh(const FPRDirectorRuleRuntimeState* State);
	FDelegateHandle RuleRuntimeChangedHandle;
};
