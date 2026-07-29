// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/Warden/PRWardenBossComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRWardenBossContractTest,
	"ProjectR.Chapter.Warden.WardenBoss.BoundedRuntimeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRWardenBossContractTest::RunTest(const FString& Parameters)
{
	UPRWardenBossComponent* Component = NewObject<UPRWardenBossComponent>();
	const FPRWardenBossRuntimeState& State = Component->GetRuntimeState();
	TestEqual(TEXT("Warden starts dormant"), State.Phase, EPRWardenBossPhase::Dormant);
	TestEqual(TEXT("Warden starts with no risk pressure"), State.RiskPressure, 0);
	TestEqual(TEXT("Warden starts with no risk layers"), State.RiskLayers, 0);
	Component->ConfigureChapterState(7);
	TestEqual(TEXT("Only bounded Warden risk pressure reaches the Boss contract"), Component->GetRuntimeState().RiskPressure, 4);
	return true;
}

#endif
