// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Roguelike/Progression/PRProgressionTypes.h"
#include "Roguelike/Progression/PRProgressionRegistryDataAsset.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRProgressionFoundationTest,
	"ProjectR.Progression.Foundation.SchemaRegistryAndPrerequisites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRProgressionFoundationTest::RunTest(const FString& Parameters)
{
	const FPRProgressionSnapshot Snapshot;
	TestEqual(TEXT("Counterproof starts at zero"), Snapshot.CounterproofFragments, 0);
	TestEqual(TEXT("Memory starts at zero"), Snapshot.MemoryFragments, 0);
	TestTrue(TEXT("Unlock list starts empty"), Snapshot.UnlockedNodeIds.IsEmpty());
	TestEqual(TEXT("Unlock sequence starts at zero"), Snapshot.UnlockSequence, int64{0});

	UPRProgressionRegistryDataAsset* Registry = LoadObject<UPRProgressionRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Data/Progression/DA_ProgressionRegistry.DA_ProgressionRegistry"));
	TestNotNull(TEXT("The fixed progression registry loads"), Registry);
	if (Registry)
	{
		TestTrue(TEXT("Registry enforces all twelve fixed node definitions"), Registry->IsRegistryReady());
		TestEqual(TEXT("Registry exposes exactly twelve ordered node references"), Registry->Nodes.Num(), 12);
		TestNotNull(TEXT("Registry resolves PlayerMaxHealth by stable id"),
			Registry->FindNode(FPrimaryAssetId(FPrimaryAssetType(TEXT("ProgressionNode")), TEXT("PlayerMaxHealth"))));
	}
	return true;
}

#endif
