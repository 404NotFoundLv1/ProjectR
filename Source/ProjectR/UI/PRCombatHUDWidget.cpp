// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRCombatHUDWidget.h"

#include "Components/TextBlock.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "UI/PRBossHealthWidget.h"
#include "UI/PRBossPrototypeResultWidget.h"

void UPRCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	FPRBossRuntimeState State;
	bool bHasBoss = false;
	if (UPRBossSubsystem* Bosses = GetWorld() ? GetWorld()->GetSubsystem<UPRBossSubsystem>() : nullptr)
	{
		bHasBoss = Bosses->GetActiveBossState(State);
		BossStateChangedHandle = Bosses->OnBossStateChanged().AddUObject(
			this, &UPRCombatHUDWidget::HandleBossStateChanged);
	}
	RefreshBossPresentation(State, bHasBoss);
}

void UPRCombatHUDWidget::NativeDestruct()
{
	if (UPRBossSubsystem* Bosses = GetWorld() ? GetWorld()->GetSubsystem<UPRBossSubsystem>() : nullptr)
	{
		Bosses->OnBossStateChanged().Remove(BossStateChangedHandle);
	}
	BossStateChangedHandle.Reset();
	Super::NativeDestruct();
}

void UPRCombatHUDWidget::HandleBossStateChanged(const FPRBossRuntimeState& InState)
{
	RefreshBossPresentation(InState, true);
}

void UPRCombatHUDWidget::RefreshBossPresentation(const FPRBossRuntimeState& InState, const bool bHasBoss)
{
	const ESlateVisibility BossVisibility = bHasBoss
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;
	if (BossContextText)
	{
		BossContextText->SetVisibility(BossVisibility);
		BossContextText->SetText(FText::FromString(bHasBoss
			? FString::Printf(TEXT("审计者本地规则: %s"), *InState.ActiveRuleId.ToString())
			: TEXT("")));
	}
	if (BossHealth)
	{
		BossHealth->SetVisibility(BossVisibility);
	}
	if (BossPrototypeResult)
	{
		BossPrototypeResult->SetVisibility(BossVisibility);
	}
}
