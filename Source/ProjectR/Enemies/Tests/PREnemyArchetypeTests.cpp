// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPlaneMovementComponent.h"
#include "Enemies/PREnemySubsystem.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Abilities/PRAbilitySetDataAsset.h"
#include "GameplayEffect.h"
#include "Engine/Blueprint.h"
#include "NiagaraSystem.h"
#include "Sound/SoundWave.h"
#include "StateTree.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRCombatFeedbackInterface.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"

#include "Misc/AutomationTest.h"

namespace PREnemyArchetypeAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyGASOwnerAvatarAttributesAbilityGrantTest,
	"ProjectR.Enemy.GAS.OwnerAvatarAttributesAbilityGrant",
	PREnemyArchetypeAutomation::TestFlags)

bool FPREnemyGASOwnerAvatarAttributesAbilityGrantTest::RunTest(const FString& Parameters)
{
	const UClass* EnemyClass = APREnemyCharacter::StaticClass();
	TestTrue(TEXT("Enemy derives from Character"), EnemyClass->IsChildOf(ACharacter::StaticClass()));
	TestTrue(TEXT("Enemy implements Combatant"), EnemyClass->ImplementsInterface(UPRCombatantInterface::StaticClass()));
	TestTrue(TEXT("Enemy implements CombatFeedback"), EnemyClass->ImplementsInterface(UPRCombatFeedbackInterface::StaticClass()));
	TestTrue(TEXT("Enemy implements AbilityTarget"), EnemyClass->ImplementsInterface(UPRAbilityTargetInterface::StaticClass()));
	TestTrue(TEXT("Enemy exposes an AbilitySystemInterface"), EnemyClass->ImplementsInterface(UAbilitySystemInterface::StaticClass()));
	TestTrue(TEXT("Enemy subsystem is a world subsystem"),
		UPREnemySubsystem::StaticClass()->IsChildOf(UWorldSubsystem::StaticClass()));
	TestTrue(TEXT("Enemy plane movement derives from CharacterMovement"),
		UPREnemyPlaneMovementComponent::StaticClass()->IsChildOf(UCharacterMovementComponent::StaticClass()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyCheckpointAAssetManifestTest,
	"ProjectR.Enemy.Assets.CheckpointAManifest",
	PREnemyArchetypeAutomation::TestFlags)

bool FPREnemyCheckpointAAssetManifestTest::RunTest(const FString& Parameters)
{
	const UPREnemyPrototypeRegistryDataAsset* Registry = LoadObject<UPREnemyPrototypeRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	const UPREnemyPrototypeDataAsset* Melee = LoadObject<UPREnemyPrototypeDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_MeleeMinion.DA_Enemy_MeleeMinion"));
	const UPREnemyAttackDataAsset* Strike = LoadObject<UPREnemyAttackDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_MeleeStrike.DA_EnemyAttack_MeleeStrike"));
	const UPRAbilitySetDataAsset* AbilitySet = LoadObject<UPRAbilitySetDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Abilities/DA_EnemyAbilitySet_Melee.DA_EnemyAbilitySet_Melee"));
	TestNotNull(TEXT("Fixed prototype registry exists"), Registry);
	TestNotNull(TEXT("Melee prototype exists"), Melee);
	TestNotNull(TEXT("Melee attack data exists"), Strike);
	TestNotNull(TEXT("Melee ability set exists"), AbilitySet);
	const UBlueprint* DefaultAttributesGE = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"));
	const UBlueprint* MeleeCooldownGE = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_MeleeStrike_Cooldown.GE_EnemyAttack_MeleeStrike_Cooldown"));
	TestNotNull(TEXT("Default attribute GE is a configurable GameplayEffect Blueprint"), DefaultAttributesGE);
	TestNotNull(TEXT("Melee cooldown GE is a configurable GameplayEffect Blueprint"), MeleeCooldownGE);
	TestTrue(TEXT("Default attribute GE generated class derives GameplayEffect"),
		DefaultAttributesGE && DefaultAttributesGE->GeneratedClass
		&& DefaultAttributesGE->GeneratedClass->IsChildOf(UGameplayEffect::StaticClass()));
	TestTrue(TEXT("Melee cooldown GE generated class derives GameplayEffect"),
		MeleeCooldownGE && MeleeCooldownGE->GeneratedClass
		&& MeleeCooldownGE->GeneratedClass->IsChildOf(UGameplayEffect::StaticClass()));
	TestNotNull(TEXT("Enemy StateTree exists"), LoadObject<UStateTree>(nullptr,
		TEXT("/Game/ProjectR/Enemies/AI/ST_Enemy_Base.ST_Enemy_Base")));
	TestNotNull(TEXT("Melee VFX exists"), LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ProjectR/Enemies/VFX/VFX_Enemy_MeleeStrike.VFX_Enemy_MeleeStrike")));
	TestNotNull(TEXT("Melee SFX exists"), LoadObject<USoundWave>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Audio/SFX_Enemy_MeleeStrike.SFX_Enemy_MeleeStrike")));
	if (Melee)
	{
		TestEqual(TEXT("Melee health is fixed"), Melee->Attributes.Health, 100.0f);
		TestEqual(TEXT("Melee move speed is fixed"), Melee->Attributes.MoveSpeed, 450.0f);
		TestEqual(TEXT("Melee acquire range is fixed"), Melee->Perception.AcquireRange, 900.0f);
	}
	if (Strike)
	{
		TestEqual(TEXT("Melee raw damage base is fixed"), Strike->BaseDamage, 10.0f);
		TestEqual(TEXT("Melee AP scale is fixed"), Strike->AttackPowerScale, 0.5f);
		TestEqual(TEXT("Melee sweep radius matches the fixed 0-140 range band"), Strike->SweepRadius, 140.0f);
		TestEqual(TEXT("Melee sweep half angle is fixed"), Strike->SweepHalfAngleDegrees, 60.0f);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
