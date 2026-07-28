// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/HUD.h"

#include "PRRealityHubHUD.generated.h"

class UPRRealityHubWidget;

/** Local-only Reality Hub presentation owner. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API APRRealityHubHUD : public AHUD
{
	GENERATED_BODY()

public:
	UPRRealityHubWidget* GetRealityHubWidget() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|RealityHub") TSubclassOf<UPRRealityHubWidget> RealityHubWidgetClass;

private:
	UPROPERTY(Transient) TObjectPtr<UPRRealityHubWidget> RealityHubWidget;
};
