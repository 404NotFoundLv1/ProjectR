// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "PRCombatFeedbackWidget.generated.h"

class UPRAbilitySystemComponent;
class UTextBlock;
class UVerticalBox;
struct FPRAbilityLifecycleEvent;
struct FPRCombatEvent;

/** Bounded local feedback stream derived from immutable combat and ability facts. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRCombatFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> FeedbackEntries;

private:
	struct FFeedbackEntry
	{
		FGuid Id;
		FName SourceId;
		FName TargetId;
		FGameplayTag AbilityTag;
		FName Semantic;
		double CreatedAt = 0.0;
		int32 Count = 1;
		float TotalValue = 0.0f;
		FTimerHandle ExpireTimer;
		TObjectPtr<UTextBlock> Text;
	};

	void HandleCombatEvent(const FPRCombatEvent& Event);
	void HandleAbilityLifecycleEvent(const FPRAbilityLifecycleEvent& Event);
	void AddFeedback(FName SourceId, FName TargetId, FGameplayTag AbilityTag, FName Semantic, const FString& Text, float Value, bool bAllowMerge);
	void RemoveFeedback(FGuid Id);
	void ClearFeedback();
	bool IsPlayerRelevant(const FPRCombatEvent& Event) const;

	TWeakObjectPtr<UPRAbilitySystemComponent> AbilitySystem;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle AbilityLifecycleHandle;
	TArray<FFeedbackEntry> Entries;
	TSet<FGuid> ConsumedCombatEventIds;
};
