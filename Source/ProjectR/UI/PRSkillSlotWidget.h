// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/PRAbilityTypes.h"
#include "Blueprint/UserWidget.h"

#include "PRSkillSlotWidget.generated.h"

class UProgressBar;
class UTextBlock;

/** One immutable AbilitySet entry presented by the local combat HUD. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRSkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ConfigureSlot(const FGameplayTag& InAbilityTag, int32 InSlotIndex);
	void SetRuntimeState(const FPRAbilityRuntimeState& InState);
	FGameplayTag GetAbilityTag() const;
	float GetCooldownPercent() const;
	static FText GetDisplayNameForAbilityTag(const FGameplayTag& InAbilityTag);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SkillNameText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SkillStateText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> FailureText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> CooldownBar;

private:
	void RefreshPresentation();

	FGameplayTag AbilityTag;
	FPRAbilityRuntimeState RuntimeState;
	int32 SlotIndex = INDEX_NONE;
};
