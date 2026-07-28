// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRRealityHubHUD.h"

#include "UI/PRRealityHubWidget.h"

UPRRealityHubWidget* APRRealityHubHUD::GetRealityHubWidget() const { return RealityHubWidget; }

void APRRealityHubHUD::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !RealityHubWidgetClass) return;
	RealityHubWidget = CreateWidget<UPRRealityHubWidget>(PlayerController, RealityHubWidgetClass);
	if (IsValid(RealityHubWidget)) RealityHubWidget->AddToPlayerScreen();
}

void APRRealityHubHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(RealityHubWidget)) RealityHubWidget->RemoveFromParent();
	RealityHubWidget = nullptr;
	Super::EndPlay(EndPlayReason);
}
