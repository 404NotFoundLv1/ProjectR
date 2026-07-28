// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRRealityHubTrainingReturnWidget.h"

#include "Components/Button.h"
#include "RealityHub/PRRealityHubSubsystem.h"

void UPRRealityHubTrainingReturnWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_ReturnToRealityHub) Button_ReturnToRealityHub->OnClicked.AddDynamic(this, &UPRRealityHubTrainingReturnWidget::HandleReturnClicked);
}

void UPRRealityHubTrainingReturnWidget::HandleReturnClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPRRealityHubSubsystem* Hub = GameInstance->GetSubsystem<UPRRealityHubSubsystem>()) Hub->RequestReturnToRealityHub();
	}
}
