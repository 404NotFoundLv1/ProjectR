// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"

#include "PRPlayerResourcesWidget.generated.h"

class APRPlayerState;
class UProgressBar;
class UTextBlock;
struct FPRAttributeChange;

/** Local, event-driven display of the player combat resources. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRPlayerResourcesWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	float GetHealthPercent() const;
	float GetShieldPercent() const;
	float GetEnergyPercent() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> ShieldBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> EnergyBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ShieldText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> EnergyText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> PermissionText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ResonanceText;

private:
	void HandleAttributeChanged(const FPRAttributeChange& Change);
	void RefreshPresentation();

	TWeakObjectPtr<APRPlayerState> PlayerState;
	FDelegateHandle AttributeChangedHandle;
	float HealthPercent = 0.0f;
	float ShieldPercent = 0.0f;
	float EnergyPercent = 0.0f;
};
