// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "RealityHub/PRRealityHubForecastPolicy.h"
#include "RealityHub/PRRealityHubTypes.h"
#include "Director/PRPlayerProfileTypes.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRRealityHubFoundationTest,
	"ProjectR.RealityHub.Foundation.TerminalContractAndForecastFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRRealityHubFoundationTest::RunTest(const FString& Parameters)
{
	FPRRealityHubForecast Forecast;
	TestFalse(TEXT("An unavailable profile never produces a forecast"),
		FPRRealityHubForecastPolicy::BuildUnavailable(Forecast));
	TestEqual(TEXT("Unavailable forecast has an explicit result"),
		Forecast.Result, EPRRealityHubForecastResult::UnavailableProfile);
	TestFalse(TEXT("Unavailable forecast never exposes a rule id"), Forecast.RuleId.IsValid());

	FPRPlayerProfileSnapshot Profile;
	Profile.ProfileSessionId = FGuid::NewGuid();
	Profile.DeathCount = 2;
	TArray<FGameplayTag> Candidates;
	Candidates.Add(FGameplayTag::RequestGameplayTag(TEXT("Rule.SurvivalProtocol"), false));
	Candidates.Add(FGameplayTag::RequestGameplayTag(TEXT("Rule.DistanceCorrection"), false));
	TestTrue(TEXT("A local profile and fixed registry candidates produce a preview"),
		FPRRealityHubForecastPolicy::BuildForecast(Profile, Candidates, Forecast));
	TestEqual(TEXT("The deterministic preview selects the expected known rule"),
		Forecast.RuleId, FGameplayTag::RequestGameplayTag(TEXT("Rule.SurvivalProtocol"), false));
	TestEqual(TEXT("The preview keeps the deterministic level"), Forecast.Level, 2);
	return true;
}

#endif
