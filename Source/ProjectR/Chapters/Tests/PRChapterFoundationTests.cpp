// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Chapters/PRChapterTypes.h"
#include "Misc/AutomationTest.h"

namespace PRChapterAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRChapterFoundationFixedAllocatorContractTest,
	"ProjectR.Chapter.Foundation.FixedAllocatorContract",
	PRChapterAutomation::TestFlags)

bool FPRChapterFoundationFixedAllocatorContractTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Chapter subsystem is a GameInstance subsystem"), UPRChapterSubsystem::StaticClass());

	FPRChapterSnapshot Snapshot;
	TestEqual(TEXT("A new chapter snapshot is inactive"), Snapshot.State, EPRChapterLifecycleState::Inactive);
	TestEqual(TEXT("Allocator chapter id is stable"), UPRChapterContentRegistryDataAsset::GetAllocatorChapterId(),
		FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRChapter")), TEXT("DA_Chapter_Allocator")));
	TestEqual(TEXT("Allocator content id is stable"), UPRChapterContentRegistryDataAsset::GetAllocatorContentId(), FName(TEXT("Allocator")));
	TestEqual(TEXT("Allocator proof id is stable"), UPRChapterContentRegistryDataAsset::GetAllocatorProofId(), FName(TEXT("HumanAnomalyProof.Allocator")));

	const TArray<FName> Directives = UPRChapterContentRegistryDataAsset::GetAllocatorDirectiveIds();
	TestEqual(TEXT("Allocator has exactly five local directives"), Directives.Num(), 5);
	const TArray<FName> Expected = {
		TEXT("Allocator.ResourceLock"),
		TEXT("Allocator.RewardWithholding"),
		TEXT("Allocator.PriceAudit"),
		TEXT("Allocator.ScarcityMarkup"),
		TEXT("Allocator.EqualizationQuota")};
	TestEqual(TEXT("Fixed seed 61000 selects ResourceLock"), UPRChapterContentRegistryDataAsset::GetDirectiveForSeed(61000), Expected[0]);
	TestEqual(TEXT("Fixed seed 61004 selects EqualizationQuota"), UPRChapterContentRegistryDataAsset::GetDirectiveForSeed(61004), Expected[4]);
	TestEqual(TEXT("Directive ordering is the public deterministic mapping"), Directives, Expected);
	return true;
}

#endif
