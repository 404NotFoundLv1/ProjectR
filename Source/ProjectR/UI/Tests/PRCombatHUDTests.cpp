// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

#include "GameFramework/HUD.h"
#include "UI/PRCombatHUD.h"
#include "UI/PRCombatHUDWidget.h"
#include "UI/PRCombatFeedbackWidget.h"
#include "UI/PRBossHealthWidget.h"
#include "UI/PRBossPrototypeResultWidget.h"
#include "UI/PRPlayerResourcesWidget.h"
#include "UI/PRSkillBarWidget.h"
#include "UI/PRSkillSlotWidget.h"

namespace PRCombatHUDAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatHUDSchemaRedTest,
	"ProjectR.UI.CombatHUD.SchemaOwnership",
    PRCombatHUDAutomation::TestFlags)

bool FPRCombatHUDSchemaRedTest::RunTest(const FString& Parameters)
{
	const UClass* CombatHUDClass = FindObject<UClass>(nullptr, TEXT("/Script/ProjectR.PRCombatHUD"));
	TestNotNull(TEXT("Combat HUD native owner exists"), CombatHUDClass);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatHUDOwnershipTest,
	"ProjectR.UI.CombatHUD.OwnershipAndReadOnlyTypes",
	PRCombatHUDAutomation::TestFlags)

bool FPRCombatHUDOwnershipTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Combat HUD derives from Engine HUD"), APRCombatHUD::StaticClass()->IsChildOf(AHUD::StaticClass()));
	TestTrue(TEXT("Root combat HUD is a UserWidget"), UPRCombatHUDWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	TestTrue(TEXT("Resource presentation is a UserWidget"), UPRPlayerResourcesWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	TestTrue(TEXT("Skill bar is a UserWidget"), UPRSkillBarWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	TestTrue(TEXT("Skill slot is a UserWidget"), UPRSkillSlotWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	TestTrue(TEXT("Feedback stream is a UserWidget"), UPRCombatFeedbackWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatHUDResourceSchemaTest,
	"ProjectR.UI.CombatHUD.ResourcesAndSkillRuntimeSchema",
	PRCombatHUDAutomation::TestFlags)

bool FPRCombatHUDResourceSchemaTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Resource Health bar contract"), FindFProperty<FProperty>(UPRPlayerResourcesWidget::StaticClass(), TEXT("HealthBar")));
	TestNotNull(TEXT("Resource Shield bar contract"), FindFProperty<FProperty>(UPRPlayerResourcesWidget::StaticClass(), TEXT("ShieldBar")));
	TestNotNull(TEXT("Resource Energy bar contract"), FindFProperty<FProperty>(UPRPlayerResourcesWidget::StaticClass(), TEXT("EnergyBar")));
	TestNotNull(TEXT("Permission presentation contract"), FindFProperty<FProperty>(UPRPlayerResourcesWidget::StaticClass(), TEXT("PermissionText")));
	TestNotNull(TEXT("Resonance presentation contract"), FindFProperty<FProperty>(UPRPlayerResourcesWidget::StaticClass(), TEXT("ResonanceText")));
	TestNotNull(TEXT("Skill bar hard AbilitySet reference"), FindFProperty<FProperty>(UPRSkillBarWidget::StaticClass(), TEXT("DefaultAbilitySet")));
	TestNotNull(TEXT("Skill slot failure presentation"), FindFProperty<FProperty>(UPRSkillSlotWidget::StaticClass(), TEXT("FailureText")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatHUDFeedbackSchemaTest,
	"ProjectR.UI.CombatHUD.FeedbackEventBoundary",
	PRCombatHUDAutomation::TestFlags)

bool FPRCombatHUDFeedbackSchemaTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Feedback display container contract"), FindFProperty<FProperty>(UPRCombatFeedbackWidget::StaticClass(), TEXT("FeedbackEntries")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatHUDSkillDisplayNameTest,
	"ProjectR.UI.CombatHUD.SkillDisplayNameMapping",
	PRCombatHUDAutomation::TestFlags)

bool FPRCombatHUDSkillDisplayNameTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ShadowThrust uses its stable display name"), UPRSkillSlotWidget::GetDisplayNameForAbilityTag(FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"))).ToString(), FString(TEXT("Shadow Thrust")));
	TestEqual(TEXT("FireSlash uses its stable display name"), UPRSkillSlotWidget::GetDisplayNameForAbilityTag(FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash"))).ToString(), FString(TEXT("Fire Slash")));
	TestEqual(TEXT("ThunderDrop uses its stable display name"), UPRSkillSlotWidget::GetDisplayNameForAbilityTag(FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop"))).ToString(), FString(TEXT("Thunder Drop")));
	TestEqual(TEXT("AfterimageDodge uses its stable display name"), UPRSkillSlotWidget::GetDisplayNameForAbilityTag(FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"))).ToString(), FString(TEXT("Afterimage Dodge")));
	TestEqual(TEXT("VectorHook uses its stable display name"), UPRSkillSlotWidget::GetDisplayNameForAbilityTag(FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook"))).ToString(), FString(TEXT("Vector Hook")));
	TestEqual(TEXT("CounterProofWall uses its stable display name"), UPRSkillSlotWidget::GetDisplayNameForAbilityTag(FGameplayTag::RequestGameplayTag(TEXT("Skill.CounterProofWall"))).ToString(), FString(TEXT("Counterproof Wall")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatHUDBossWidgetBindingTest,
	"ProjectR.UI.CombatHUD.BossWidgetBindingSchema",
	PRCombatHUDAutomation::TestFlags)

bool FPRCombatHUDBossWidgetBindingTest::RunTest(const FString& Parameters)
{
	auto HasOptionalBinding = [this](const UClass* Class, const TCHAR* Name)
	{
		const FProperty* Property = FindFProperty<FProperty>(Class, Name);
		return Property && Property->HasMetaData(TEXT("BindWidgetOptional"));
	};
	TestTrue(TEXT("Boss health text binds to the WidgetBlueprint"), HasOptionalBinding(UPRBossHealthWidget::StaticClass(), TEXT("HealthShieldText")));
	TestTrue(TEXT("Boss phase text binds to the WidgetBlueprint"), HasOptionalBinding(UPRBossHealthWidget::StaticClass(), TEXT("PhaseText")));
	TestTrue(TEXT("Boss rule text binds to the WidgetBlueprint"), HasOptionalBinding(UPRBossHealthWidget::StaticClass(), TEXT("RuleText")));
	TestTrue(TEXT("Boss prediction text binds to the WidgetBlueprint"), HasOptionalBinding(UPRBossHealthWidget::StaticClass(), TEXT("PredictionText")));
	TestTrue(TEXT("Boss attack text binds to the WidgetBlueprint"), HasOptionalBinding(UPRBossHealthWidget::StaticClass(), TEXT("AttackText")));
	TestTrue(TEXT("Boss result text binds to the WidgetBlueprint"), HasOptionalBinding(UPRBossPrototypeResultWidget::StaticClass(), TEXT("PrototypeResultText")));
	return true;
}
