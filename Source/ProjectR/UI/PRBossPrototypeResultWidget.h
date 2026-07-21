// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Enemies/Bosses/PRBossTypes.h"

#include "PRBossPrototypeResultWidget.generated.h"

/** Read-only prototype completion presenter; it never writes Save, rewards, or account state. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRBossPrototypeResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	const FPRPrototypeRunResult& GetPrototypeRunResult() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintImplementableEvent, Category="ProjectR|Boss") void OnPrototypeRunCompletedPresentation();

private:
	void EnsurePresentation();
	void RefreshPresentation();
	void HandlePrototypeRunCompleted(const FPRPrototypeRunResult& InResult);
	FPRPrototypeRunResult Result;
	FDelegateHandle CompletionHandle;

	UPROPERTY(Transient, meta=(BindWidgetOptional)) TObjectPtr<class UTextBlock> PrototypeResultText;
};
