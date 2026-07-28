// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "UI/PRRealityHubWidget.h"
#include "PRCompanionQuestHubWidget.generated.h"
class UWidget;
UCLASS(Abstract)
class PROJECTR_API UPRCompanionQuestHubWidget : public UPRRealityHubWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	UFUNCTION() void HandleQuestTerminalClicked();
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> CompanionQuestTerminal;
};
