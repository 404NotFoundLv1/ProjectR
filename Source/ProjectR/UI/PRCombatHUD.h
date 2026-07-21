// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/HUD.h"

#include "PRCombatHUD.generated.h"

class UPRCombatHUDWidget;

/** Local-only owner for ProjectR's read-only combat presentation. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API APRCombatHUD : public AHUD
{
	GENERATED_BODY()

public:
	UPRCombatHUDWidget* GetCombatHUDWidget() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|HUD")
	TSubclassOf<UPRCombatHUDWidget> CombatHUDWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPRCombatHUDWidget> CombatHUDWidget;
};
