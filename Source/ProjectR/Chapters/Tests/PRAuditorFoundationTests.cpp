// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAuditorFoundationFixedContractTest,
	"ProjectR.Chapter.Auditor.Foundation.FixedContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRAuditorFoundationFixedContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Seed 61300 selects the fixed Auditor habit directive"),
		UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Auditor"), 61300),
		FName(TEXT("Auditor.HabitReplication")));
	return true;
}

#endif
