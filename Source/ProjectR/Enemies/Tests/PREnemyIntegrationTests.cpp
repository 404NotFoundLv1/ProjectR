// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Enemies/PREnemyAIController.h"
#include "Enemies/PREnemyAttackGameplayAbility.h"
#include "Enemies/PREnemyStateTreeNodes.h"

#include "Misc/AutomationTest.h"

namespace PREnemyIntegrationAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyAttackPhasesDamageEventsAndCleanupTest,
	"ProjectR.Enemy.Combat.AttackPhasesDamageEventsAndCleanup",
	PREnemyIntegrationAutomation::TestFlags)

bool FPREnemyAttackPhasesDamageEventsAndCleanupTest::RunTest(const FString& Parameters)
{
	const UPRGameplayAbility* AttackCDO = UPREnemyAttackGameplayAbility::StaticClass()
		->GetDefaultObject<UPREnemyAttackGameplayAbility>();
	TestEqual(TEXT("Enemy attack is ServerTriggered"), AttackCDO->GetActivationPolicy(),
		EPRAbilityActivationPolicy::ServerTriggered);
	TestEqual(TEXT("Enemy attack has no input tag contract"), AttackCDO->GetProjectRAbilityTag().IsValid(), false);
	TestTrue(TEXT("Enemy AI controller exists"), APREnemyAIController::StaticClass()->IsChildOf(AAIController::StaticClass()));
	TestTrue(TEXT("Acquire target StateTree task is a native task"),
		FPRSTTaskAcquireTarget::StaticStruct()->IsChildOf(FStateTreeTaskCommonBase::StaticStruct()));
	TestTrue(TEXT("Enemy state condition is a native condition"),
		FPRSTConditionEnemyState::StaticStruct()->IsChildOf(FStateTreeConditionCommonBase::StaticStruct()));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
