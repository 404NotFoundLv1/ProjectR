// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"

#include "PRRealityHubTrainingReturnWidget.generated.h"

class UButton;

/** Transient return control shown only in the fixed CombatGym training route. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRRealityHubTrainingReturnWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

private:
	UFUNCTION() void HandleReturnClicked();
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_ReturnToRealityHub;
};
