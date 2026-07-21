// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Enemies/Bosses/PRBossTypes.h"

#include "PRCombatHUDWidget.generated.h"

class UPRBossHealthWidget;
class UPRBossPrototypeResultWidget;
class UTextBlock;
class UWidget;

/** Composes read-only player, ability, feedback, and frozen Boss presentation widgets. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BossContextText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UPRBossHealthWidget> BossHealth;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UPRBossPrototypeResultWidget> BossPrototypeResult;

private:
	void HandleBossStateChanged(const FPRBossRuntimeState& InState);
	void RefreshBossPresentation(const FPRBossRuntimeState& InState, bool bHasBoss);

	FDelegateHandle BossStateChangedHandle;
};
