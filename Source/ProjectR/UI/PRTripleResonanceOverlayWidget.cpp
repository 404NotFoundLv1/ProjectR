// Copyright ProjectR. All Rights Reserved.

#include "UI/PRTripleResonanceOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "TripleResonance/PRTripleResonanceSubsystem.h"

namespace PRTripleResonanceOverlayPrivate
{
FText BuildAccessibleText(const FPRTripleResonanceSnapshot& Snapshot)
{
	const TCHAR* Status = TEXT("Unavailable");
	switch (Snapshot.State)
	{
	case EPRTripleResonanceState::Ready: Status = TEXT("Ready"); break;
	case EPRTripleResonanceState::QTEActive: Status = TEXT("QTE active"); break;
	case EPRTripleResonanceState::AbilityPending: Status = TEXT("Resonance ready"); break;
	case EPRTripleResonanceState::Executing: Status = TEXT("Executing"); break;
	case EPRTripleResonanceState::Resolved: Status = TEXT("Resolved"); break;
	case EPRTripleResonanceState::Failed: Status = TEXT("Failed"); break;
	case EPRTripleResonanceState::ReadyToRetry: Status = TEXT("Save retry required"); break;
	default: break;
	}
	const TCHAR* Step = TEXT("Execute to begin");
	if (Snapshot.ActiveStep == EPRTripleResonanceStep::Axiom) Step = TEXT("Axiom: Interact");
	else if (Snapshot.ActiveStep == EPRTripleResonanceStep::Kindle) Step = TEXT("Kindle: Attack");
	else if (Snapshot.ActiveStep == EPRTripleResonanceStep::Null) Step = TEXT("Null: Execute");
	const FString Failure = Snapshot.FailureReason.IsNone() ? TEXT("None") : Snapshot.FailureReason.ToString();
	return FText::FromString(FString::Printf(
		TEXT("TRIPLE RESONANCE\nStatus: %s\nInput: %s\nSequence: Axiom Interact -> Kindle Attack -> Null Execute\nWindow: %.2f seconds\nReason: %s"),
		Status, Step, Snapshot.RemainingSeconds, *Failure));
}
}

FPRTripleResonanceSnapshot UPRTripleResonanceOverlayWidget::GetTripleResonanceSnapshot() const
{
	FPRTripleResonanceSnapshot Snapshot;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UPRTripleResonanceSubsystem* Triple = GameInstance->GetSubsystem<UPRTripleResonanceSubsystem>()) Triple->GetSnapshot(Snapshot);
	}
	return Snapshot;
}

void UPRTripleResonanceOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!PresentationText && WidgetTree)
	{
		PresentationText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TripleResonanceAccessibleText")));
		if (!PresentationText)
		{
			PresentationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TripleResonanceAccessibleText"));
			if (!WidgetTree->RootWidget) WidgetTree->RootWidget = PresentationText;
			else if (UPanelWidget* Panel = Cast<UPanelWidget>(WidgetTree->RootWidget)) Panel->AddChild(PresentationText);
		}
	}
	if (UPRTripleResonanceSubsystem* Triple = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRTripleResonanceSubsystem>() : nullptr)
	{
		StateChangedHandle = Triple->OnStateChanged().AddUObject(this, &UPRTripleResonanceOverlayWidget::RefreshPresentation);
	}
	RefreshPresentation(GetTripleResonanceSnapshot());
}

void UPRTripleResonanceOverlayWidget::NativeDestruct()
{
	if (UPRTripleResonanceSubsystem* Triple = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRTripleResonanceSubsystem>() : nullptr)
	{
		Triple->OnStateChanged().Remove(StateChangedHandle);
	}
	StateChangedHandle.Reset();
	Super::NativeDestruct();
}

void UPRTripleResonanceOverlayWidget::RefreshPresentation(const FPRTripleResonanceSnapshot& Snapshot)
{
	if (PresentationText) PresentationText->SetText(PRTripleResonanceOverlayPrivate::BuildAccessibleText(Snapshot));
}
