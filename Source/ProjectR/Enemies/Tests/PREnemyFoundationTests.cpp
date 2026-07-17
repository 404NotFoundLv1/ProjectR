// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Enemies/PREnemyTypes.h"
#include "Core/PRTagLibrary.h"

#include "Misc/AutomationTest.h"

namespace PREnemyAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemySchemaTypesAndDataValidationTest,
	"ProjectR.Enemy.Schema.TypesTagsAndDataValidation",
	PREnemyAutomation::TestFlags)

bool FPREnemySchemaTypesAndDataValidationTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Dormant is the first stable enemy brain state"),
		static_cast<uint8>(EPREnemyBrainState::Dormant), static_cast<uint8>(0));
	TestEqual(TEXT("Dead is the final stable enemy brain state"),
		static_cast<uint8>(EPREnemyBrainState::Dead), static_cast<uint8>(7));
	TestEqual(TEXT("MeleeSweep is the first stable attack kind"),
		static_cast<uint8>(EPREnemyAttackKind::MeleeSweep), static_cast<uint8>(0));

	const FPREnemyRuntimeState RuntimeState;
	TestFalse(TEXT("Runtime SpawnId defaults invalid"), RuntimeState.SpawnId.IsValid());
	TestEqual(TEXT("Runtime defaults Dormant"), RuntimeState.BrainState, EPREnemyBrainState::Dormant);
	TestEqual(TEXT("Runtime defaults without an active attack"), RuntimeState.AttackPhase, EPREnemyAttackPhase::None);
	TestFalse(TEXT("Runtime defaults not alive"), RuntimeState.bAlive);

	UPREnemyPrototypeDataAsset* Prototype = NewObject<UPREnemyPrototypeDataAsset>();
	UPREnemyAttackDataAsset* Attack = NewObject<UPREnemyAttackDataAsset>();
	UPREnemyPrototypeRegistryDataAsset* Registry = NewObject<UPREnemyPrototypeRegistryDataAsset>();
	TestEqual(TEXT("Prototype primary asset type"), Prototype->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("ProjectREnemyPrototype")));
	TestEqual(TEXT("Attack primary asset type"), Attack->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("ProjectREnemyAttack")));
	TestEqual(TEXT("Registry primary asset type"), Registry->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("ProjectREnemyPrototypeRegistry")));
	TestTrue(TEXT("Enemy health SetByCaller tag is available"), UPRTagLibrary::GetEnemyDataHealthTag().IsValid());
	TestTrue(TEXT("Enemy reevaluate event tag is available"), UPRTagLibrary::GetEnemyAIEventReevaluateTag().IsValid());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
