// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRCombatHUD.h"

#include "Blueprint/UserWidget.h"
#include "UI/PRCombatHUDWidget.h"

UPRCombatHUDWidget* APRCombatHUD::GetCombatHUDWidget() const
{
	return CombatHUDWidget;
}

void APRCombatHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !CombatHUDWidgetClass)
	{
		return;
	}

	CombatHUDWidget = CreateWidget<UPRCombatHUDWidget>(PlayerController, CombatHUDWidgetClass);
	if (IsValid(CombatHUDWidget))
	{
		CombatHUDWidget->AddToPlayerScreen();
	}
}

void APRCombatHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(CombatHUDWidget))
	{
		CombatHUDWidget->RemoveFromParent();
	}
	CombatHUDWidget = nullptr;
	Super::EndPlay(EndPlayReason);
}
