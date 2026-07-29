// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Chapters/Pacifier/PRPacifierChapterDataAsset.h"
#include "Misc/AutomationTest.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRuleDataAsset.h"
#include "Enemies/PREnemyContentRegistryDataAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPacifierContentContractTest,
	"ProjectR.Chapter.Pacifier.Content.ClosedPressureAndStoryContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRPacifierContentContractTest::RunTest(const FString& Parameters)
{
	UPRChapterRuleDataAsset* Rule = NewObject<UPRChapterRuleDataAsset>();
	Rule->ContentId = UPRChapterContentRegistryDataAsset::GetPacifierContentId();
	Rule->DirectiveId = TEXT("Pacifier.IllusionVeil");
	Rule->RequiredDirectorRuleId = FGameplayTag::RequestGameplayTag(TEXT("Rule.PredictionLock"), false);
	Rule->PreferredRoomIds = {FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Pacifier_Combat_IllusionSweep"))};
	TestTrue(TEXT("Pacifier directive accepts only its exact Director whitelist mapping"), Rule->IsRuleDefinitionValid());

	UPRChapterRoguelikeContentRegistryDataAsset* Registry = NewObject<UPRChapterRoguelikeContentRegistryDataAsset>();
	Registry->ContentId = UPRChapterContentRegistryDataAsset::GetPacifierContentId();
	TestTrue(TEXT("Pacifier is one of the three fixed chapter shop contents"), Registry->SupportsChapterShopRooms());
	FPRChapterEventPressureBinding Binding;
	Binding.EventId = FPrimaryAssetId(TEXT("ProjectRRoomEvent"), TEXT("DA_RoomEvent_Pacifier_SafeRewardOffer"));
	Binding.ChoiceId = TEXT("TakeSafeReward");
	Binding.PressureDelta = 1;
	Binding.ExcludedFutureRoomIds = {FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Pacifier_Combat_IllusionSweep"))};
	Registry->EventPressureBindings = {Binding};
	int32 Delta = 0;
	TestTrue(TEXT("Known Pacifier event choices expose their bounded pressure delta"),
		Registry->FindPressureDelta(Binding.EventId, Binding.ChoiceId, Delta));
	TestEqual(TEXT("SafeRewardOffer raises ComfortPressure by one"), Delta, 1);
	TestFalse(TEXT("Unknown Pacifier choices are rejected"),
		Registry->FindPressureDelta(Binding.EventId, TEXT("UnknownChoice"), Delta));

	UPRPacifierChapterDataAsset* Chapter = NewObject<UPRPacifierChapterDataAsset>();
	Chapter->BaseStoryText = FText::FromString(TEXT("base"));
	Chapter->NoRetreatStoryText = FText::FromString(TEXT("no-retreat"));
	Chapter->LearnToRetreatStoryText = FText::FromString(TEXT("learn-retreat"));
	const FPRPacifierStoryProjection Unavailable = Chapter->BuildStoryProjection(false, true, true, true);
	TestFalse(TEXT("Kindle story is unavailable when Kindle is not Primary"), Unavailable.bAvailable);
	TestEqual(TEXT("Unavailable story uses a fixed fallback"), Unavailable.FallbackReason, FName(TEXT("Pacifier.StoryUnavailable")));
	const FPRPacifierStoryProjection Advanced = Chapter->BuildStoryProjection(true, true, true, true);
	TestTrue(TEXT("Completed public quest facts expose a fixed local story"), Advanced.bAvailable);
	TestEqual(TEXT("LearnToRetreat has deterministic precedence"),
		Advanced.StoryBeatId, FName(TEXT("Story.Pacifier.Kindle.LearnToRetreat")));

	const UPRPacifierChapterDataAsset* AuthoredChapter = LoadObject<UPRPacifierChapterDataAsset>(
		nullptr,
		TEXT("/Game/ProjectR/Chapters/Pacifier/DA_Chapter_Pacifier.DA_Chapter_Pacifier"));
	const UPRChapterRoguelikeContentRegistryDataAsset* AuthoredRoomRegistry =
		LoadObject<UPRChapterRoguelikeContentRegistryDataAsset>(
			nullptr,
			TEXT("/Game/ProjectR/Chapters/Pacifier/DA_RoguelikeContentRegistry_Pacifier.DA_RoguelikeContentRegistry_Pacifier"));
	const UPREnemyContentRegistryDataAsset* AuthoredEnemyRegistry = LoadObject<UPREnemyContentRegistryDataAsset>(
		nullptr,
		TEXT("/Game/ProjectR/Chapters/Pacifier/DA_EnemyContentRegistry_Pacifier.DA_EnemyContentRegistry_Pacifier"));
	TestTrue(TEXT("Authored Pacifier chapter is present and valid"), AuthoredChapter && AuthoredChapter->IsPacifierDefinitionValid());
	TestTrue(TEXT("Authored Pacifier room closure is present and valid"), AuthoredRoomRegistry && AuthoredRoomRegistry->IsRegistryReady());
	TestTrue(TEXT("Authored Pacifier enemy closure is present and valid"), AuthoredEnemyRegistry && AuthoredEnemyRegistry->IsRegistryReady());
	return true;
}

#endif
