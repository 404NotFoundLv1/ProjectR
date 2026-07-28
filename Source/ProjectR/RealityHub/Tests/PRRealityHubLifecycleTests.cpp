// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "RealityHub/PRRealityHubSubsystem.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRRealityHubLifecycleTest,
	"ProjectR.RealityHub.Lifecycle.FixedIdentityAndSeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRRealityHubLifecycleTest::RunTest(const FString& Parameters)
{
	const FPrimaryAssetId Technician = UPRRealityHubSubsystem::GetFixedIdentityId(EPRRealityHubIdentity::Technician);
	TestEqual(TEXT("The cassette slot maps Technician to the fixed registry identity"), Technician,
		FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRAccountIdentity")), TEXT("Technician")));
	const FGuid AccountId(1, 2, 3, 4);
	TestEqual(TEXT("Run seed is deterministic for an account"),
		UPRRealityHubSubsystem::MakeFixedRunSeed(AccountId), UPRRealityHubSubsystem::MakeFixedRunSeed(AccountId));
	TestNotEqual(TEXT("A valid fixed seed is never zero"), UPRRealityHubSubsystem::MakeFixedRunSeed(AccountId), 0);
	return true;
}

#endif
