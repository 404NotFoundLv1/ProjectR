// Copyright Epic Games, Inc. All Rights Reserved.
#include "UI/PRCompanionQuestHubWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
void UPRCompanionQuestHubWidget::NativeConstruct() { Super::NativeConstruct(); if (UButton* Button = Cast<UButton>(WidgetTree->FindWidget(TEXT("Button_Companion")))) Button->OnClicked.AddDynamic(this, &UPRCompanionQuestHubWidget::HandleQuestTerminalClicked); if (CompanionQuestTerminal) CompanionQuestTerminal->SetVisibility(ESlateVisibility::Collapsed); }
void UPRCompanionQuestHubWidget::NativeDestruct() { if (UButton* Button = Cast<UButton>(WidgetTree->FindWidget(TEXT("Button_Companion")))) Button->OnClicked.RemoveDynamic(this, &UPRCompanionQuestHubWidget::HandleQuestTerminalClicked); Super::NativeDestruct(); }
void UPRCompanionQuestHubWidget::HandleQuestTerminalClicked()
{
	if (CompanionQuestTerminal)
	{
		CompanionQuestTerminal->SetVisibility(ESlateVisibility::Visible);
	}
	if (UTextBlock* Status = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Text_Status"))))
	{
		Status->SetText(FText::FromString(TEXT("Companion quest terminal ready. Select an eligible fixed quest.")));
	}
}
