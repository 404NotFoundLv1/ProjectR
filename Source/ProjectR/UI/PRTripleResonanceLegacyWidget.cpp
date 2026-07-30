// Copyright ProjectR. All Rights Reserved.

#include "UI/PRTripleResonanceLegacyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "TripleResonance/PRTripleResonanceSubsystem.h"

FPRTripleResonanceLegacySnapshot UPRTripleResonanceLegacyWidget::GetTripleResonanceLegacySnapshot() const
{
	FPRTripleResonanceLegacySnapshot Snapshot;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UPRTripleResonanceSubsystem* Triple = GameInstance->GetSubsystem<UPRTripleResonanceSubsystem>()) Triple->GetLegacySnapshot(Snapshot);
	}
	return Snapshot;
}

void UPRTripleResonanceLegacyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!PresentationText && WidgetTree)
	{
		PresentationText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TripleResonanceLegacyAccessibleText")));
		if (!PresentationText)
		{
			PresentationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TripleResonanceLegacyAccessibleText"));
			if (!WidgetTree->RootWidget) WidgetTree->RootWidget = PresentationText;
			else if (UPanelWidget* Panel = Cast<UPanelWidget>(WidgetTree->RootWidget)) Panel->AddChild(PresentationText);
		}
	}
	if (UPRTripleResonanceSubsystem* Triple = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRTripleResonanceSubsystem>() : nullptr)
	{
		OperationHandle = Triple->OnOperation().AddUObject(this, &UPRTripleResonanceLegacyWidget::RefreshPresentation);
	}
	RefreshPresentation(FPRTripleResonanceOperationEvent());
}

void UPRTripleResonanceLegacyWidget::NativeDestruct()
{
	if (UPRTripleResonanceSubsystem* Triple = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRTripleResonanceSubsystem>() : nullptr)
	{
		Triple->OnOperation().Remove(OperationHandle);
	}
	OperationHandle.Reset();
	Super::NativeDestruct();
}

void UPRTripleResonanceLegacyWidget::RefreshPresentation(const FPRTripleResonanceOperationEvent&)
{
	if (!PresentationText) return;
	const FPRTripleResonanceLegacySnapshot Snapshot = GetTripleResonanceLegacySnapshot();
	const FString Skill = Snapshot.bHasSkillMemory ? Snapshot.AbilityTag.ToString() : TEXT("None");
	PresentationText->SetText(FText::FromString(FString::Printf(
		TEXT("TRIPLE RESONANCE LEGACY\nSkill memory: %s\nHigh-risk proof: %s"),
		*Skill, Snapshot.bHasHighRiskProof ? TEXT("Earned") : TEXT("Not earned"))));
}
