// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillTypes.h"
#include "Core/PRTagLibrary.h"
#include "Combat/PRCombatTypes.h"

#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"

namespace PRPlayerSkillAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerSkillSchemaTest,
	"ProjectR.PlayerSkill.Schema.TypesTagsAndDataValidation",
	PRPlayerSkillAutomation::TestFlags)

bool FPRPlayerSkillSchemaTest::RunTest(const FString& Parameters)
{
	const FPRAbilityDisplacementRequest DisplacementDefaults;
	TestFalse(TEXT("Displacement source defaults null"), DisplacementDefaults.SourceActor.IsValid());
	TestFalse(TEXT("Displacement target defaults null"), DisplacementDefaults.TargetActor.IsValid());
	TestEqual(TEXT("Displacement duration defaults zero"), DisplacementDefaults.Duration, 0.0f);
	TestTrue(TEXT("Displacement sweeps by default"), DisplacementDefaults.bSweep);
	TestTrue(TEXT("Displacement stops on blocking hit by default"), DisplacementDefaults.bStopOnBlockingHit);

	const FPRPlayerSkillExecutionSnapshot SnapshotDefaults;
	TestFalse(TEXT("Snapshot ability tag defaults invalid"), SnapshotDefaults.AbilityTag.IsValid());
	TestEqual(TEXT("Snapshot attack power defaults zero"), SnapshotDefaults.AttackPower, 0.0f);
	TestFalse(TEXT("Snapshot critical defaults false"), SnapshotDefaults.bCritical);

	TestEqual(TEXT("Applied status remains zero"), static_cast<uint8>(EPRCombatRequestStatus::Applied), static_cast<uint8>(0));
	TestEqual(TEXT("Invulnerable status remains one"), static_cast<uint8>(EPRCombatRequestStatus::RejectedInvulnerable), static_cast<uint8>(1));
	TestEqual(TEXT("Dead status remains two"), static_cast<uint8>(EPRCombatRequestStatus::RejectedDead), static_cast<uint8>(2));
	TestEqual(TEXT("Alive status remains three"), static_cast<uint8>(EPRCombatRequestStatus::RejectedAlive), static_cast<uint8>(3));
	TestEqual(TEXT("Invalid status remains four"), static_cast<uint8>(EPRCombatRequestStatus::Invalid), static_cast<uint8>(4));
	TestEqual(TEXT("Blocked status appends as five"), static_cast<uint8>(EPRCombatRequestStatus::RejectedBlocked), static_cast<uint8>(5));

	const TPair<const TCHAR*, const FGameplayTag&> RuntimeTags[] = {
		{TEXT("Skill.BasicAttack"), UPRTagLibrary::GetSkillBasicAttackTag()},
		{TEXT("Input.Skill.ShadowThrust"), UPRTagLibrary::GetInputSkillShadowThrustTag()},
		{TEXT("Input.Skill.FireSlash"), UPRTagLibrary::GetInputSkillFireSlashTag()},
		{TEXT("Input.Skill.ThunderDrop"), UPRTagLibrary::GetInputSkillThunderDropTag()},
		{TEXT("Input.Skill.VectorHook"), UPRTagLibrary::GetInputSkillVectorHookTag()},
		{TEXT("Input.Skill.CounterProofWall"), UPRTagLibrary::GetInputSkillCounterProofWallTag()},
		{TEXT("Ability.ActivateFail.NoTarget"), UPRTagLibrary::GetAbilityActivateFailNoTargetTag()},
		{TEXT("Ability.ActivateFail.Obstructed"), UPRTagLibrary::GetAbilityActivateFailObstructedTag()},
		{TEXT("Ability.ActivateFail.InvalidMovement"), UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag()},
		{TEXT("Ability.State.PlayerSkillActive"), UPRTagLibrary::GetAbilityStatePlayerSkillActiveTag()},
		{TEXT("Combat.Event.AbilityOutcome"), UPRTagLibrary::GetCombatEventAbilityOutcomeTag()},
		{TEXT("Combat.Response.DisplacementApplied"), UPRTagLibrary::GetCombatResponseDisplacementAppliedTag()},
		{TEXT("Combat.Response.DecoyCreated"), UPRTagLibrary::GetCombatResponseDecoyCreatedTag()},
		{TEXT("Combat.Response.DecoyConsumed"), UPRTagLibrary::GetCombatResponseDecoyConsumedTag()},
		{TEXT("Combat.Response.GuardBlocked"), UPRTagLibrary::GetCombatResponseGuardBlockedTag()},
		{TEXT("Combat.Response.PerfectTiming"), UPRTagLibrary::GetCombatResponsePerfectTimingTag()},
		{TEXT("State.Burning"), UPRTagLibrary::GetStateBurningTag()},
		{TEXT("State.Guarding"), UPRTagLibrary::GetStateGuardingTag()}};
	for (const TPair<const TCHAR*, const FGameplayTag&>& Entry : RuntimeTags)
	{
		TestTrue(Entry.Key, Entry.Value.IsValid());
		TestEqual(Entry.Key, Entry.Value.ToString(), FString(Entry.Key));
	}
	const TCHAR* CooldownTags[] = {
		TEXT("Cooldown.Skill.ShadowThrust"), TEXT("Cooldown.Skill.FireSlash"),
		TEXT("Cooldown.Skill.ThunderDrop"), TEXT("Cooldown.Skill.AfterimageDodge"),
		TEXT("Cooldown.Skill.VectorHook"), TEXT("Cooldown.Skill.CounterProofWall")};
	for (const TCHAR* TagName : CooldownTags)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		TestTrue(TagName, Tag.IsValid());
		TestEqual(TagName, Tag.ToString(), FString(TagName));
	}

	UPRPlayerSkillDataAsset* SkillData = NewObject<UPRPlayerSkillDataAsset>();
	TestEqual(TEXT("Skill primary asset type"), SkillData->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("ProjectRPlayerSkill")));
#if WITH_EDITOR
	FDataValidationContext InvalidContext;
	TestEqual(TEXT("Empty skill data is invalid"),
		SkillData->IsDataValid(InvalidContext), EDataValidationResult::Invalid);
#endif

	const UPRFireSlashSkillFragment* Fire = NewObject<UPRFireSlashSkillFragment>();
	TestEqual(TEXT("Burning base damage"), Fire->BurningBaseDamage, 4.0f);
	TestEqual(TEXT("Burning scale"), Fire->BurningAttackPowerScale, 0.1f);
	TestEqual(TEXT("Burning interval"), Fire->BurningTickInterval, 0.5f);
	TestEqual(TEXT("Burning tick count"), Fire->BurningTickCount, 3);
	TestEqual(TEXT("Thunder fallback"), NewObject<UPRThunderDropSkillFragment>()->FallbackDistance, 450.0f);
	TestEqual(TEXT("Afterimage perfect window"),
		NewObject<UPRAfterimageDodgeSkillFragment>()->PerfectTimingWindow, 0.14f);
	TestEqual(TEXT("Hook stop distance"), NewObject<UPRVectorHookSkillFragment>()->StopDistance, 120.0f);
	TestEqual(TEXT("Wall perfect window"),
		NewObject<UPRCounterProofWallSkillFragment>()->PerfectTimingWindow, 0.15f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
