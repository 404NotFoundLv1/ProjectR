// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UI/PRCompanionQuestHubWidget.h"

#include "PRMemoryHubWidget.generated.h"

class UWidget;

/** Narrow RealityHub extension: it reveals a read-only Memory panel without owning Memory logic. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRMemoryHubWidget : public UPRCompanionQuestHubWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	UFUNCTION() void HandleMemorySummaryClicked();
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> MemorySummaryPanel;
};
