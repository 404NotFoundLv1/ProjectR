// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPlaneMovementComponent.h"
#include "Enemies/PREnemySubsystem.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAttributeSet.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyCheckpointBAssetManifestTest,
	"ProjectR.Enemy.Assets.CheckpointBManifest",
	PREnemyArchetypeAutomation::TestFlags)

bool FPREnemyCheckpointBAssetManifestTest::RunTest(const FString& Parameters)
{
	const UClass* ProjectileClass = FindObject<UClass>(nullptr, TEXT("/Script/ProjectR.PREnemyProjectile"));
	TestNotNull(TEXT("Native ranged projectile class exists"), ProjectileClass);

	const UPREnemyPrototypeRegistryDataAsset* Registry = LoadObject<UPREnemyPrototypeRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	const UPREnemyPrototypeDataAsset* Ranged = LoadObject<UPREnemyPrototypeDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_RangedMinion.DA_Enemy_RangedMinion"));
	const UPREnemyAttackDataAsset* Shot = LoadObject<UPREnemyAttackDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_RangedShot.DA_EnemyAttack_RangedShot"));
	const UPRAbilitySetDataAsset* AbilitySet = LoadObject<UPRAbilitySetDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Abilities/DA_EnemyAbilitySet_Ranged.DA_EnemyAbilitySet_Ranged"));
	const UBlueprint* RangedBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_RangedMinion.BP_Enemy_RangedMinion"));
	const UBlueprint* ProjectileBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_EnemyProjectile_Ranged.BP_EnemyProjectile_Ranged"));
	const UBlueprint* CooldownBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_RangedShot_Cooldown.GE_EnemyAttack_RangedShot_Cooldown"));

	TestNotNull(TEXT("Ranged prototype exists"), Ranged);
	TestNotNull(TEXT("Ranged attack data exists"), Shot);
	TestNotNull(TEXT("Ranged ability set exists"), AbilitySet);
	TestNotNull(TEXT("Ranged enemy Blueprint exists"), RangedBlueprint);
	TestNotNull(TEXT("Ranged projectile Blueprint exists"), ProjectileBlueprint);
	TestNotNull(TEXT("Ranged cooldown Blueprint exists"), CooldownBlueprint);
	TestNotNull(TEXT("Ranged VFX exists"), LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ProjectR/Enemies/VFX/VFX_Enemy_RangedShot.VFX_Enemy_RangedShot")));
	TestNotNull(TEXT("Ranged SFX exists"), LoadObject<USoundWave>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Audio/SFX_Enemy_RangedShot.SFX_Enemy_RangedShot")));

	if (Registry)
	{
		TestEqual(TEXT("Registry contains Melee and Ranged entries"), Registry->GetEntries().Num(), 2);
		if (Registry->GetEntries().Num() == 2)
		{
			TestEqual(TEXT("Melee entry remains first"), Registry->GetEntries()[0].PrototypeTag,
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")));
			TestEqual(TEXT("Ranged entry is appended"), Registry->GetEntries()[1].PrototypeTag,
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion")));
		}
	}
	if (Ranged)
	{
		TestEqual(TEXT("Ranged health is fixed"), Ranged->Attributes.Health, 80.0f);
		TestEqual(TEXT("Ranged max health is fixed"), Ranged->Attributes.MaxHealth, 80.0f);
		TestEqual(TEXT("Ranged move speed is fixed"), Ranged->Attributes.MoveSpeed, 380.0f);
		TestEqual(TEXT("Ranged acquire range is fixed"), Ranged->Perception.AcquireRange, 1100.0f);
		TestEqual(TEXT("Ranged lose range is fixed"), Ranged->Perception.LoseRange, 1400.0f);
		TestEqual(TEXT("Ranged preferred minimum is fixed"), Ranged->Perception.PreferredMinRange, 350.0f);
		TestEqual(TEXT("Ranged preferred maximum is fixed"), Ranged->Perception.PreferredMaxRange, 700.0f);
	}
	if (Shot)
	{
		TestEqual(TEXT("Ranged attack is projectile"), Shot->Kind, EPREnemyAttackKind::Projectile);
		TestEqual(TEXT("Ranged minimum range is fixed"), Shot->MinRange, 350.0f);
		TestEqual(TEXT("Ranged maximum range is fixed"), Shot->MaxRange, 700.0f);
		TestEqual(TEXT("Ranged base damage is fixed"), Shot->BaseDamage, 8.0f);
		TestEqual(TEXT("Ranged AP scale is fixed"), Shot->AttackPowerScale, 0.4f);
		TestEqual(TEXT("Ranged projectile speed is fixed"), Shot->ProjectileSpeed, 1200.0f);
		TestEqual(TEXT("Ranged projectile lifetime is fixed"), Shot->ProjectileLifetime, 1.5f);
	}
	const UBlueprint* DefaultAttributesBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"));
	const UGameplayEffect* DefaultAttributes = DefaultAttributesBlueprint && DefaultAttributesBlueprint->GeneratedClass
		? DefaultAttributesBlueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	TestNotNull(TEXT("Shared enemy default-attributes GE exists for Ranged P0 initialization"), DefaultAttributes);
	if (DefaultAttributes)
	{
		int32 HealthIndex = INDEX_NONE;
		int32 MaxHealthIndex = INDEX_NONE;
		int32 ShieldIndex = INDEX_NONE;
		int32 MaxShieldIndex = INDEX_NONE;
		int32 EnergyIndex = INDEX_NONE;
		int32 MaxEnergyIndex = INDEX_NONE;
		for (int32 Index = 0; Index < DefaultAttributes->Modifiers.Num(); ++Index)
		{
			const FGameplayAttribute Attribute = DefaultAttributes->Modifiers[Index].Attribute;
			if (Attribute == UPRAttributeSet::GetHealthAttribute()) HealthIndex = Index;
			else if (Attribute == UPRAttributeSet::GetMaxHealthAttribute()) MaxHealthIndex = Index;
			else if (Attribute == UPRAttributeSet::GetShieldAttribute()) ShieldIndex = Index;
			else if (Attribute == UPRAttributeSet::GetMaxShieldAttribute()) MaxShieldIndex = Index;
			else if (Attribute == UPRAttributeSet::GetEnergyAttribute()) EnergyIndex = Index;
			else if (Attribute == UPRAttributeSet::GetMaxEnergyAttribute()) MaxEnergyIndex = Index;
		}
		TestTrue(TEXT("Default GE applies MaxHealth before Health for non-default P0 values"),
			MaxHealthIndex != INDEX_NONE && HealthIndex != INDEX_NONE && MaxHealthIndex < HealthIndex);
		TestTrue(TEXT("Default GE applies MaxShield before Shield for non-default P0 values"),
			MaxShieldIndex != INDEX_NONE && ShieldIndex != INDEX_NONE && MaxShieldIndex < ShieldIndex);
		TestTrue(TEXT("Default GE applies MaxEnergy before Energy for non-default P0 values"),
			MaxEnergyIndex != INDEX_NONE && EnergyIndex != INDEX_NONE && MaxEnergyIndex < EnergyIndex);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
