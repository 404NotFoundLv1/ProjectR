// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRBossPrototypeResultWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Enemies/Bosses/PRBossSubsystem.h"

const FPRPrototypeRunResult& UPRBossPrototypeResultWidget::GetPrototypeRunResult() const { return Result; }

void UPRBossPrototypeResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsurePresentation();
	if (UPRBossSubsystem* Bosses = GetWorld() ? GetWorld()->GetSubsystem<UPRBossSubsystem>() : nullptr)
	{
		CompletionHandle = Bosses->OnPrototypeRunCompleted().AddUObject(this, &UPRBossPrototypeResultWidget::HandlePrototypeRunCompleted);
	}
	RefreshPresentation();
}

void UPRBossPrototypeResultWidget::NativeDestruct()
{
	if (UPRBossSubsystem* Bosses = GetWorld() ? GetWorld()->GetSubsystem<UPRBossSubsystem>() : nullptr)
	{
		Bosses->OnPrototypeRunCompleted().Remove(CompletionHandle);
	}
	Super::NativeDestruct();
}

void UPRBossPrototypeResultWidget::HandlePrototypeRunCompleted(const FPRPrototypeRunResult& InResult)
{
	Result = InResult;
	RefreshPresentation();
	OnPrototypeRunCompletedPresentation();
}

void UPRBossPrototypeResultWidget::EnsurePresentation()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	ResultText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrototypeResultText"));
	WidgetTree->RootWidget = ResultText;
}

void UPRBossPrototypeResultWidget::RefreshPresentation()
{
	if (ResultText)
	{
		ResultText->SetText(FText::FromString(FString::Printf(TEXT("Prototype Complete: %d Counterproof Fragment"), Result.CounterproofFragmentsAwarded)));
	}
}
