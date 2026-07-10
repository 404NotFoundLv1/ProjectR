// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"

#include "PRDeveloperSettings.generated.h"

/** Project-wide feature gates used to select safe development and fallback behavior. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="ProjectR"))
class PROJECTR_API UPRDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPRDeveloperSettings();

	/** Enables development-only diagnostics where the consuming system also permits them. */
	UPROPERTY(Config, EditAnywhere, Category="Feature Flags", meta=(DisplayName="Enable Debug Features"))
	bool bEnableDebugFeatures;

	/** Uses the deterministic local Director fallback instead of an external provider. */
	UPROPERTY(Config, EditAnywhere, Category="Feature Flags", meta=(DisplayName="Use Mock Director"))
	bool bUseMockDirector;

	/** Allows future Steam integrations to run after their owning version implements them. */
	UPROPERTY(Config, EditAnywhere, Category="Feature Flags", meta=(DisplayName="Enable Steam Features"))
	bool bEnableSteamFeatures;
};
