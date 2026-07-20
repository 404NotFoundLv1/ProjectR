// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPlaneMovementComponent.h"
#include "Enemies/PREnemySubsystem.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRGameplayAbility.h"
#include "Abilities/PRAttributeSet.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffect.h"
#include "Engine/Blueprint.h"
#include "NiagaraSystem.h"
#include "Sound/SoundWave.h"
#include "StateTree.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRCombatFeedbackInterface.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace PREnemyArchetypeAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

struct FExpectedDefaultAttributeModifier
{
	FGameplayAttribute Attribute;
	FGameplayTag DataTag;
	const TCHAR* Name;
};

TArray<FExpectedDefaultAttributeModifier> GetExpectedDefaultAttributeModifiers()
{
	return {
		{UPRAttributeSet::GetMaxHealthAttribute(), UPRTagLibrary::GetEnemyDataMaxHealthTag(), TEXT("MaxHealth")},
		{UPRAttributeSet::GetHealthAttribute(), UPRTagLibrary::GetEnemyDataHealthTag(), TEXT("Health")},
		{UPRAttributeSet::GetMaxShieldAttribute(), UPRTagLibrary::GetEnemyDataMaxShieldTag(), TEXT("MaxShield")},
		{UPRAttributeSet::GetShieldAttribute(), UPRTagLibrary::GetEnemyDataShieldTag(), TEXT("Shield")},
		{UPRAttributeSet::GetMaxEnergyAttribute(), UPRTagLibrary::GetEnemyDataMaxEnergyTag(), TEXT("MaxEnergy")},
		{UPRAttributeSet::GetEnergyAttribute(), UPRTagLibrary::GetEnemyDataEnergyTag(), TEXT("Energy")},
		{UPRAttributeSet::GetAttackPowerAttribute(), UPRTagLibrary::GetEnemyDataAttackPowerTag(), TEXT("AttackPower")},
		{UPRAttributeSet::GetMoveSpeedAttribute(), UPRTagLibrary::GetEnemyDataMoveSpeedTag(), TEXT("MoveSpeed")},
		{UPRAttributeSet::GetCritChanceAttribute(), UPRTagLibrary::GetEnemyDataCritChanceTag(), TEXT("CritChance")},
		{UPRAttributeSet::GetPermissionAttribute(), UPRTagLibrary::GetEnemyDataPermissionTag(), TEXT("Permission")},
		{UPRAttributeSet::GetResonanceAttribute(), UPRTagLibrary::GetEnemyDataResonanceTag(), TEXT("Resonance")}};
}

int32 GetReflectedArrayNum(const UObject* Object, const FName PropertyName)
{
	const FArrayProperty* Property = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
	if (!Property)
	{
		return INDEX_NONE;
	}
	const void* ArrayAddress = Property->ContainerPtrToValuePtr<void>(Object);
	return FScriptArrayHelper(Property, ArrayAddress).Num();
}
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
		TestTrue(TEXT("Registry retains Melee and Ranged entries before downstream appends"), Registry->GetEntries().Num() >= 2);
		if (Registry->GetEntries().Num() >= 2)
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
		const TArray<PREnemyArchetypeAutomation::FExpectedDefaultAttributeModifier> ExpectedModifiers =
			PREnemyArchetypeAutomation::GetExpectedDefaultAttributeModifiers();
		TestEqual(TEXT("Default GE is Instant"), DefaultAttributes->DurationPolicy,
			EGameplayEffectDurationType::Instant);
		TestEqual(TEXT("Default GE period is zero"), DefaultAttributes->Period.GetValueAtLevel(1.0f), 0.0f);
		TestEqual(TEXT("Default GE has exactly eleven modifiers"), DefaultAttributes->Modifiers.Num(), ExpectedModifiers.Num());
		TestEqual(TEXT("Default GE has no executions"), DefaultAttributes->Executions.Num(), 0);
		TestEqual(TEXT("Default GE has no gameplay cues"), DefaultAttributes->GameplayCues.Num(), 0);
		TestEqual(TEXT("Default GE stacking is none"), DefaultAttributes->GetStackingType(), EGameplayEffectStackingType::None);
		TestEqual(TEXT("Default GE stack limit is zero"), DefaultAttributes->StackLimitCount, 0);
		TestEqual(TEXT("Default GE grants no abilities"),
			PREnemyArchetypeAutomation::GetReflectedArrayNum(DefaultAttributes, TEXT("GrantedAbilities")), 0);
		TestEqual(TEXT("Default GE has no GE components"),
			PREnemyArchetypeAutomation::GetReflectedArrayNum(DefaultAttributes, TEXT("GEComponents")), 0);

		for (int32 Index = 0; Index < FMath::Min(DefaultAttributes->Modifiers.Num(), ExpectedModifiers.Num()); ++Index)
		{
			const FGameplayModifierInfo& Modifier = DefaultAttributes->Modifiers[Index];
			const PREnemyArchetypeAutomation::FExpectedDefaultAttributeModifier& Expected = ExpectedModifiers[Index];
			TestEqual(FString::Printf(TEXT("Default GE modifier %d attribute is %s"), Index, Expected.Name),
				Modifier.Attribute, Expected.Attribute);
			TestEqual(FString::Printf(TEXT("Default GE modifier %d is Override"), Index),
				Modifier.ModifierOp, EGameplayModOp::Override);
			TestEqual(FString::Printf(TEXT("Default GE modifier %d is SetByCaller"), Index),
				Modifier.ModifierMagnitude.GetMagnitudeCalculationType(), EGameplayEffectMagnitudeCalculation::SetByCaller);
			TestEqual(FString::Printf(TEXT("Default GE modifier %d uses the fixed data tag"), Index),
				Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag, Expected.DataTag);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyCheckpointCAssetManifestTest,
	"ProjectR.Enemy.Assets.CheckpointCManifest",
	PREnemyArchetypeAutomation::TestFlags)

bool FPREnemyCheckpointCAssetManifestTest::RunTest(const FString& Parameters)
{
	const UPREnemyPrototypeRegistryDataAsset* Registry = LoadObject<UPREnemyPrototypeRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	const UPREnemyPrototypeDataAsset* Shield = LoadObject<UPREnemyPrototypeDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_ShieldMinion.DA_Enemy_ShieldMinion"));
	const UPREnemyAttackDataAsset* Bash = LoadObject<UPREnemyAttackDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_ShieldBash.DA_EnemyAttack_ShieldBash"));
	const UPRAbilitySetDataAsset* AbilitySet = LoadObject<UPRAbilitySetDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Abilities/DA_EnemyAbilitySet_Shield.DA_EnemyAbilitySet_Shield"));
	const UBlueprint* ShieldBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_ShieldMinion.BP_Enemy_ShieldMinion"));
	const UBlueprint* AbilityBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/GA_Enemy_ShieldBash.GA_Enemy_ShieldBash"));
	const UBlueprint* CooldownBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_ShieldBash_Cooldown.GE_EnemyAttack_ShieldBash_Cooldown"));

	TestNotNull(TEXT("Shield prototype exists"), Shield);
	TestNotNull(TEXT("ShieldBash attack data exists"), Bash);
	TestNotNull(TEXT("Shield ability set exists"), AbilitySet);
	TestNotNull(TEXT("Shield enemy Blueprint exists"), ShieldBlueprint);
	TestNotNull(TEXT("ShieldBash ability Blueprint exists"), AbilityBlueprint);
	TestNotNull(TEXT("ShieldBash cooldown Blueprint exists"), CooldownBlueprint);
	TestNotNull(TEXT("ShieldBash VFX exists"), LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ProjectR/Enemies/VFX/VFX_Enemy_ShieldBash.VFX_Enemy_ShieldBash")));
	TestNotNull(TEXT("ShieldBash SFX exists"), LoadObject<USoundWave>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Audio/SFX_Enemy_ShieldBash.SFX_Enemy_ShieldBash")));

	if (Registry)
	{
		TestTrue(TEXT("Registry retains the first three entries before downstream appends"), Registry->GetEntries().Num() >= 3);
		if (Registry->GetEntries().Num() >= 3)
		{
			TestEqual(TEXT("Melee entry remains first"), Registry->GetEntries()[0].PrototypeTag,
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")));
			TestEqual(TEXT("Ranged entry remains second"), Registry->GetEntries()[1].PrototypeTag,
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion")));
			TestEqual(TEXT("Shield entry is appended third"), Registry->GetEntries()[2].PrototypeTag,
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.ShieldMinion")));
		}
	}
	if (Shield)
	{
		TestEqual(TEXT("Shield mobility is Heavy"), Shield->Mobility, EPRAbilityTargetMobility::Heavy);
		TestEqual(TEXT("Shield health is fixed"), Shield->Attributes.Health, 140.0f);
		TestEqual(TEXT("Shield max health is fixed"), Shield->Attributes.MaxHealth, 140.0f);
		TestEqual(TEXT("Shield amount is fixed"), Shield->Attributes.Shield, 80.0f);
		TestEqual(TEXT("Shield max amount is fixed"), Shield->Attributes.MaxShield, 80.0f);
		TestEqual(TEXT("Shield energy is fixed"), Shield->Attributes.Energy, 0.0f);
		TestEqual(TEXT("Shield max energy is fixed"), Shield->Attributes.MaxEnergy, 1.0f);
		TestEqual(TEXT("Shield attack power is fixed"), Shield->Attributes.AttackPower, 12.0f);
		TestEqual(TEXT("Shield move speed is fixed"), Shield->Attributes.MoveSpeed, 300.0f);
		TestEqual(TEXT("Shield acquire range is fixed"), Shield->Perception.AcquireRange, 900.0f);
		TestEqual(TEXT("Shield lose range is fixed"), Shield->Perception.LoseRange, 1200.0f);
		TestEqual(TEXT("Shield preferred minimum is fixed"), Shield->Perception.PreferredMinRange, 0.0f);
		TestEqual(TEXT("Shield preferred maximum is fixed"), Shield->Perception.PreferredMaxRange, 160.0f);
	}
	if (Bash)
	{
		TestEqual(TEXT("ShieldBash is a melee sweep"), Bash->Kind, EPREnemyAttackKind::MeleeSweep);
		TestEqual(TEXT("ShieldBash minimum range is fixed"), Bash->MinRange, 0.0f);
		TestEqual(TEXT("ShieldBash maximum range is fixed"), Bash->MaxRange, 160.0f);
		TestEqual(TEXT("ShieldBash base damage is fixed"), Bash->BaseDamage, 10.0f);
		TestEqual(TEXT("ShieldBash AP scale is fixed"), Bash->AttackPowerScale, 0.5f);
		TestEqual(TEXT("ShieldBash windup is fixed"), Bash->Windup, 0.40f);
		TestEqual(TEXT("ShieldBash active window is fixed"), Bash->ActiveWindow, 0.15f);
		TestEqual(TEXT("ShieldBash recovery is fixed"), Bash->Recovery, 0.55f);
		TestEqual(TEXT("ShieldBash cooldown is fixed"), Bash->Cooldown, 1.60f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyCheckpointDAssetManifestTest,
	"ProjectR.Enemy.Assets.CheckpointDManifest",
	PREnemyArchetypeAutomation::TestFlags)

bool FPREnemyCheckpointDAssetManifestTest::RunTest(const FString& Parameters)
{
	const UPREnemyPrototypeRegistryDataAsset* Registry = LoadObject<UPREnemyPrototypeRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	const UPREnemyPrototypeDataAsset* Elite = LoadObject<UPREnemyPrototypeDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_EliteAuditGuard.DA_Enemy_EliteAuditGuard"));
	const UPREnemyAttackDataAsset* Strike = LoadObject<UPREnemyAttackDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_EliteStrike.DA_EnemyAttack_EliteStrike"));
	const UPRAbilitySetDataAsset* AbilitySet = LoadObject<UPRAbilitySetDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Abilities/DA_EnemyAbilitySet_EliteAuditGuard.DA_EnemyAbilitySet_EliteAuditGuard"));
	const UBlueprint* FrozenStunnedBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Stunned.GE_State_Stunned"));
	const UBlueprint* EliteAbilityBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/GA_Enemy_EliteStrike.GA_Enemy_EliteStrike"));
	const UBlueprint* EliteCooldownBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_EliteStrike_Cooldown.GE_EnemyAttack_EliteStrike_Cooldown"));

	TestNotNull(TEXT("Elite prototype exists"), Elite);
	TestNotNull(TEXT("EliteStrike attack data exists"), Strike);
	TestNotNull(TEXT("Elite ability set exists"), AbilitySet);
	TestNotNull(TEXT("Elite enemy Blueprint exists"), LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_EliteAuditGuard.BP_Enemy_EliteAuditGuard")));
	TestNotNull(TEXT("EliteStrike ability Blueprint exists"), LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/GA_Enemy_EliteStrike.GA_Enemy_EliteStrike")));
	TestNotNull(TEXT("EliteStrike cooldown Blueprint exists"), LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_EliteStrike_Cooldown.GE_EnemyAttack_EliteStrike_Cooldown")));
	TestNotNull(TEXT("EliteStrike VFX exists"), LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ProjectR/Enemies/VFX/VFX_Enemy_EliteStrike.VFX_Enemy_EliteStrike")));
	TestNotNull(TEXT("EliteStrike SFX exists"), LoadObject<USoundWave>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Audio/SFX_Enemy_EliteStrike.SFX_Enemy_EliteStrike")));

	const FClassProperty* ShieldBreakEffectProperty = FindFProperty<FClassProperty>(
		UPREnemyPrototypeDataAsset::StaticClass(), TEXT("ShieldBreakEffect"));
	TestNotNull(TEXT("Prototype exposes the data-driven ShieldBreak effect"), ShieldBreakEffectProperty);

	if (Registry)
	{
		TestEqual(TEXT("Registry has exactly four D entries"), Registry->GetEntries().Num(), 4);
		if (Registry->GetEntries().Num() == 4)
		{
			TestEqual(TEXT("Elite entry is appended fourth"), Registry->GetEntries()[3].PrototypeTag,
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard")));
		}
	}
	if (Elite)
	{
		TestEqual(TEXT("Elite mobility is Anchored"), Elite->Mobility, EPRAbilityTargetMobility::Anchored);
		TestEqual(TEXT("Elite Health is fixed"), Elite->Attributes.Health, 300.0f);
		TestEqual(TEXT("Elite MaxHealth is fixed"), Elite->Attributes.MaxHealth, 300.0f);
		TestEqual(TEXT("Elite Shield is fixed"), Elite->Attributes.Shield, 150.0f);
		TestEqual(TEXT("Elite MaxShield is fixed"), Elite->Attributes.MaxShield, 150.0f);
		TestEqual(TEXT("Elite AttackPower is fixed"), Elite->Attributes.AttackPower, 15.0f);
		TestEqual(TEXT("Elite MoveSpeed is fixed"), Elite->Attributes.MoveSpeed, 350.0f);
		TestEqual(TEXT("Elite acquire range is fixed"), Elite->Perception.AcquireRange, 1200.0f);
		TestEqual(TEXT("Elite lose range is fixed"), Elite->Perception.LoseRange, 1500.0f);
		TestEqual(TEXT("Elite preferred maximum is fixed"), Elite->Perception.PreferredMaxRange, 190.0f);
		TestNotNull(TEXT("Elite ShieldBreak effect is configured"), Elite->ShieldBreakEffect.Get());
		TestTrue(TEXT("Elite ShieldBreak effect reuses frozen Stunned"),
			Elite->ShieldBreakEffect.Get() == (FrozenStunnedBlueprint ? FrozenStunnedBlueprint->GeneratedClass : nullptr));
	}
	if (Strike)
	{
		TestEqual(TEXT("EliteStrike is a melee sweep"), Strike->Kind, EPREnemyAttackKind::MeleeSweep);
		TestEqual(TEXT("EliteStrike maximum range is fixed"), Strike->MaxRange, 190.0f);
		TestEqual(TEXT("EliteStrike base damage is fixed"), Strike->BaseDamage, 12.0f);
		TestEqual(TEXT("EliteStrike AP scale is fixed"), Strike->AttackPowerScale, 0.8f);
		TestEqual(TEXT("EliteStrike windup is fixed"), Strike->Windup, 0.50f);
		TestEqual(TEXT("EliteStrike active window is fixed"), Strike->ActiveWindow, 0.18f);
		TestEqual(TEXT("EliteStrike recovery is fixed"), Strike->Recovery, 0.65f);
		TestEqual(TEXT("EliteStrike cooldown is fixed"), Strike->Cooldown, 1.50f);
	}
	if (AbilitySet && EliteAbilityBlueprint)
	{
		TestEqual(TEXT("Elite ability set has exactly one entry"), AbilitySet->GetAbilityEntries().Num(), 1);
		if (AbilitySet->GetAbilityEntries().Num() == 1)
		{
			TestTrue(TEXT("Elite ability set grants EliteStrike Blueprint"),
				AbilitySet->GetAbilityEntries()[0].AbilityClass.Get() == EliteAbilityBlueprint->GeneratedClass);
			TestTrue(TEXT("Elite ability set uses EliteStrike data"),
				AbilitySet->GetAbilityEntries()[0].AbilityData.Get() == Strike);
		}
	}
	if (EliteAbilityBlueprint && EliteAbilityBlueprint->GeneratedClass)
	{
		const UPRGameplayAbility* AbilityCDO = Cast<UPRGameplayAbility>(EliteAbilityBlueprint->GeneratedClass->GetDefaultObject());
		TestNotNull(TEXT("EliteStrike GA CDO is a ProjectR ability"), AbilityCDO);
		if (AbilityCDO)
		{
			TestEqual(TEXT("EliteStrike GA is ServerTriggered"), AbilityCDO->GetActivationPolicy(),
				EPRAbilityActivationPolicy::ServerTriggered);
			TestEqual(TEXT("EliteStrike GA tag is fixed"), AbilityCDO->GetProjectRAbilityTag(),
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.EliteStrike")));
		}
	}
	if (EliteCooldownBlueprint && EliteCooldownBlueprint->GeneratedClass)
	{
		const UGameplayEffect* CooldownCDO = EliteCooldownBlueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>();
		TestNotNull(TEXT("EliteStrike cooldown GE CDO exists"), CooldownCDO);
		if (CooldownCDO)
		{
			float Duration = 0.0f;
			TestEqual(TEXT("EliteStrike cooldown uses duration policy"), CooldownCDO->DurationPolicy,
				EGameplayEffectDurationType::HasDuration);
			TestTrue(TEXT("EliteStrike cooldown duration is fixed"),
				CooldownCDO->DurationMagnitude.GetStaticMagnitudeIfPossible(1.0f, Duration)
				&& FMath::IsNearlyEqual(Duration, 1.50f));
			TestTrue(TEXT("EliteStrike cooldown grants its exact cooldown tag"),
				CooldownCDO->GetGrantedTags().HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Enemy.EliteStrike"))));
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
