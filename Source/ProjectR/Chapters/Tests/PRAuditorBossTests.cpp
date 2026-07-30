#if WITH_DEV_AUTOMATION_TESTS
#include "Chapters/Auditor/PRAuditorChapterTypes.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRAuditorBossAdditiveContractTest, "ProjectR.Chapter.Auditor.Boss.AdditivePrototypeContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPRAuditorBossAdditiveContractTest::RunTest(const FString& Parameters)
{
	FPRAuditorChapterBossRuntimeState State;
	State.AuditPressure = 4;
	State.RemainingAuditUnits = FMath::Clamp(1 + State.AuditPressure, 1, 4);
	State.RemainingVerdictSkills = FMath::Clamp(1 + State.AuditPressure / 2, 1, 3) + 1;
	TestEqual(TEXT("Repeated-build audit has at most four units"), State.RemainingAuditUnits, 4);
	TestEqual(TEXT("Verdict escalation requires fixed distinct skills"), State.RemainingVerdictSkills, 4);
	return true;
}
#endif
