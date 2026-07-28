// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRMemorySummaryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Memory/PRMemorySubsystem.h"

void UPRMemorySummaryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Button_MemoryOptionOne")))) Button->OnClicked.AddDynamic(this, &UPRMemorySummaryWidget::SubmitFirst);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Button_MemoryOptionTwo")))) Button->OnClicked.AddDynamic(this, &UPRMemorySummaryWidget::SubmitSecond);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Button_MemoryOptionThree")))) Button->OnClicked.AddDynamic(this, &UPRMemorySummaryWidget::SubmitThird);
	if (UPRMemorySubsystem* Memory = GetMemory()) { StateChangedHandle = Memory->OnStateChanged().AddUObject(this, &UPRMemorySummaryWidget::HandleStateChanged); OperationHandle = Memory->OnOperation().AddUObject(this, &UPRMemorySummaryWidget::HandleOperation); Memory->GetSnapshot(DisplayedSnapshot); RenderSnapshot(); }
}
void UPRMemorySummaryWidget::NativeDestruct()
{
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Button_MemoryOptionOne")))) Button->OnClicked.RemoveDynamic(this, &UPRMemorySummaryWidget::SubmitFirst);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Button_MemoryOptionTwo")))) Button->OnClicked.RemoveDynamic(this, &UPRMemorySummaryWidget::SubmitSecond);
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("Button_MemoryOptionThree")))) Button->OnClicked.RemoveDynamic(this, &UPRMemorySummaryWidget::SubmitThird);
	if (UPRMemorySubsystem* Memory = GetMemory()) { Memory->OnStateChanged().Remove(StateChangedHandle); Memory->OnOperation().Remove(OperationHandle); }
	StateChangedHandle.Reset(); OperationHandle.Reset();
	Super::NativeDestruct();
}
FPRMemorySnapshot UPRMemorySummaryWidget::GetDisplayedSnapshot() const { return DisplayedSnapshot; }
void UPRMemorySummaryWidget::SubmitFirst() { Submit(EPRMemoryPlayerOptionSlot::First); }
void UPRMemorySummaryWidget::SubmitSecond() { Submit(EPRMemoryPlayerOptionSlot::Second); }
void UPRMemorySummaryWidget::SubmitThird() { Submit(EPRMemoryPlayerOptionSlot::Third); }
void UPRMemorySummaryWidget::Submit(const EPRMemoryPlayerOptionSlot InOptionSlot) { if (UPRMemorySubsystem* Memory = GetMemory()) Memory->SubmitLatestPlayerOption(InOptionSlot); }
void UPRMemorySummaryWidget::HandleStateChanged(const FPRMemorySnapshot& NewSnapshot) { DisplayedSnapshot = NewSnapshot; RenderSnapshot(); }
void UPRMemorySummaryWidget::HandleOperation(const FPRMemoryOperationEvent& Event) { if (Text_MemoryStatus) Text_MemoryStatus->SetText(FText::FromString(Event.ReasonId.IsNone() ? TEXT("Memory operation completed.") : Event.ReasonId.ToString())); }
void UPRMemorySummaryWidget::RenderSnapshot()
{
	if (Text_MemoryStatus)
	{
		if (!DisplayedSnapshot.bHasLatestSummary)
		{
			Text_MemoryStatus->SetText(FText::FromString(TEXT("Memory Summary: no archived bounded record is available.")));
		}
		else
		{
			const FString Fallback = DisplayedSnapshot.LatestSummary.bUsedFallback
				? FString::Printf(TEXT("\nFallback: %s"), *DisplayedSnapshot.LatestSummary.FallbackReasonId.ToString()) : FString();
			Text_MemoryStatus->SetText(FText::FromString(DisplayedSnapshot.LatestSummary.SummaryText + Fallback));
		}
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FText Text = DisplayedSnapshot.LatestOptionDisplayTexts.IsValidIndex(Index)
			? DisplayedSnapshot.LatestOptionDisplayTexts[Index] : FText::FromString(TEXT("Unavailable"));
		SetOptionText(Index == 0 ? TEXT("Button_MemoryOptionOne") : Index == 1 ? TEXT("Button_MemoryOptionTwo") : TEXT("Button_MemoryOptionThree"), Text);
	}
	PresentMemorySnapshot(DisplayedSnapshot);
}
void UPRMemorySummaryWidget::SetOptionText(const FName ButtonName, const FText& Text)
{
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(ButtonName)))
	{
		if (UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0))) Label->SetText(Text);
	}
}
UPRMemorySubsystem* UPRMemorySummaryWidget::GetMemory() const { return GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRMemorySubsystem>() : nullptr; }
