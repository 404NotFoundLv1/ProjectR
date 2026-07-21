// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Enemies/Bosses/PRAuditorBossComponent.h"
#include "Enemies/Bosses/PRAuditorBossDataAsset.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"

namespace PRBossMechanicsAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

const UPRAuditorBossDataAsset* LoadAuditorPrototype()
{
	return LoadObject<UPRAuditorBossDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Prototypes/DA_Boss_Auditor.DA_Boss_Auditor"));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAuditorBossDeterministicRuleTest,
	"ProjectR.Boss.Mechanics.SamplingRulesAndPrediction",
	PRBossMechanicsAutomation::TestFlags)

bool FPRAuditorBossDeterministicRuleTest::RunTest(const FString& Parameters)
{
	using namespace PRBossMechanicsAutomation;
	const FGameplayTag ShadowThrust = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
	const FGameplayTag FireSlash = FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash"));
	const FGameplayTag Repetition = FGameplayTag::RequestGameplayTag(TEXT("Rule.RepetitionPenalty"));
	const FGameplayTag Distance = FGameplayTag::RequestGameplayTag(TEXT("Rule.DistanceCorrection"));

	const TArray<FPRAuditorBossSample> RepeatedSamples = {
		{ShadowThrust, 240.0f, false, false, 1.0},
		{FireSlash, 300.0f, false, false, 2.0},
		{ShadowThrust, 260.0f, true, false, 3.0},
		{ShadowThrust, 280.0f, false, true, 4.0}};
	TestEqual(TEXT("Three uses select RepetitionPenalty"),
		UPRAuditorBossComponent::SelectRuleAuditRule(RepeatedSamples), Repetition);
	TestEqual(TEXT("Most-used skill is predicted"),
		UPRAuditorBossComponent::SelectPredictedAbilityTag(RepeatedSamples), ShadowThrust);

	const TArray<FPRAuditorBossSample> DistantSamples = {
		{ShadowThrust, 700.0f, false, false, 1.0},
		{FireSlash, 800.0f, false, false, 2.0},
		{ShadowThrust, 900.0f, false, false, 3.0}};
	TestEqual(TEXT("Median distance above 650 selects DistanceCorrection"),
		UPRAuditorBossComponent::SelectRuleAuditRule(DistantSamples), Distance);

	const UPRAuditorBossDataAsset* Prototype = LoadAuditorPrototype();
	TestNotNull(TEXT("Auditor prototype exists"), Prototype);
	if (!Prototype)
	{
		return false;
	}
	TestEqual(TEXT("Auditor has exactly three formal attacks"), Prototype->AttackDefinitions.Num(), 3);
	TestTrue(TEXT("Prediction shield effect is configured"), Prototype->PredictionShieldEffect != nullptr);
	if (Prototype->PredictionShieldEffect)
	{
		const UGameplayEffect* Shield = Prototype->PredictionShieldEffect->GetDefaultObject<UGameplayEffect>();
		TestEqual(TEXT("Prediction shield is Infinite"), Shield->DurationPolicy, EGameplayEffectDurationType::Infinite);
		TestEqual(TEXT("Prediction shield has one persistent MaxShield modifier"), Shield->Modifiers.Num(), 1);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
