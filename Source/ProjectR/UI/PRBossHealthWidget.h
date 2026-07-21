// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Enemies/Bosses/PRBossTypes.h"

#include "PRBossHealthWidget.generated.h"

/** Read-only presentation adapter. Blueprint graphs may bind fields but cannot make Boss decisions. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRBossHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	const FPRBossRuntimeState& GetBossRuntimeState() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintImplementableEvent, Category="ProjectR|Boss") void OnBossRuntimeStateUpdated();

private:
	void EnsurePresentation();
	void RefreshPresentation();
	void HandleBossStateChanged(const FPRBossRuntimeState& InState);
	FPRBossRuntimeState RuntimeState;
	FDelegateHandle StateChangedHandle;

	UPROPERTY(Transient) TObjectPtr<class UTextBlock> HealthShieldText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> PhaseText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> RuleText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> PredictionText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> AttackText;
};
