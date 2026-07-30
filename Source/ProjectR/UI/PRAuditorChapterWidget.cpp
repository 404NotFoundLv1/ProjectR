// Copyright ProjectR. All Rights Reserved.

#include "UI/PRAuditorChapterWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UPRAuditorChapterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!StoryText && WidgetTree)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AuditorOverlayRoot"));
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
			StateChangedHandle = Subsystem->OnStateChanged().AddUObject(this, &UPRAuditorChapterWidget::HandleChapterStateChanged);
		}
	}
	RefreshPresentation();
}

void UPRAuditorChapterWidget::NativeDestruct()
{
	if (UPRChapterSubsystem* Subsystem = ChapterSubsystem.Get()) Subsystem->OnStateChanged().Remove(StateChangedHandle);
	StateChangedHandle.Reset();
	ChapterSubsystem.Reset();
	Snapshot = FPRChapterSnapshot();
	Super::NativeDestruct();
}

const FPRChapterSnapshot& UPRAuditorChapterWidget::GetChapterSnapshot() const { return Snapshot; }
void UPRAuditorChapterWidget::HandleChapterStateChanged(const FPRChapterSnapshot& InSnapshot) { Snapshot = InSnapshot; RefreshPresentation(); }

void UPRAuditorChapterWidget::RefreshPresentation()
{
	if (!StoryText) return;
	const FText Story = Snapshot.AuditorStory.bAvailable
		? Snapshot.AuditorStory.Text
		: FText::FromString(Snapshot.AuditorStory.FallbackReason.IsNone()
			? TEXT("Auditor story projection unavailable.")
			: Snapshot.AuditorStory.FallbackReason.ToString());
	StoryText->SetText(FText::FromString(FString::Printf(
		TEXT("AUDITOR | Audit pressure %d/4 | %s\n%s"),
		Snapshot.AuditPressure,
		*Snapshot.DirectiveId.ToString(),
		*Story.ToString())));
}
