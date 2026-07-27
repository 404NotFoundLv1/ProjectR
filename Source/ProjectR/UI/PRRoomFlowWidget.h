// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Roguelike/PRRoomTypes.h"

#include "PRRoomFlowWidget.generated.h"

class UPRRoomSubsystem;
class UPRRoguelikeContentRegistryDataAsset;

/** Read-only room flow projection; authoritative selection remains in UPRRoomSubsystem. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRRoomFlowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Resolves a player-facing room label from the fixed Roguelike content registry. */
	static FText GetChoiceLabel(const UPRRoguelikeContentRegistryDataAsset& Registry, const FPrimaryAssetId& RoomId);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="ProjectR|Roguelike") void RequestRoomSelection(FPrimaryAssetId RoomId);
	UFUNCTION() void SelectFirstRoom();
	UFUNCTION() void SelectSecondRoom();

private:
	void HandleRoomStateChanged(const FPRRoomRuntimeState& State);
	void SetSelectionInputMode(bool bActive);
	TWeakObjectPtr<UPRRoomSubsystem> RoomSubsystem;
	FDelegateHandle StateChangedHandle;
	TArray<FPrimaryAssetId> PublishedChoices;
};
