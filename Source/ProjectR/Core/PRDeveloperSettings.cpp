// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/PRDeveloperSettings.h"

UPRDeveloperSettings::UPRDeveloperSettings()
	: bEnableDebugFeatures(
#if UE_BUILD_SHIPPING
		false
#else
		true
#endif
	)
	, bUseMockDirector(true)
	, bEnableSteamFeatures(false)
{
}
