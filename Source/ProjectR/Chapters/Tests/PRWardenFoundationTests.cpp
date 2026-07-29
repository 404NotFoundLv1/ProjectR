// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/Warden/PRWardenChapterDataAsset.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRWardenFoundationFixedContractTest,
	"ProjectR.Chapter.Warden.Foundation.FixedContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRWardenFoundationFixedContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Warden chapter id is stable"),
		UPRChapterContentRegistryDataAsset::GetWardenChapterId(),
		FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRChapter")), TEXT("DA_Chapter_Warden")));
	TestEqual(TEXT("Warden content id is stable"), UPRChapterContentRegistryDataAsset::GetWardenContentId(), FName(TEXT("Warden")));
	TestEqual(TEXT("Warden proof id is stable"), UPRChapterContentRegistryDataAsset::GetWardenProofId(), FName(TEXT("HumanAnomalyProof.Warden")));
	TestEqual(TEXT("Warden seed 61100 selects PredictivePatrol"),
		UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Warden"), 61100),
		FName(TEXT("Warden.PredictivePatrol")));
	TestEqual(TEXT("Warden seed 61104 selects TrapEscalation"),
		UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Warden"), 61104),
		FName(TEXT("Warden.TrapEscalation")));
	TestTrue(TEXT("Unknown chapter content has no directive"),
		UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Unknown"), 61100).IsNone());
	const UPRChapterRoguelikeContentRegistryDataAsset* Registry = LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Warden/DA_RoguelikeContentRegistry_Warden.DA_RoguelikeContentRegistry_Warden"));
	TestNotNull(TEXT("The fixed Warden Registry asset loads"), Registry);
	if (Registry)
	{
		TestEqual(TEXT("The fixed Warden Registry has the closed Warden content id"), Registry->ContentId, FName(TEXT("Warden")));
		TestTrue(TEXT("The fixed Warden Registry has a fully closed runtime manifest"), Registry->IsRegistryReady());
	}
	const UPRWardenChapterDataAsset* Chapter = LoadObject<UPRWardenChapterDataAsset>(nullptr, TEXT("/Game/ProjectR/Chapters/Warden/DA_Chapter_Warden.DA_Chapter_Warden"));
	TestNotNull(TEXT("The fixed Warden Chapter asset loads"), Chapter);
	if (Chapter)
	{
		TestEqual(TEXT("The fixed Warden Chapter has its exact closed Enemy Registry identity"), Chapter->EnemyContentRegistryId, UPRChapterContentRegistryDataAsset::GetWardenEnemyRegistryId());
	}
	return true;
}

#endif
