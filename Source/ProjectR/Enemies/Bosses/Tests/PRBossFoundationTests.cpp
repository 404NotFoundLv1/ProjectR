// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Enemies/Bosses/PRBossTypes.h"
#include "Enemies/Bosses/PRAuditorBossDataAsset.h"
#include "Enemies/Bosses/PRBossAuditor.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Core/PRTagLibrary.h"

#include "Misc/AutomationTest.h"

namespace PRBossFoundationAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRAuditorBossFoundationContractTest,
	"ProjectR.Boss.Foundation.TypesRegistryAndLifecycle", PRBossFoundationAutomation::TestFlags)

bool FPRAuditorBossFoundationContractTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Dormant remains the first Boss phase"), static_cast<uint8>(EPRAuditorBossPhase::Dormant), static_cast<uint8>(0));
	TestEqual(TEXT("Defeated remains the terminal Boss phase"), static_cast<uint8>(EPRAuditorBossPhase::Defeated), static_cast<uint8>(4));
	TestTrue(TEXT("Auditor derives from the formal Enemy actor"), APRBossAuditor::StaticClass()->IsChildOf(APREnemyCharacter::StaticClass()));
	TestTrue(TEXT("Boss subsystem is world-owned"), UPRBossSubsystem::StaticClass()->IsChildOf(UWorldSubsystem::StaticClass()));
	TestTrue(TEXT("Prediction-blocked tag is available"), UPRTagLibrary::GetCombatResponsePredictionBlockedTag().IsValid());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
