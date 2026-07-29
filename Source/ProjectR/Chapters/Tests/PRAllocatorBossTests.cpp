// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/Allocator/PRAllocatorBossComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRAllocatorBossContractTest, "ProjectR.Chapter.AllocatorBoss.BoundedRuntimeContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRAllocatorBossContractTest::RunTest(const FString& Parameters)
{
	UPRAllocatorBossComponent* Component = NewObject<UPRAllocatorBossComponent>();
	const FPRAllocatorBossRuntimeState& State = Component->GetRuntimeState();
	TestEqual(TEXT("Allocator phase starts dormant"), State.Phase, EPRAllocatorBossPhase::Dormant);
	TestEqual(TEXT("Allocation pressure starts bounded at zero"), State.AllocationPressure, 0);
	TestEqual(TEXT("Audit units start at zero"), State.AuditUnits, 0);
	return true;
}

#endif
