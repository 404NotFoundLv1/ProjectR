// Copyright ProjectR. All Rights Reserved.

#include "UI/PRPacifierChapterWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UPRPacifierChapterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!StoryText && WidgetTree)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PacifierOverlayRoot"));
		StoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StoryText"));
		WidgetTree->RootWidget = Root;
		if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(StoryText))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.0f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			CanvasSlot->SetPosition(FVector2D(0.0f, 104.0f));
		}
	}
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPRChapterSubsystem* Subsystem = GameInstance->GetSubsystem<UPRChapterSubsystem>())
		{
			ChapterSubsystem = Subsystem;
			Subsystem->GetSnapshot(Snapshot);
			StateChangedHandle = Subsystem->OnStateChanged().AddUObject(this, &UPRPacifierChapterWidget::HandleChapterStateChanged);
		}
	}
	RefreshPresentation();
}

void UPRPacifierChapterWidget::NativeDestruct()
{
	if (UPRChapterSubsystem* Subsystem = ChapterSubsystem.Get()) Subsystem->OnStateChanged().Remove(StateChangedHandle);
	StateChangedHandle.Reset();
	ChapterSubsystem.Reset();
	Snapshot = FPRChapterSnapshot();
	Super::NativeDestruct();
}

const FPRChapterSnapshot& UPRPacifierChapterWidget::GetChapterSnapshot() const { return Snapshot; }
void UPRPacifierChapterWidget::HandleChapterStateChanged(const FPRChapterSnapshot& InSnapshot) { Snapshot = InSnapshot; RefreshPresentation(); }

void UPRPacifierChapterWidget::RefreshPresentation()
{
	if (!StoryText) return;
	const FText Story = Snapshot.PacifierStory.bAvailable
		? Snapshot.PacifierStory.Text
		: Snapshot.PacifierStory.FallbackReason.IsNone()
		? FText::FromString(TEXT("抚慰者：剧情投影不可用。"))
		: FText::FromString(FString::Printf(TEXT("抚慰者：%s"), *Snapshot.PacifierStory.FallbackReason.ToString()));
	StoryText->SetText(FText::FromString(FString::Printf(
		TEXT("抚慰者 | 舒适压力 %d/4 | %s\n%s"),
		Snapshot.ComfortPressure,
		*Snapshot.DirectiveId.ToString(),
		*Story.ToString())));
}
