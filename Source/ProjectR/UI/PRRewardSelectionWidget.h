// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Roguelike/PRRewardTypes.h"
#include "Roguelike/PRRoomTypes.h"

#include "PRRewardSelectionWidget.generated.h"

class UPRRoomSubsystem;

UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRRewardSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="ProjectR|Roguelike") void RequestRewardSelection(FPrimaryAssetId RewardId);
	UFUNCTION() void SelectFirstReward();
	UFUNCTION() void SelectSecondReward();
	UFUNCTION() void SelectThirdReward();

private:
	void HandleRewardOfferChanged(const FPRRewardOffer& Offer);
	void HandleRoomStateChanged(const FPRRoomRuntimeState& State);
	void SetSelectionInputMode(bool bActive);
	TWeakObjectPtr<UPRRoomSubsystem> RoomSubsystem;
	FDelegateHandle OfferChangedHandle;
	FDelegateHandle StateChangedHandle;
	TArray<FPrimaryAssetId> PublishedChoices;
};
