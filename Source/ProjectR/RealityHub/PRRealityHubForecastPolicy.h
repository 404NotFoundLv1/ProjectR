// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RealityHub/PRRealityHubTypes.h"

/** Pure local policy for the Hub preview; it cannot call a provider or mutate Director state. */
struct PROJECTR_API FPRRealityHubForecastPolicy
{
	static bool BuildUnavailable(FPRRealityHubForecast& OutForecast);
	static bool BuildForecast(const struct FPRPlayerProfileSnapshot& Profile, const TArray<FGameplayTag>& CandidateRuleIds, FPRRealityHubForecast& OutForecast);
};
