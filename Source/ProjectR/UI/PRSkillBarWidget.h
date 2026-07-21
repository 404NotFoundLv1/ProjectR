// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"

#include "PRSkillBarWidget.generated.h"

class UPRAbilitySetDataAsset;
class UPRAbilitySystemComponent;
class UPRSkillSlotWidget;
struct FPRAbilityLifecycleEvent;

/** Reads the frozen DefaultAbilitySet and reflects its six runtime AbilitySpecs. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRSkillBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|HUD")
	TObjectPtr<UPRAbilitySetDataAsset> DefaultAbilitySet;

private:
	void HandleAbilityLifecycleEvent(const FPRAbilityLifecycleEvent& Event);
	void RefreshAllSlots();
	void RefreshCooldowns();
	void UpdateCooldownTimer();

	TWeakObjectPtr<UPRAbilitySystemComponent> AbilitySystem;
	TArray<TObjectPtr<UPRSkillSlotWidget>> Slots;
	FDelegateHandle AbilityLifecycleHandle;
	FTimerHandle CooldownTimer;
};
