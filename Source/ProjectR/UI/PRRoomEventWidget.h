// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Roguelike/PRRoomTypes.h"

#include "PRRoomEventWidget.generated.h"

class UPRRoomSubsystem;

UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRRoomEventWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="ProjectR|Roguelike") void RequestEventChoice(FName ChoiceId);
	UFUNCTION() void SelectFirstEventChoice();
	UFUNCTION() void SelectSecondEventChoice();
	UFUNCTION() void SelectThirdEventChoice();

private:
	void HandleRoomStateChanged(const FPRRoomRuntimeState& State);
	void HandleEventResolved(const FPRRoomEventResult& Result);
	void SetSelectionInputMode(bool bActive);
	TWeakObjectPtr<UPRRoomSubsystem> RoomSubsystem;
	FDelegateHandle StateChangedHandle;
	FDelegateHandle EventResolvedHandle;
	TArray<FName> PublishedChoices;
};
