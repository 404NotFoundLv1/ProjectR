// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Companions/PRCompanionTypes.h"
#include "Misc/AutomationTest.h"
#include "QTE/PRQTEDataAsset.h"
#include "QTE/PRQTERegistryDataAsset.h"

namespace PRQTEEffectAutomation
{
constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

struct FExpectedQTE
{
	FName Id;
	FGameplayTag Companion;
	EPRQTETriggerSource Source;
	EPRQTETriggerKind Trigger;
	EPRQTEEffectKind Effect;
	float Magnitude;
	float Range;
	int32 MaxTargets;
	int32 PulseCount;
	EPRQTETargetScope EffectTargetScope;
	float TriggerHealthFraction;
	int32 RequiredDistinctTargets;
	bool bRequireFirstShieldBreak;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRQTEEffectManifestTest, "ProjectR.QTE.Effects.TwelveFixedContracts", PRQTEEffectAutomation::TestFlags)

bool FPRQTEEffectManifestTest::RunTest(const FString& Parameters)
{
	using namespace PRQTEEffectAutomation;
	TestEqual(TEXT("QTE cooldown applies the fixed overload multiplier"), FPRQTEContract::CalculateEffectiveCooldown(24.0f, 50), 36.0f);
	TestEqual(TEXT("QTE cooldown clamps overload to the documented maximum"), FPRQTEContract::CalculateEffectiveCooldown(18.0f, 140), 36.0f);
	const FExpectedQTE Expected[] = {
		{ TEXT("Axiom_WeaknessAxiom"), FPRCompanionContract::AxiomTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::ShieldBroken, EPRQTEEffectKind::Damage, 40.0f, 0.0f, 1, 0, EPRQTETargetScope::EliteOrBoss, 0.0f, 0, true },
		{ TEXT("Axiom_ProbabilityShield"), FPRCompanionContract::AxiomTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::LowHealthDamage, EPRQTEEffectKind::ShieldAndInvulnerable, 20.0f, 0.0f, 1, 0, EPRQTETargetScope::AnyEnemy, 0.35f, 0, false },
		{ TEXT("Axiom_PhaseCut"), FPRCompanionContract::AxiomTag(), EPRQTETriggerSource::CompanionSupportEvent, EPRQTETriggerKind::SupportDuringAuditorWindup, EPRQTEEffectKind::Stun, 0.75f, 0.0f, 1, 0, EPRQTETargetScope::EliteOrBoss, 0.0f, 0, false },
		{ TEXT("Axiom_CooperativeRefutation"), FPRCompanionContract::AxiomTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::PredictionBlocked, EPRQTEEffectKind::FutureSample, 0.0f, 0.0f, 0, 0, EPRQTETargetScope::AnyEnemy, 0.0f, 0, false },
		{ TEXT("Kindle_Breakout"), FPRCompanionContract::KindleTag(), EPRQTETriggerSource::CompanionSupportEvent, EPRQTETriggerKind::KindleCrowdSupport, EPRQTEEffectKind::Stun, 0.75f, 260.0f, 3, 0, EPRQTETargetScope::LightEnemy, 0.0f, 0, false },
		{ TEXT("Kindle_BurnoutTornado"), FPRCompanionContract::KindleTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::FireSlashThreeHits, EPRQTEEffectKind::PulseDamage, 8.0f, 0.0f, 3, 3, EPRQTETargetScope::AnyEnemy, 0.0f, 3, false },
		{ TEXT("Kindle_DetonationRelay"), FPRCompanionContract::KindleTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::ShadowThrustHit, EPRQTEEffectKind::Damage, 35.0f, 0.0f, 1, 0, EPRQTETargetScope::AnyEnemy, 0.0f, 0, false },
		{ TEXT("Kindle_ReverseBurnRescue"), FPRCompanionContract::KindleTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::LowHealthDamage, EPRQTEEffectKind::ShieldAndInvulnerable, 20.0f, 0.0f, 1, 0, EPRQTETargetScope::AnyEnemy, 0.20f, 0, false },
		{ TEXT("Null_VulnerabilityBackstab"), FPRCompanionContract::NullTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::DecoyConsumed, EPRQTEEffectKind::Damage, 25.0f, 450.0f, 1, 0, EPRQTETargetScope::AnyEnemy, 0.0f, 0, false },
		{ TEXT("Null_DecoyJudgment"), FPRCompanionContract::NullTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::PerfectDecoyConsumed, EPRQTEEffectKind::Stun, 0.75f, 450.0f, 1, 0, EPRQTETargetScope::NonBossEnemy, 0.0f, 0, false },
		{ TEXT("Null_GarbageCollection"), FPRCompanionContract::NullTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::NormalLowHealth, EPRQTEEffectKind::ConfirmBasicAttackKill, 0.60f, 0.0f, 1, 0, EPRQTETargetScope::AnyEnemy, 0.20f, 0, false },
		{ TEXT("Null_VerdictTamper"), FPRCompanionContract::NullTag(), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::PredictionBlocked, EPRQTEEffectKind::FutureSample, 0.0f, 0.0f, 0, 0, EPRQTETargetScope::AnyEnemy, 0.0f, 0, false }
	};

	UPRQTERegistryDataAsset* Registry = LoadObject<UPRQTERegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/QTE/DA_QTERegistry.DA_QTERegistry"));
	if (!TestNotNull(TEXT("Twelve-contract validation requires the fixed registry"), Registry)) return false;
	TestEqual(TEXT("The fixed registry has twelve entries"), Registry->GetEntries().Num(), static_cast<int32>(UE_ARRAY_COUNT(Expected)));
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Expected) && Registry->GetEntries().IsValidIndex(Index); ++Index)
	{
		const FExpectedQTE& Contract = Expected[Index];
		UPRQTEDataAsset* Asset = Registry->GetEntries()[Index].LoadSynchronous();
		if (!TestNotNull(*FString::Printf(TEXT("QTE %s loads"), *Contract.Id.ToString()), Asset)) continue;
		TestEqual(TEXT("Fixed identifier"), Asset->QTEId, Contract.Id);
		TestTrue(TEXT("Fixed companion"), Asset->CompanionId.MatchesTagExact(Contract.Companion));
		TestEqual(TEXT("Fixed trigger source"), Asset->TriggerSource, Contract.Source);
		TestEqual(TEXT("Fixed trigger kind"), Asset->TriggerKind, Contract.Trigger);
		if (!TestEqual(TEXT("Each fixed QTE has one effect"), Asset->SuccessEffects.Num(), 1)) continue;
		const FPRQTEEffectDefinition& Effect = Asset->SuccessEffects[0];
		TestEqual(TEXT("Fixed effect kind"), Effect.EffectKind, Contract.Effect);
		TestEqual(TEXT("Fixed effect magnitude"), Effect.Magnitude, Contract.Magnitude);
		TestEqual(TEXT("Fixed effect range"), Effect.Range, Contract.Range);
		TestEqual(TEXT("Fixed effect target cap"), Effect.MaxTargets, Contract.MaxTargets);
		TestEqual(TEXT("Fixed effect pulse count"), Effect.PulseCount, Contract.PulseCount);
		TestEqual(TEXT("Fixed effect target scope"), Effect.TargetScope, Contract.EffectTargetScope);
		TestEqual(TEXT("Fixed low-health trigger fraction"), Asset->TriggerHealthFraction, Contract.TriggerHealthFraction);
		TestEqual(TEXT("Fixed distinct target requirement"), Asset->RequiredDistinctTargetCount, Contract.RequiredDistinctTargets);
		TestEqual(TEXT("Fixed first shield-break requirement"), Asset->bRequireFirstTargetShieldBreak, Contract.bRequireFirstShieldBreak);
	}
	return true;
}

#endif
