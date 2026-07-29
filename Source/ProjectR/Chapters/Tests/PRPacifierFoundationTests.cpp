// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Roguelike/PRRoomSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPacifierFoundationFixedContractTest,
	"ProjectR.Chapter.Pacifier.Foundation.FixedContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRPacifierFoundationFixedContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Pacifier chapter id is stable"),
		UPRChapterContentRegistryDataAsset::GetPacifierChapterId(),
		FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRChapter")), TEXT("DA_Chapter_Pacifier")));
	TestEqual(TEXT("Pacifier content id is stable"), UPRChapterContentRegistryDataAsset::GetPacifierContentId(), FName(TEXT("Pacifier")));
	TestEqual(TEXT("Pacifier boss id is stable"), UPRChapterContentRegistryDataAsset::GetPacifierBossId(), FName(TEXT("Pacifier")));
	TestEqual(TEXT("Pacifier proof id is stable"), UPRChapterContentRegistryDataAsset::GetPacifierProofId(), FName(TEXT("HumanAnomalyProof.Pacifier")));
	TestEqual(TEXT("Pacifier room registry id is closed"),
		UPRChapterContentRegistryDataAsset::GetPacifierRoomRegistryId(),
		FPrimaryAssetId(TEXT("ProjectRChapterRoguelikeRegistry"), TEXT("DA_RoguelikeContentRegistry_Pacifier")));
	TestEqual(TEXT("Pacifier enemy registry id is closed"),
		UPRChapterContentRegistryDataAsset::GetPacifierEnemyRegistryId(),
		FPrimaryAssetId(TEXT("ProjectREnemyContentRegistry"), TEXT("DA_EnemyContentRegistry_Pacifier")));
	TestEqual(TEXT("Pacifier final room id is closed"),
		UPRChapterContentRegistryDataAsset::GetPacifierFinalRoomId(),
		FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Pacifier_Boss_Pacifier")));

	const TPair<int32, FName> Expected[] = {
		{61200, TEXT("Pacifier.ComfortProjection")},
		{61201, TEXT("Pacifier.EmotionalDampening")},
		{61202, TEXT("Pacifier.IllusionVeil")},
		{61203, TEXT("Pacifier.RiskSuppression")},
		{61204, TEXT("Pacifier.SafetyIncentive")}};
	for (const TPair<int32, FName>& Entry : Expected)
	{
		TestEqual(
			FString::Printf(TEXT("Seed %d selects its fixed Pacifier directive"), Entry.Key),
			UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Pacifier"), Entry.Key),
			Entry.Value);
		TestEqual(
			FString::Printf(TEXT("Seed %d produces its fixed bounded path length"), Entry.Key),
			UPRRoomSubsystem::GetRoomPathLengthForSeed(Entry.Key),
			6 + (Entry.Key - 61200));
	}

	TestEqual(TEXT("Allocator mapping remains compatible"),
		UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Allocator"), 61000),
		FName(TEXT("Allocator.ResourceLock")));
	TestEqual(TEXT("Warden mapping remains compatible"),
		UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Warden"), 61100),
		FName(TEXT("Warden.PredictivePatrol")));
	TestTrue(TEXT("Unknown chapter content remains rejected"),
		UPRChapterContentRegistryDataAsset::GetDirectiveForContentAndSeed(TEXT("Unknown"), 61200).IsNone());

	FPRChapterPersistenceData ClosedChain;
	ClosedChain.CompletedChapterIds = {
		UPRChapterContentRegistryDataAsset::GetAllocatorChapterId(),
		UPRChapterContentRegistryDataAsset::GetWardenChapterId()};
	ClosedChain.HumanAnomalyProofIds = {
		UPRChapterContentRegistryDataAsset::GetAllocatorProofId(),
		UPRChapterContentRegistryDataAsset::GetWardenProofId()};
	TestTrue(TEXT("The fixed Allocator/Warden proof chain is accepted"),
		UPRChapterSubsystem::IsFixedProofChainValid(ClosedChain));

	FPRChapterPersistenceData MissingCompletion = ClosedChain;
	MissingCompletion.CompletedChapterIds.Remove(UPRChapterContentRegistryDataAsset::GetWardenChapterId());
	TestFalse(TEXT("A proof without its matching completed chapter is rejected"),
		UPRChapterSubsystem::IsFixedProofChainValid(MissingCompletion));

	FPRChapterPersistenceData UnknownProof = ClosedChain;
	UnknownProof.HumanAnomalyProofIds.Add(TEXT("HumanAnomalyProof.Unknown"));
	TestFalse(TEXT("Unknown proof ids cannot enter the fixed three-chapter closure"),
		UPRChapterSubsystem::IsFixedProofChainValid(UnknownProof));

	FPRChapterPersistenceData UnknownChapter = ClosedChain;
	UnknownChapter.CompletedChapterIds.Add(FPrimaryAssetId(TEXT("ProjectRChapter"), TEXT("DA_Chapter_Unknown")));
	TestFalse(TEXT("Unknown completed chapter ids cannot enter the fixed three-chapter closure"),
		UPRChapterSubsystem::IsFixedProofChainValid(UnknownChapter));
	return true;
}

#endif
