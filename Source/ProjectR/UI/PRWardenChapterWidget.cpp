// Copyright ProjectR. All Rights Reserved.

#include "UI/PRWardenChapterWidget.h"

#include "Chapters/PRChapterSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UPRWardenChapterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!StoryText && WidgetTree)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WardenOverlayRoot"));
		StoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StoryText"));
		WidgetTree->RootWidget = Root;
		if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(StoryText))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.0f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			CanvasSlot->SetPosition(FVector2D(0.0f, 72.0f));
		}
	}
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPRChapterSubsystem* Subsystem = GameInstance->GetSubsystem<UPRChapterSubsystem>())
		{
			ChapterSubsystem = Subsystem;
			Subsystem->GetSnapshot(Snapshot);
			StateChangedHandle = Subsystem->OnStateChanged().AddUObject(this, &UPRWardenChapterWidget::HandleChapterStateChanged);
		}
	}
	RefreshPresentation();
}

void UPRWardenChapterWidget::NativeDestruct()
{
	if (UPRChapterSubsystem* Subsystem = ChapterSubsystem.Get()) Subsystem->OnStateChanged().Remove(StateChangedHandle);
	StateChangedHandle.Reset();
	ChapterSubsystem.Reset();
	Snapshot = FPRChapterSnapshot();
	Super::NativeDestruct();
}

const FPRChapterSnapshot& UPRWardenChapterWidget::GetChapterSnapshot() const { return Snapshot; }
void UPRWardenChapterWidget::HandleChapterStateChanged(const FPRChapterSnapshot& InSnapshot) { Snapshot = InSnapshot; RefreshPresentation(); }

void UPRWardenChapterWidget::RefreshPresentation()
{
	if (!StoryText) return;
	if (Snapshot.WardenStory.bAvailable)
	{
		StoryText->SetText(Snapshot.WardenStory.Text);
		return;
	}
	const FText Fallback = Snapshot.WardenStory.FallbackReason.IsNone()
		? FText::FromString(TEXT("守夜者：剧情投影不可用。"))
		: FText::FromString(FString::Printf(TEXT("守夜者：%s"), *Snapshot.WardenStory.FallbackReason.ToString()));
	StoryText->SetText(Fallback);
}
