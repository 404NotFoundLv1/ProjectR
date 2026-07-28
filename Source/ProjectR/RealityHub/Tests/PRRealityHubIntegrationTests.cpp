// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "RealityHub/PRRealityHubTerminalRegistryDataAsset.h"

#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRRealityHubIntegrationTest,
	"ProjectR.RealityHub.Integration.FixedRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRRealityHubIntegrationTest::RunTest(const FString& Parameters)
{
	const UPRRealityHubTerminalRegistryDataAsset* Registry = LoadObject<UPRRealityHubTerminalRegistryDataAsset>(
		nullptr,
		TEXT("/Game/ProjectR/Data/RealityHub/DA_RealityHubTerminalRegistry.DA_RealityHubTerminalRegistry"));
	if (!TestNotNull(TEXT("The fixed Reality Hub registry asset exists"), Registry)) return false;
	TestTrue(TEXT("The registry contains only the fixed five terminals and a ready Director registry"), Registry->IsRegistryReady());
	TArray<FGameplayTag> RuleIds;
	Registry->GetForecastRuleIds(RuleIds);
	TestTrue(TEXT("The local forecast consumes at least one known rule id"), RuleIds.Num() > 0);
	return true;
}

#endif
