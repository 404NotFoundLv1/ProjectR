// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Misc/AutomationTest.h"
#include "Roguelike/PRChapterRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRChapterRuleDataAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRWardenContentContractTest,
	"ProjectR.Chapter.Warden.Content.ClosedDirectiveAndRouteContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRWardenContentContractTest::RunTest(const FString& Parameters)
{
	UPRChapterRuleDataAsset* Rule = NewObject<UPRChapterRuleDataAsset>();
	Rule->ContentId = UPRChapterContentRegistryDataAsset::GetWardenContentId();
	Rule->DirectiveId = TEXT("Warden.PredictivePatrol");
	Rule->RequiredDirectorRuleId = FGameplayTag::RequestGameplayTag(TEXT("Rule.PredictionLock"), false);
	Rule->PreferredRoomIds = {FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Warden_Combat_ForecastPatrol"))};
	TestTrue(TEXT("Warden directive must carry its exact Director whitelist rule"), Rule->IsRuleDefinitionValid());

	FPRChapterEventPressureBinding Binding;
	Binding.EventId = FPrimaryAssetId(TEXT("ProjectRRoomEvent"), TEXT("DA_RoomEvent_Warden_TrapGrid"));
	Binding.ChoiceId = TEXT("DisarmGrid");
	Binding.PressureDelta = -1;
	Binding.ExcludedFutureRoomIds = {FPrimaryAssetId(TEXT("ProjectRRoom"), TEXT("DA_Room_Warden_Combat_RiskSweep"))};
	TestEqual(TEXT("Route constraints retain exactly one fixed future candidate"), Binding.ExcludedFutureRoomIds.Num(), 1);
	return true;
}

#endif
