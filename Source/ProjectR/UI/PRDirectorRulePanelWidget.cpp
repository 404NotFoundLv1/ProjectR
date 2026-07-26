// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRDirectorRulePanelWidget.h"

#include "Components/TextBlock.h"
#include "Director/PRDirectorSubsystem.h"
#include "Engine/GameInstance.h"

void UPRDirectorRulePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UPRDirectorSubsystem* Director = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr;
	if (Director)
	{
		RuleRuntimeChangedHandle = Director->OnRuleRuntimeChanged().AddUObject(this, &UPRDirectorRulePanelWidget::HandleRuleRuntimeChanged);
		TArray<FPRDirectorRuleRuntimeState> States;
		Director->GetRuleRuntimeStates(States);
		Refresh(States.IsEmpty() ? nullptr : &States[0]);
	}
	else
	{
		Refresh(nullptr);
	}
}

void UPRDirectorRulePanelWidget::NativeDestruct()
{
	if (UPRDirectorSubsystem* Director = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr)
	{
		Director->OnRuleRuntimeChanged().Remove(RuleRuntimeChangedHandle);
	}
	RuleRuntimeChangedHandle.Reset();
	Super::NativeDestruct();
}

void UPRDirectorRulePanelWidget::HandleRuleRuntimeChanged(const FPRDirectorRuleRuntimeState& State)
{
	Refresh(State.Status == EPRDirectorRuleRuntimeStatus::Inactive ? nullptr : &State);
}

void UPRDirectorRulePanelWidget::Refresh(const FPRDirectorRuleRuntimeState* State)
{
	const bool bVisible = State != nullptr;
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (RuleNameText) RuleNameText->SetText(bVisible ? FText::FromString(State->RuleId.ToString()) : FText::GetEmpty());
	if (RuleReasonText) RuleReasonText->SetText(bVisible ? State->VisibleReason : FText::GetEmpty());
	if (RuleEffectText) RuleEffectText->SetText(bVisible ? State->EffectDescription : FText::GetEmpty());
	if (RuleCounterText) RuleCounterText->SetText(bVisible
		? FText::FromString(FString::Printf(TEXT("%s (%d/%d)"), *State->CounterDescription.ToString(), State->CounterProgress, State->CounterTarget))
		: FText::GetEmpty());
}
