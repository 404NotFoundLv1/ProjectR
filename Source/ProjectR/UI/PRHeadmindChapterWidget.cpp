// Copyright ProjectR. All Rights Reserved.

#include "UI/PRHeadmindChapterWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UPRHeadmindChapterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!ChapterText && WidgetTree)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HeadmindOverlayRoot"));
		ChapterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChapterText"));
		WidgetTree->RootWidget = Root;
		if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(ChapterText)) { CanvasSlot->SetAnchors(FAnchors(0.5f, 0.0f)); CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f)); CanvasSlot->SetPosition(FVector2D(0.0f, 104.0f)); }
	}
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UPRChapterSubsystem* Subsystem = GameInstance->GetSubsystem<UPRChapterSubsystem>())
		{
			ChapterSubsystem = Subsystem; Subsystem->GetSnapshot(Snapshot);
			StateChangedHandle = Subsystem->OnStateChanged().AddUObject(this, &UPRHeadmindChapterWidget::HandleChapterStateChanged);
		}
	}
	RefreshPresentation();
}

void UPRHeadmindChapterWidget::NativeDestruct()
{
	if (UPRChapterSubsystem* Subsystem = ChapterSubsystem.Get()) Subsystem->OnStateChanged().Remove(StateChangedHandle);
	StateChangedHandle.Reset(); ChapterSubsystem.Reset(); Snapshot = FPRChapterSnapshot();
	Super::NativeDestruct();
}

const FPRChapterSnapshot& UPRHeadmindChapterWidget::GetChapterSnapshot() const { return Snapshot; }
void UPRHeadmindChapterWidget::HandleChapterStateChanged(const FPRChapterSnapshot& InSnapshot) { Snapshot = InSnapshot; RefreshPresentation(); }

void UPRHeadmindChapterWidget::RefreshPresentation()
{
	if (!ChapterText) return;
	ChapterText->SetText(FText::FromString(FString::Printf(TEXT("HEADMIND | Synthesis pressure %d/4 | %s\nTriple Resonance: %d"), Snapshot.SynthesisPressure, *Snapshot.DirectiveId.ToString(), static_cast<int32>(Snapshot.HeadmindBoss.TripleResonance.State))));
}
