// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Roguelike/Account/PRAccountIdentityDataAsset.h"
#include "Roguelike/Account/PRAccountIdentityRegistryDataAsset.h"
#include "Save/PRAccountSaveTypes.h"

#include "Misc/AutomationTest.h"

namespace PRAccountFoundationAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAccountFoundationIdentitySchemaAndRegistryTest,
	"ProjectR.Account.Foundation.IdentitySchemaAndRegistry",
	PRAccountFoundationAutomation::TestFlags)

bool FPRAccountFoundationIdentitySchemaAndRegistryTest::RunTest(const FString& Parameters)
{
	FPRAccountPersistenceData Persistence = FPRAccountPersistenceContract::MakeDefault();
	TestFalse(TEXT("A default profile has no active account"), Persistence.bHasActiveAccount);
	TestEqual(TEXT("A default profile has no graveyard records"), Persistence.Graveyard.Num(), 0);
	TestEqual(TEXT("A default profile has no deleted accounts"), Persistence.LifetimeDeletedAccountCount, int64{0});
	TestEqual(TEXT("A default profile has no counterproof fragments"), Persistence.CounterproofFragments, int32{0});
	TestTrue(TEXT("Default account persistence is canonical"), FPRAccountPersistenceContract::IsCanonical(Persistence));

	TestNotNull(TEXT("Account identity assets are primary data assets"), UPRAccountIdentityDataAsset::StaticClass());
	TestNotNull(TEXT("Account identity registry is a primary data asset"), UPRAccountIdentityRegistryDataAsset::StaticClass());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
