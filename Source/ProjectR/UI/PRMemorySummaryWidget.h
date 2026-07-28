// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Memory/PRMemoryTypes.h"

#include "PRMemorySummaryWidget.generated.h"

class UTextBlock;

/** Read-only presentation of the latest bounded summary and its three fixed transient choices. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRMemorySummaryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintPure, Category="ProjectR|Memory") FPRMemorySnapshot GetDisplayedSnapshot() const;
	UFUNCTION(BlueprintImplementableEvent, Category="ProjectR|Memory") void PresentMemorySnapshot(const FPRMemorySnapshot& NewSnapshot);
private:
	UFUNCTION() void SubmitFirst();
	UFUNCTION() void SubmitSecond();
	UFUNCTION() void SubmitThird();
	void Submit(EPRMemoryPlayerOptionSlot InOptionSlot);
	void HandleStateChanged(const FPRMemorySnapshot& NewSnapshot);
	void HandleOperation(const FPRMemoryOperationEvent& Event);
	void RenderSnapshot();
	void SetOptionText(FName ButtonName, const FText& Text);
	class UPRMemorySubsystem* GetMemory() const;
	FDelegateHandle StateChangedHandle;
	FDelegateHandle OperationHandle;
	FPRMemorySnapshot DisplayedSnapshot;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_MemoryStatus;
};
