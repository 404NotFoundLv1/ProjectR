// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRMemoryHubWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UPRMemoryHubWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UButton* Button = Cast<UButton>(WidgetTree->FindWidget(TEXT("Button_MemorySummary")))) Button->OnClicked.AddDynamic(this, &UPRMemoryHubWidget::HandleMemorySummaryClicked);
	if (MemorySummaryPanel) MemorySummaryPanel->SetVisibility(ESlateVisibility::Collapsed);
}
void UPRMemoryHubWidget::NativeDestruct()
{
	if (UButton* Button = Cast<UButton>(WidgetTree->FindWidget(TEXT("Button_MemorySummary")))) Button->OnClicked.RemoveDynamic(this, &UPRMemoryHubWidget::HandleMemorySummaryClicked);
	Super::NativeDestruct();
}
void UPRMemoryHubWidget::HandleMemorySummaryClicked()
{
	if (MemorySummaryPanel) MemorySummaryPanel->SetVisibility(ESlateVisibility::Visible);
	if (UTextBlock* Status = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Text_Status")))) Status->SetText(FText::FromString(TEXT("Memory summary: archived facts and fixed options only. No relationship or progress change is available.")));
}
