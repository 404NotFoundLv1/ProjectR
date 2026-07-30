// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRHeadmindFoundationFixedContractTest,
	"ProjectR.Chapter.Headmind.Foundation.FixedContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRHeadmindFoundationFixedContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Seed 61400 selects the fixed Headmind fusion directive"),
		UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Headmind"), 61400),
		FName(TEXT("Headmind.ObediencePrediction")));
	TestEqual(TEXT("Headmind uses the final fixed proof"),
		UPRChapterContentRegistryDataAsset::GetHeadmindProofId(),
		FName(TEXT("HumanAnomalyProof.Headmind")));
	return true;
}

#endif
