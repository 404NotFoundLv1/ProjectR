// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "RealityHub/PRRealityHubTypes.h"

#include "PRRealityHubWidget.generated.h"

class UButton;
class UTextBlock;

/** Read-only Hub presentation with fixed C++ button delegates. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API UPRRealityHubWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category="ProjectR|RealityHub") FPRRealityHubSnapshot GetDisplayedSnapshot() const;
	UFUNCTION(BlueprintPure, Category="ProjectR|RealityHub") FPRRealityHubForecast GetDisplayedForecast() const;

	UFUNCTION(BlueprintImplementableEvent, Category="ProjectR|RealityHub") void PresentSnapshot(const FPRRealityHubSnapshot& NewSnapshot);
	UFUNCTION(BlueprintImplementableEvent, Category="ProjectR|RealityHub") void PresentOperation(const FPRRealityHubOperationEvent& Event);

private:
	UFUNCTION() void HandleCassetteClicked();
	UFUNCTION() void HandleCreateProfileClicked();
	UFUNCTION() void HandleTechnicianClicked();
	UFUNCTION() void HandleSecurityClicked();
	UFUNCTION() void HandleExileClicked();
	UFUNCTION() void HandleObserverClicked();
	UFUNCTION() void HandleBlankClicked();
	UFUNCTION() void HandleEnterNetworkClicked();
	UFUNCTION() void HandleCompanionClicked();
	UFUNCTION() void HandleGraveyardClicked();
	UFUNCTION() void HandleTrainingClicked();
	UFUNCTION() void HandleForecasterClicked();
	UFUNCTION() void HandleProgressionClicked();
	void HandleStateChanged(const FPRRealityHubSnapshot& NewSnapshot);
	void HandleOperation(const FPRRealityHubOperationEvent& Event);
	void SetStatusText(const FText& NewStatus);
	class UPRRealityHubSubsystem* GetHub() const;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_CassetteSlot;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_CreateProfile;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_IdentityTechnician;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_IdentitySecurity;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_IdentityExile;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_IdentityObserver;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_IdentityBlank;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_EnterNetwork;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Companion;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Graveyard;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_TrainingSimulator;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_DirectorForecaster;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Progression;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Status;
	FDelegateHandle StateChangedHandle;
	FDelegateHandle OperationHandle;
	FPRRealityHubSnapshot DisplayedSnapshot;
	FPRRealityHubForecast DisplayedForecast;
};
