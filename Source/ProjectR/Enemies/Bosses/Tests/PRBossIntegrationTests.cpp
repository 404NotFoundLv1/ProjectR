// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Enemies/Bosses/PRAuditorBossDataAsset.h"
#include "Enemies/Bosses/PRBossPrototypeGameMode.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "UI/PRBossHealthWidget.h"
#include "UI/PRBossPrototypeResultWidget.h"

#include "Misc/AutomationTest.h"

namespace PRBossIntegrationAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

template <typename T>
const T* LoadAsset(const TCHAR* Path)
{
	return LoadObject<T>(nullptr, Path);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAuditorBossIntegrationAssetTest,
	"ProjectR.Boss.Integration.GameModeWidgetsAndCleanup",
	PRBossIntegrationAutomation::TestFlags)

bool FPRAuditorBossIntegrationAssetTest::RunTest(const FString& Parameters)
{
	using namespace PRBossIntegrationAutomation;
	const UPREnemyPrototypeRegistryDataAsset* Registry = LoadAsset<UPREnemyPrototypeRegistryDataAsset>(
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	TestNotNull(TEXT("Fixed Enemy registry exists"), Registry);
	if (Registry)
	{
		const TArray<FPREnemyPrototypeRegistryEntry>& Entries = Registry->GetEntries();
		TestEqual(TEXT("Registry has the four baseline prototypes plus Auditor"), Entries.Num(), 5);
		const TArray<FGameplayTag> ExpectedTags = {
			FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")),
			FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion")),
			FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.ShieldMinion")),
			FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard")),
			FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.AuditorBoss"))};
		for (int32 Index = 0; Index < FMath::Min(Entries.Num(), ExpectedTags.Num()); ++Index)
		{
			TestEqual(FString::Printf(TEXT("Registry tag %d preserves the fixed whitelist order"), Index), Entries[Index].PrototypeTag, ExpectedTags[Index]);
		}
	}

	const UPRAuditorBossDataAsset* Prototype = LoadAsset<UPRAuditorBossDataAsset>(
		TEXT("/Game/ProjectR/Enemies/Bosses/Prototypes/DA_Boss_Auditor.DA_Boss_Auditor"));
	TestNotNull(TEXT("Auditor prototype asset exists"), Prototype);
	if (Prototype)
	{
		TestEqual(TEXT("Boss Health is the frozen P0 value"), Prototype->Attributes.Health, 900.0f);
		TestEqual(TEXT("Boss MaxHealth is the frozen P0 value"), Prototype->Attributes.MaxHealth, 900.0f);
		TestEqual(TEXT("Boss Shield is the frozen P0 value"), Prototype->Attributes.Shield, 120.0f);
		TestEqual(TEXT("Boss MaxShield is the frozen P0 value"), Prototype->Attributes.MaxShield, 120.0f);
		TestEqual(TEXT("Boss AttackPower is the frozen P0 value"), Prototype->Attributes.AttackPower, 18.0f);
		TestEqual(TEXT("Boss MoveSpeed is the frozen P0 value"), Prototype->Attributes.MoveSpeed, 320.0f);
		TestEqual(TEXT("Boss acquire range is frozen"), Prototype->Perception.AcquireRange, 1600.0f);
		TestEqual(TEXT("Boss lose range is frozen"), Prototype->Perception.LoseRange, 1900.0f);
		TestEqual(TEXT("Boss preferred min range is frozen"), Prototype->Perception.PreferredMinRange, 180.0f);
		TestEqual(TEXT("Boss preferred max range is frozen"), Prototype->Perception.PreferredMaxRange, 650.0f);
		TestEqual(TEXT("Boss fragment result remains one"), Prototype->CounterproofFragmentsAwarded, 1);
	}

	TestTrue(TEXT("Boss prototype GameMode uses the formal run GameMode base"),
		APRBossPrototypeGameMode::StaticClass()->IsChildOf(APRNetworkRunGameMode::StaticClass()));
	TestTrue(TEXT("Boss health widget is read-only UserWidget presentation"),
		UPRBossHealthWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	TestTrue(TEXT("Prototype result widget is read-only UserWidget presentation"),
		UPRBossPrototypeResultWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
