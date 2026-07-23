// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Companions/PRCompanionComponent.h"
#include "Companions/PRCompanionPawn.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

namespace PRCompanionLifecycleAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionNoKillContractTest,
	"ProjectR.Companion.Lifecycle.NoKillHealthFloors",
	PRCompanionLifecycleAutomation::TestFlags)

bool FPRCompanionNoKillContractTest::RunTest(const FString& Parameters)
{
	const FGameplayTag NormalTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion"), false);
	const FGameplayTag EliteTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard"), false);
	TestEqual(TEXT("Normal enemy support floor is twenty percent"),
		UPRCompanionComponent::CalculateHealthFloor(NormalTag, 100.0f), 20.0f);
	TestEqual(TEXT("Elite enemy support floor is thirty-five percent"),
		UPRCompanionComponent::CalculateHealthFloor(EliteTag, 100.0f), 35.0f);
	TestEqual(TEXT("Damage respects existing Shield before Health floor"),
		UPRCompanionComponent::ClampNoKillDamage(12.0f, 6.0f, 25.0f, 20.0f), 11.0f);
	TestEqual(TEXT("Damage is skipped at the exact health floor"),
		UPRCompanionComponent::ClampNoKillDamage(12.0f, 0.0f, 20.0f, 20.0f), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionPawnMovementContractTest,
	"ProjectR.Companion.Lifecycle.PawnFollowAndCollisionContract",
	PRCompanionLifecycleAutomation::TestFlags)

bool FPRCompanionPawnMovementContractTest::RunTest(const FString& Parameters)
{
	APRCompanionPawn* Pawn = NewObject<APRCompanionPawn>();
	TestNotNull(TEXT("Companion pawn is constructible without an ASC"), Pawn);
	if (!Pawn) return false;
	TestFalse(TEXT("Companion pawn has no permanent actor Tick"), Pawn->PrimaryActorTick.bCanEverTick);
	TestNotNull(TEXT("Companion has a non-combat runtime component"), Pawn->GetCompanionComponent());
	TestEqual(TEXT("Companion uses fixed follow speed"), Pawn->GetCharacterMovement()->MaxFlySpeed, 550.0f);
	TestTrue(TEXT("Companion movement is constrained to the X/Z plane"), Pawn->GetCharacterMovement()->bConstrainToPlane);
	TestEqual(TEXT("Companion ignores Pawn collision"), Pawn->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
	TestEqual(TEXT("Companion sweeps WorldStatic only"), Pawn->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_WorldStatic), ECR_Block);
	TestFalse(TEXT("Companion creates no overlap gameplay"), Pawn->GetCapsuleComponent()->GetGenerateOverlapEvents());
	return true;
}

#endif
