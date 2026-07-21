// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRBossHealthWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Enemies/Bosses/PRBossSubsystem.h"

namespace PRBossWidget
{
const TCHAR* ToPhaseText(const EPRAuditorBossPhase Phase)
{
	switch (Phase)
	{
	case EPRAuditorBossPhase::Dormant: return TEXT("Dormant");
	case EPRAuditorBossPhase::Sampling: return TEXT("Sampling");
	case EPRAuditorBossPhase::RuleAudit: return TEXT("RuleAudit");
	case EPRAuditorBossPhase::PredictionShield: return TEXT("PredictionShield");
	case EPRAuditorBossPhase::Defeated: return TEXT("Defeated");
	default: return TEXT("Unknown");
	}
}
}

const FPRBossRuntimeState& UPRBossHealthWidget::GetBossRuntimeState() const { return RuntimeState; }

void UPRBossHealthWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsurePresentation();
	if (UPRBossSubsystem* Bosses = GetWorld() ? GetWorld()->GetSubsystem<UPRBossSubsystem>() : nullptr)
	{
		Bosses->GetActiveBossState(RuntimeState);
		StateChangedHandle = Bosses->OnBossStateChanged().AddUObject(this, &UPRBossHealthWidget::HandleBossStateChanged);
	}
	RefreshPresentation();
}

void UPRBossHealthWidget::NativeDestruct()
{
	if (UPRBossSubsystem* Bosses = GetWorld() ? GetWorld()->GetSubsystem<UPRBossSubsystem>() : nullptr)
	{
		Bosses->OnBossStateChanged().Remove(StateChangedHandle);
	}
	Super::NativeDestruct();
}

void UPRBossHealthWidget::HandleBossStateChanged(const FPRBossRuntimeState& InState)
{
	RuntimeState = InState;
	RefreshPresentation();
	OnBossRuntimeStateUpdated();
}

void UPRBossHealthWidget::EnsurePresentation()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BossRuntimeRoot"));
	WidgetTree->RootWidget = Root;
	auto AddLine = [this, Root](const FName Name)
	{
		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Root->AddChildToVerticalBox(Line);
		return Line;
	};
	HealthShieldText = AddLine(TEXT("HealthShieldText"));
	PhaseText = AddLine(TEXT("PhaseText"));
	RuleText = AddLine(TEXT("RuleText"));
	PredictionText = AddLine(TEXT("PredictionText"));
	AttackText = AddLine(TEXT("AttackText"));
}

void UPRBossHealthWidget::RefreshPresentation()
{
	if (HealthShieldText)
	{
		HealthShieldText->SetText(FText::FromString(FString::Printf(TEXT("Health %.0f / %.0f  Shield %.0f / %.0f"), RuntimeState.Health, RuntimeState.MaxHealth, RuntimeState.Shield, RuntimeState.MaxShield)));
	}
	if (PhaseText) PhaseText->SetText(FText::FromString(FString::Printf(TEXT("Phase: %s"), PRBossWidget::ToPhaseText(RuntimeState.Phase))));
	if (RuleText) RuleText->SetText(FText::FromString(FString::Printf(TEXT("Rule: %s"), *RuntimeState.ActiveRuleId.ToString())));
	if (PredictionText) PredictionText->SetText(FText::FromString(FString::Printf(TEXT("Prediction: %s"), *RuntimeState.PredictedAbilityTag.ToString())));
	if (AttackText) AttackText->SetText(FText::FromString(FString::Printf(TEXT("Attack: %s  Phase: %d"), *RuntimeState.ActiveAttackTag.ToString(), RuntimeState.AttackPhase)));
}
