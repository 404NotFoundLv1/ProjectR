// Copyright Epic Games, Inc. All Rights Reserved.

#include "PREnemyAuthoringToolset.h"

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAttributeSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyAttackGameplayAbility.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyProjectile.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Enemies/PREnemyStateTreeNodes.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/SoundFactory.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#include "ObjectTools.h"
#include "Sound/SoundWave.h"
#include "StateTree.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorModule.h"
#include "StateTreeEditorSchema.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeState.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace PREnemyAuthoringToolset
{
struct FScopedToolResultRoot
{
	explicit FScopedToolResultRoot(UToolCallAsyncResultString* InResult)
		: Result(InResult)
	{
		check(Result);
		Result->AddToRoot();
	}

	~FScopedToolResultRoot()
	{
		Result->RemoveFromRoot();
	}

	UToolCallAsyncResultString* Result;
};

constexpr const TCHAR* Packages[] = {
	TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry"),
	TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes"),
	TEXT("/Game/ProjectR/Enemies/AI/ST_Enemy_Base"),
	TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_Base"),
	TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_MeleeMinion"),
	TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_MeleeMinion"),
	TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_MeleeStrike"),
	TEXT("/Game/ProjectR/Enemies/Attacks/GA_Enemy_MeleeStrike"),
	TEXT("/Game/ProjectR/Enemies/Abilities/DA_EnemyAbilitySet_Melee"),
	TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_MeleeStrike_Cooldown"),
	TEXT("/Game/ProjectR/Enemies/VFX/VFX_Enemy_MeleeStrike"),
	TEXT("/Game/ProjectR/Enemies/Audio/SFX_Enemy_MeleeStrike")};

constexpr const TCHAR* CheckpointBPackages[] = {
	TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_RangedMinion"),
	TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_RangedMinion"),
	TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_RangedShot"),
	TEXT("/Game/ProjectR/Enemies/Attacks/GA_Enemy_RangedShot"),
	TEXT("/Game/ProjectR/Enemies/Abilities/DA_EnemyAbilitySet_Ranged"),
	TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_RangedShot_Cooldown"),
	TEXT("/Game/ProjectR/Enemies/VFX/VFX_Enemy_RangedShot"),
	TEXT("/Game/ProjectR/Enemies/Audio/SFX_Enemy_RangedShot"),
	TEXT("/Game/ProjectR/Enemies/Blueprints/BP_EnemyProjectile_Ranged")};

constexpr const TCHAR* CheckpointCPackages[] = {
	TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_ShieldMinion"),
	TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_ShieldMinion"),
	TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_ShieldBash"),
	TEXT("/Game/ProjectR/Enemies/Attacks/GA_Enemy_ShieldBash"),
	TEXT("/Game/ProjectR/Enemies/Abilities/DA_EnemyAbilitySet_Shield"),
	TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_ShieldBash_Cooldown"),
	TEXT("/Game/ProjectR/Enemies/VFX/VFX_Enemy_ShieldBash"),
	TEXT("/Game/ProjectR/Enemies/Audio/SFX_Enemy_ShieldBash")};

template <typename T>
T* CreateAsset(const TCHAR* Path)
{
	const FString LongName(Path);
	UPackage* Package = CreatePackage(*LongName);
	T* Asset = NewObject<T>(Package, *FPackageName::GetLongPackageAssetName(LongName), RF_Public | RF_Standalone);
	FAssetRegistryModule::AssetCreated(Asset);
	Package->MarkPackageDirty();
	return Asset;
}

UBlueprint* CreateBlueprint(const TCHAR* Path, UClass* Parent)
{
	const FString LongName(Path);
	UPackage* Package = CreatePackage(*LongName);
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(Parent, Package,
		FName(FPackageName::GetLongPackageAssetName(LongName)), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	if (Blueprint) FAssetRegistryModule::AssetCreated(Blueprint);
	return Blueprint;
}

UBlueprint* CreateGameplayEffectBlueprint(const TCHAR* Path)
{
	return CreateBlueprint(Path, UGameplayEffect::StaticClass());
}

UNiagaraSystem* CreateNiagaraSystem(const TCHAR* Path)
{
	const FString LongName(Path);
	UNiagaraSystemFactoryNew* Factory = NewObject<UNiagaraSystemFactoryNew>();
	return Cast<UNiagaraSystem>(FAssetToolsModule::GetModule().Get().CreateAsset(
		FPackageName::GetLongPackageAssetName(LongName), FPackageName::GetLongPackagePath(LongName), UNiagaraSystem::StaticClass(), Factory));
}

USoundWave* CreatePlaceholderSound(const TCHAR* Path)
{
	const uint8 WaveBytes[] = {
		'R','I','F','F', 0x24,0x00,0x00,0x00, 'W','A','V','E', 'f','m','t',' ',
		0x10,0x00,0x00,0x00, 0x01,0x00, 0x01,0x00, 0x22,0x56,0x00,0x00,
		0x44,0xac,0x00,0x00, 0x02,0x00,0x10,0x00, 'd','a','t','a', 0x00,0x00,0x00,0x00};
	const FString LongName(Path);
	UPackage* Package = CreatePackage(*LongName);
	USoundFactory* Factory = NewObject<USoundFactory>();
	const uint8* WaveBuffer = WaveBytes;
	USoundWave* Sound = Cast<USoundWave>(Factory->FactoryCreateBinary(USoundWave::StaticClass(), Package,
		*FPackageName::GetLongPackageAssetName(LongName), RF_Public | RF_Standalone, nullptr, TEXT("wav"),
		WaveBuffer, WaveBytes + UE_ARRAY_COUNT(WaveBytes), GWarn));
	if (Sound)
	{
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Sound);
	}
	return Sound;
}

void AddSetByCallerModifier(UGameplayEffect* Effect, const FGameplayAttribute Attribute, const FGameplayTag Tag)
{
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Override;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = Tag;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}

bool Preflight(FString& Error)
{
	for (const TCHAR* Path : Packages)
	{
		if (FindObject<UObject>(nullptr, Path))
		{
			Error = FString::Printf(TEXT("Checkpoint A manifest collision: %s"), Path);
			return false;
		}
	}
	return true;
}

bool ResetFixedManifest(FString& Error, int32& OutDeleted)
{
	OutDeleted = 0;
	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
	TArray<FAssetData> AssetsToDelete;
	for (const TCHAR* Path : Packages)
	{
		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssetsByPackageName(FName(Path), FoundAssets, true);
		if (FoundAssets.Num() > 1)
		{
			Error = FString::Printf(TEXT("Fixed manifest reset found multiple assets in %s"), Path);
			return false;
		}
		AssetsToDelete.Append(FoundAssets);
	}

	if (!AssetsToDelete.IsEmpty())
	{
		OutDeleted = ObjectTools::DeleteAssets(AssetsToDelete, false);
		if (OutDeleted != AssetsToDelete.Num())
		{
			Error = FString::Printf(TEXT("Fixed manifest reset deleted %d of %d exact assets."), OutDeleted, AssetsToDelete.Num());
			return false;
		}
	}

	for (const TCHAR* Path : Packages)
	{
		FString Filename;
		TArray<FAssetData> RemainingAssets;
		AssetRegistry.GetAssetsByPackageName(FName(Path), RemainingAssets, true);
		if (FPackageName::DoesPackageExist(Path, &Filename) || !RemainingAssets.IsEmpty())
		{
			Error = FString::Printf(TEXT("Fixed manifest reset did not remove %s"), Path);
			return false;
		}
	}
	return true;
}

bool SavePackageObject(UObject* Asset, FString& Error)
{
	if (!Asset || !Asset->GetOutermost()) { Error = TEXT("Null asset during fixed manifest save."); return false; }
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.Error = GError;
	const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
	if (!UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args))
	{
		Error = FString::Printf(TEXT("Could not save fixed manifest package %s"), *Asset->GetPathName());
		return false;
	}
	return true;
}

bool ConfigureEffectBlueprints(
	UBlueprint* DefaultAttributesBlueprint,
	UBlueprint* CooldownBlueprint,
	UBlueprint* MeleeBlueprint,
	UBlueprint* StrikeAbilityBlueprint,
	FString& Error)
{
	if (!DefaultAttributesBlueprint || !CooldownBlueprint || !MeleeBlueprint || !StrikeAbilityBlueprint
		|| !DefaultAttributesBlueprint->GeneratedClass || !CooldownBlueprint->GeneratedClass
		|| !MeleeBlueprint->GeneratedClass || !StrikeAbilityBlueprint->GeneratedClass)
	{
		Error = TEXT("Fixed effect Blueprint generated classes are unavailable.");
		return false;
	}

	UGameplayEffect* Defaults = DefaultAttributesBlueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	UGameplayEffect* Cooldown = CooldownBlueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	if (!Defaults || !Cooldown)
	{
		Error = TEXT("Fixed effect Blueprint CDOs are unavailable.");
		return false;
	}

	Defaults->DurationPolicy = EGameplayEffectDurationType::Instant;
	Defaults->Modifiers.Reset();
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetHealthAttribute(), UPRTagLibrary::GetEnemyDataHealthTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetMaxHealthAttribute(), UPRTagLibrary::GetEnemyDataMaxHealthTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetShieldAttribute(), UPRTagLibrary::GetEnemyDataShieldTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetMaxShieldAttribute(), UPRTagLibrary::GetEnemyDataMaxShieldTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetEnergyAttribute(), UPRTagLibrary::GetEnemyDataEnergyTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetMaxEnergyAttribute(), UPRTagLibrary::GetEnemyDataMaxEnergyTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetAttackPowerAttribute(), UPRTagLibrary::GetEnemyDataAttackPowerTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetMoveSpeedAttribute(), UPRTagLibrary::GetEnemyDataMoveSpeedTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetCritChanceAttribute(), UPRTagLibrary::GetEnemyDataCritChanceTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetPermissionAttribute(), UPRTagLibrary::GetEnemyDataPermissionTag());
	AddSetByCallerModifier(Defaults, UPRAttributeSet::GetResonanceAttribute(), UPRTagLibrary::GetEnemyDataResonanceTag());
	DefaultAttributesBlueprint->MarkPackageDirty();

	Cooldown->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	Cooldown->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.10f));
	FInheritedTagContainer CooldownTags;
	CooldownTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Enemy.MeleeStrike")));
	Cooldown->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(CooldownTags);
	CooldownBlueprint->MarkPackageDirty();

	if (FClassProperty* DefaultEffectProperty = FindFProperty<FClassProperty>(MeleeBlueprint->GeneratedClass, TEXT("DefaultAttributesEffect")))
	{
		DefaultEffectProperty->SetPropertyValue_InContainer(MeleeBlueprint->GeneratedClass->GetDefaultObject(), DefaultAttributesBlueprint->GeneratedClass);
	}
	else
	{
		Error = TEXT("BP_Enemy_MeleeMinion has no DefaultAttributesEffect CDO property.");
		return false;
	}
	MeleeBlueprint->MarkPackageDirty();

	if (FClassProperty* CooldownProperty = FindFProperty<FClassProperty>(StrikeAbilityBlueprint->GeneratedClass, TEXT("CooldownGameplayEffectClass")))
	{
		CooldownProperty->SetPropertyValue_InContainer(StrikeAbilityBlueprint->GeneratedClass->GetDefaultObject(), CooldownBlueprint->GeneratedClass);
	}
	else
	{
		Error = TEXT("GA_Enemy_MeleeStrike has no CooldownGameplayEffectClass CDO property.");
		return false;
	}
	StrikeAbilityBlueprint->MarkPackageDirty();

	FKismetEditorUtilities::CompileBlueprint(MeleeBlueprint);
	FKismetEditorUtilities::CompileBlueprint(StrikeAbilityBlueprint);
	return true;
}

bool PreflightCheckpointB(FString& Error)
{
	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
	for (const TCHAR* Path : CheckpointBPackages)
	{
		FString Filename;
		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssetsByPackageName(FName(Path), FoundAssets, true);
		if (FindObject<UObject>(nullptr, Path) || FPackageName::DoesPackageExist(Path, &Filename) || !FoundAssets.IsEmpty())
		{
			Error = FString::Printf(TEXT("Checkpoint B manifest collision: %s"), Path);
			return false;
		}
	}
	return true;
}

bool ConfigureCheckpointBAssets(
	UPREnemyPrototypeRegistryDataAsset* Registry,
	UBlueprint* RangedBlueprint,
	UPREnemyPrototypeDataAsset* Ranged,
	UPREnemyAttackDataAsset* RangedShot,
	UBlueprint* RangedAbilityBlueprint,
	UPRAbilitySetDataAsset* RangedAbilitySet,
	UBlueprint* RangedCooldownBlueprint,
	UNiagaraSystem* RangedVFX,
	USoundWave* RangedSFX,
	UBlueprint* ProjectileBlueprint,
	FString& Error)
{
	UBlueprint* BaseBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_Base.BP_Enemy_Base"));
	UBlueprint* DefaultAttributesBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"));
	UStateTree* StateTree = LoadObject<UStateTree>(nullptr,
		TEXT("/Game/ProjectR/Enemies/AI/ST_Enemy_Base.ST_Enemy_Base"));
	UClass* DamageEffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/ProjectR/Effects/GE_Damage.GE_Damage_C"));
	const FGameplayTag MeleeTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion"));
	const FGameplayTag RangedTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion"));
	const FGameplayTag RangedAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.RangedShot"));
	const FGameplayTag RangedCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Enemy.RangedShot"));
	if (!Registry || !RangedBlueprint || !Ranged || !RangedShot || !RangedAbilityBlueprint || !RangedAbilitySet
		|| !RangedCooldownBlueprint || !RangedVFX || !RangedSFX || !ProjectileBlueprint || !BaseBlueprint
		|| !DefaultAttributesBlueprint || !DefaultAttributesBlueprint->GeneratedClass || !StateTree || !DamageEffectClass
		|| !RangedBlueprint->GeneratedClass || !RangedAbilityBlueprint->GeneratedClass || !ProjectileBlueprint->GeneratedClass)
	{
		Error = TEXT("Checkpoint B fixed dependencies are unavailable.");
		return false;
	}
	if (Registry->Entries.Num() != 1 || Registry->Entries[0].PrototypeTag != MeleeTag
		|| Registry->Entries[0].Prototype.IsNull() || Registry->Entries[0].EnemyClass.IsNull())
	{
		Error = TEXT("Checkpoint B requires the exact one-entry Melee registry preimage.");
		return false;
	}
	if (!RangedBlueprint->GeneratedClass->IsChildOf(BaseBlueprint->GeneratedClass)
		|| !ProjectileBlueprint->GeneratedClass->IsChildOf(APREnemyProjectile::StaticClass()))
	{
		Error = TEXT("Checkpoint B Blueprint parent classes are invalid.");
		return false;
	}

	Ranged->EnemyId = TEXT("RangedMinion");
	Ranged->PrototypeTag = RangedTag;
	Ranged->Mobility = EPRAbilityTargetMobility::Light;
	Ranged->Attributes = {80.0f, 80.0f, 0.0f, 0.0f, 0.0f, 1.0f, 10.0f, 380.0f, 0.0f, 0.0f, 0.0f};
	Ranged->Perception.SenseInterval = 0.10f;
	Ranged->Perception.AcquireRange = 1100.0f;
	Ranged->Perception.LoseRange = 1400.0f;
	Ranged->Perception.PreferredMinRange = 350.0f;
	Ranged->Perception.PreferredMaxRange = 700.0f;
	Ranged->Perception.EdgeProbeForward = 60.0f;
	Ranged->Perception.EdgeProbeDepth = 120.0f;
	Ranged->MarkPackageDirty();

	RangedShot->AttackId = TEXT("RangedShot");
	RangedShot->AttackTag = RangedAttackTag;
	RangedShot->Kind = EPREnemyAttackKind::Projectile;
	RangedShot->MinRange = 350.0f;
	RangedShot->MaxRange = 700.0f;
	RangedShot->BaseDamage = 8.0f;
	RangedShot->AttackPowerScale = 0.4f;
	RangedShot->Windup = 0.45f;
	RangedShot->ActiveWindow = 0.05f;
	RangedShot->Recovery = 0.35f;
	RangedShot->Cooldown = 1.40f;
	RangedShot->SweepRadius = 0.0f;
	RangedShot->SweepHalfAngleDegrees = 0.0f;
	RangedShot->ProjectileClass = ProjectileBlueprint->GeneratedClass;
	RangedShot->ProjectileSpeed = 1200.0f;
	RangedShot->ProjectileLifetime = 1.5f;
	RangedShot->DamageTags.Reset();
	RangedShot->VFX = RangedVFX;
	RangedShot->SFX = RangedSFX;
	RangedShot->MarkPackageDirty();

	Ranged->AttackDefinitions.Reset();
	Ranged->AttackDefinitions.Add(RangedShot);
	Ranged->InitialAbilitySet = RangedAbilitySet;
	Ranged->MarkPackageDirty();

	RangedAbilitySet->AbilityEntries.Reset();
	FPRAbilitySetEntry& AbilityEntry = RangedAbilitySet->AbilityEntries.AddDefaulted_GetRef();
	AbilityEntry.AbilityClass = RangedAbilityBlueprint->GeneratedClass;
	AbilityEntry.AbilityData = RangedShot;
	AbilityEntry.AbilityLevel = 1;
	AbilityEntry.bGrantOnInitialization = true;
	RangedAbilitySet->MarkPackageDirty();

	UGameplayEffect* Cooldown = RangedCooldownBlueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	if (!Cooldown)
	{
		Error = TEXT("Checkpoint B cooldown GameplayEffect CDO is unavailable.");
		return false;
	}
	Cooldown->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	Cooldown->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.40f));
	Cooldown->Modifiers.Reset();
	FInheritedTagContainer CooldownTags;
	CooldownTags.AddTag(RangedCooldownTag);
	Cooldown->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(CooldownTags);
	RangedCooldownBlueprint->MarkPackageDirty();

	if (FObjectProperty* StateTreeProperty = FindFProperty<FObjectProperty>(RangedBlueprint->GeneratedClass, TEXT("EnemyStateTree")))
	{
		StateTreeProperty->SetObjectPropertyValue_InContainer(RangedBlueprint->GeneratedClass->GetDefaultObject(), StateTree);
	}
	else { Error = TEXT("BP_Enemy_RangedMinion has no EnemyStateTree CDO property."); return false; }
	if (FClassProperty* DefaultEffectProperty = FindFProperty<FClassProperty>(RangedBlueprint->GeneratedClass, TEXT("DefaultAttributesEffect")))
	{
		DefaultEffectProperty->SetPropertyValue_InContainer(RangedBlueprint->GeneratedClass->GetDefaultObject(), DefaultAttributesBlueprint->GeneratedClass);
	}
	else { Error = TEXT("BP_Enemy_RangedMinion has no DefaultAttributesEffect CDO property."); return false; }
	if (FClassProperty* DamageEffectProperty = FindFProperty<FClassProperty>(RangedBlueprint->GeneratedClass, TEXT("DamageEffect")))
	{
		DamageEffectProperty->SetPropertyValue_InContainer(RangedBlueprint->GeneratedClass->GetDefaultObject(), DamageEffectClass);
	}
	else { Error = TEXT("BP_Enemy_RangedMinion has no DamageEffect CDO property."); return false; }
	RangedBlueprint->MarkPackageDirty();

	if (FStructProperty* TagProperty = FindFProperty<FStructProperty>(RangedAbilityBlueprint->GeneratedClass, TEXT("AbilityTag")))
	{
		*TagProperty->ContainerPtrToValuePtr<FGameplayTag>(RangedAbilityBlueprint->GeneratedClass->GetDefaultObject()) = RangedAttackTag;
	}
	else { Error = TEXT("GA_Enemy_RangedShot has no AbilityTag CDO property."); return false; }
	if (FClassProperty* CooldownProperty = FindFProperty<FClassProperty>(RangedAbilityBlueprint->GeneratedClass, TEXT("CooldownGameplayEffectClass")))
	{
		CooldownProperty->SetPropertyValue_InContainer(RangedAbilityBlueprint->GeneratedClass->GetDefaultObject(), RangedCooldownBlueprint->GeneratedClass);
	}
	else { Error = TEXT("GA_Enemy_RangedShot has no CooldownGameplayEffectClass CDO property."); return false; }
	RangedAbilityBlueprint->MarkPackageDirty();

	FPREnemyPrototypeRegistryEntry& RegistryEntry = Registry->Entries.AddDefaulted_GetRef();
	RegistryEntry.PrototypeTag = RangedTag;
	RegistryEntry.Prototype = Ranged;
	RegistryEntry.EnemyClass = RangedBlueprint->GeneratedClass;
	Registry->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(RangedBlueprint);
	FKismetEditorUtilities::CompileBlueprint(RangedAbilityBlueprint);
	FKismetEditorUtilities::CompileBlueprint(RangedCooldownBlueprint);
	FKismetEditorUtilities::CompileBlueprint(ProjectileBlueprint);
	return true;
}

bool PreflightCheckpointC(FString& Error)
{
	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
	for (const TCHAR* Path : CheckpointCPackages)
	{
		FString Filename;
		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssetsByPackageName(FName(Path), FoundAssets, true);
		if (FindObject<UObject>(nullptr, Path) || FPackageName::DoesPackageExist(Path, &Filename) || !FoundAssets.IsEmpty())
		{
			Error = FString::Printf(TEXT("Checkpoint C manifest collision: %s"), Path);
			return false;
		}
	}
	return true;
}

bool ConfigureCheckpointCAssets(
	UPREnemyPrototypeRegistryDataAsset* Registry,
	UBlueprint* ShieldBlueprint,
	UPREnemyPrototypeDataAsset* Shield,
	UPREnemyAttackDataAsset* ShieldBash,
	UBlueprint* ShieldAbilityBlueprint,
	UPRAbilitySetDataAsset* ShieldAbilitySet,
	UBlueprint* ShieldCooldownBlueprint,
	UNiagaraSystem* ShieldVFX,
	USoundWave* ShieldSFX,
	FString& Error)
{
	UBlueprint* BaseBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_Base.BP_Enemy_Base"));
	UBlueprint* DefaultAttributesBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"));
	UStateTree* StateTree = LoadObject<UStateTree>(nullptr,
		TEXT("/Game/ProjectR/Enemies/AI/ST_Enemy_Base.ST_Enemy_Base"));
	UClass* DamageEffectClass = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/ProjectR/Effects/GE_Damage.GE_Damage_C"));
	const FGameplayTag MeleeTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion"));
	const FGameplayTag RangedTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion"));
	const FGameplayTag ShieldTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.ShieldMinion"));
	const FGameplayTag ShieldAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.ShieldBash"));
	const FGameplayTag ShieldCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Enemy.ShieldBash"));
	if (!Registry || !ShieldBlueprint || !Shield || !ShieldBash || !ShieldAbilityBlueprint || !ShieldAbilitySet
		|| !ShieldCooldownBlueprint || !ShieldVFX || !ShieldSFX || !BaseBlueprint || !BaseBlueprint->GeneratedClass
		|| !DefaultAttributesBlueprint || !DefaultAttributesBlueprint->GeneratedClass || !StateTree || !DamageEffectClass
		|| !ShieldBlueprint->GeneratedClass || !ShieldAbilityBlueprint->GeneratedClass)
	{
		Error = TEXT("Checkpoint C fixed dependencies are unavailable.");
		return false;
	}
	if (Registry->Entries.Num() != 2 || Registry->Entries[0].PrototypeTag != MeleeTag
		|| Registry->Entries[1].PrototypeTag != RangedTag || Registry->Entries[0].Prototype.IsNull()
		|| Registry->Entries[0].EnemyClass.IsNull() || Registry->Entries[1].Prototype.IsNull()
		|| Registry->Entries[1].EnemyClass.IsNull())
	{
		Error = TEXT("Checkpoint C requires the exact Melee/Ranged registry preimage.");
		return false;
	}
	if (!ShieldBlueprint->GeneratedClass->IsChildOf(BaseBlueprint->GeneratedClass))
	{
		Error = TEXT("BP_Enemy_ShieldMinion does not inherit the fixed Enemy Base Blueprint.");
		return false;
	}

	Shield->EnemyId = TEXT("ShieldMinion");
	Shield->PrototypeTag = ShieldTag;
	Shield->Mobility = EPRAbilityTargetMobility::Heavy;
	Shield->Attributes = {140.0f, 140.0f, 80.0f, 80.0f, 0.0f, 1.0f, 12.0f, 300.0f, 0.0f, 0.0f, 0.0f};
	Shield->Perception.SenseInterval = 0.10f;
	Shield->Perception.AcquireRange = 900.0f;
	Shield->Perception.LoseRange = 1200.0f;
	Shield->Perception.PreferredMinRange = 0.0f;
	Shield->Perception.PreferredMaxRange = 160.0f;
	Shield->Perception.EdgeProbeForward = 60.0f;
	Shield->Perception.EdgeProbeDepth = 120.0f;
	Shield->MarkPackageDirty();

	ShieldBash->AttackId = TEXT("ShieldBash");
	ShieldBash->AttackTag = ShieldAttackTag;
	ShieldBash->Kind = EPREnemyAttackKind::MeleeSweep;
	ShieldBash->MinRange = 0.0f;
	ShieldBash->MaxRange = 160.0f;
	ShieldBash->BaseDamage = 10.0f;
	ShieldBash->AttackPowerScale = 0.5f;
	ShieldBash->Windup = 0.40f;
	ShieldBash->ActiveWindow = 0.15f;
	ShieldBash->Recovery = 0.55f;
	ShieldBash->Cooldown = 1.60f;
	ShieldBash->SweepRadius = 160.0f;
	ShieldBash->SweepHalfAngleDegrees = 60.0f;
	ShieldBash->ProjectileClass.Reset();
	ShieldBash->ProjectileSpeed = 0.0f;
	ShieldBash->ProjectileLifetime = 0.0f;
	ShieldBash->DamageTags.Reset();
	ShieldBash->VFX = ShieldVFX;
	ShieldBash->SFX = ShieldSFX;
	ShieldBash->MarkPackageDirty();

	Shield->AttackDefinitions.Reset();
	Shield->AttackDefinitions.Add(ShieldBash);
	Shield->InitialAbilitySet = ShieldAbilitySet;
	Shield->MarkPackageDirty();

	ShieldAbilitySet->AbilityEntries.Reset();
	FPRAbilitySetEntry& AbilityEntry = ShieldAbilitySet->AbilityEntries.AddDefaulted_GetRef();
	AbilityEntry.AbilityClass = ShieldAbilityBlueprint->GeneratedClass;
	AbilityEntry.AbilityData = ShieldBash;
	AbilityEntry.AbilityLevel = 1;
	AbilityEntry.bGrantOnInitialization = true;
	ShieldAbilitySet->MarkPackageDirty();

	UGameplayEffect* Cooldown = ShieldCooldownBlueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	if (!Cooldown)
	{
		Error = TEXT("Checkpoint C cooldown GameplayEffect CDO is unavailable.");
		return false;
	}
	Cooldown->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	Cooldown->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.60f));
	Cooldown->Modifiers.Reset();
	FInheritedTagContainer CooldownTags;
	CooldownTags.AddTag(ShieldCooldownTag);
	Cooldown->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(CooldownTags);
	ShieldCooldownBlueprint->MarkPackageDirty();

	if (FObjectProperty* StateTreeProperty = FindFProperty<FObjectProperty>(ShieldBlueprint->GeneratedClass, TEXT("EnemyStateTree")))
	{
		StateTreeProperty->SetObjectPropertyValue_InContainer(ShieldBlueprint->GeneratedClass->GetDefaultObject(), StateTree);
	}
	else { Error = TEXT("BP_Enemy_ShieldMinion has no EnemyStateTree CDO property."); return false; }
	if (FClassProperty* DefaultEffectProperty = FindFProperty<FClassProperty>(ShieldBlueprint->GeneratedClass, TEXT("DefaultAttributesEffect")))
	{
		DefaultEffectProperty->SetPropertyValue_InContainer(ShieldBlueprint->GeneratedClass->GetDefaultObject(), DefaultAttributesBlueprint->GeneratedClass);
	}
	else { Error = TEXT("BP_Enemy_ShieldMinion has no DefaultAttributesEffect CDO property."); return false; }
	if (FClassProperty* DamageEffectProperty = FindFProperty<FClassProperty>(ShieldBlueprint->GeneratedClass, TEXT("DamageEffect")))
	{
		DamageEffectProperty->SetPropertyValue_InContainer(ShieldBlueprint->GeneratedClass->GetDefaultObject(), DamageEffectClass);
	}
	else { Error = TEXT("BP_Enemy_ShieldMinion has no DamageEffect CDO property."); return false; }
	ShieldBlueprint->MarkPackageDirty();

	if (FStructProperty* TagProperty = FindFProperty<FStructProperty>(ShieldAbilityBlueprint->GeneratedClass, TEXT("AbilityTag")))
	{
		*TagProperty->ContainerPtrToValuePtr<FGameplayTag>(ShieldAbilityBlueprint->GeneratedClass->GetDefaultObject()) = ShieldAttackTag;
	}
	else { Error = TEXT("GA_Enemy_ShieldBash has no AbilityTag CDO property."); return false; }
	if (FClassProperty* CooldownProperty = FindFProperty<FClassProperty>(ShieldAbilityBlueprint->GeneratedClass, TEXT("CooldownGameplayEffectClass")))
	{
		CooldownProperty->SetPropertyValue_InContainer(ShieldAbilityBlueprint->GeneratedClass->GetDefaultObject(), ShieldCooldownBlueprint->GeneratedClass);
	}
	else { Error = TEXT("GA_Enemy_ShieldBash has no CooldownGameplayEffectClass CDO property."); return false; }
	ShieldAbilityBlueprint->MarkPackageDirty();

	FPREnemyPrototypeRegistryEntry& RegistryEntry = Registry->Entries.AddDefaulted_GetRef();
	RegistryEntry.PrototypeTag = ShieldTag;
	RegistryEntry.Prototype = Shield;
	RegistryEntry.EnemyClass = ShieldBlueprint->GeneratedClass;
	Registry->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(ShieldBlueprint);
	FKismetEditorUtilities::CompileBlueprint(ShieldAbilityBlueprint);
	FKismetEditorUtilities::CompileBlueprint(ShieldCooldownBlueprint);
	return true;
}
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::ResetCheckpointAEnemyManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	// ObjectTools performs an engine GC while unloading packages. Keep the MCP
	// response alive until its terminal value has been published.
	FString Error;
	int32 Deleted = 0;
	if (!PREnemyAuthoringToolset::ResetFixedManifest(Error, Deleted))
	{
		Result->SetError(Error);
		return Result;
	}
	Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"deleted\":%d,\"verifiedAbsent\":12,\"schema\":\"v0.2.1-A-fixed\"}"), Deleted));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::CreateCheckpointAEnemyAssets()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	FString Error;
	if (!PREnemyAuthoringToolset::Preflight(Error)) { Result->SetError(Error); return Result; }

	UPREnemyPrototypeRegistryDataAsset* Registry = PREnemyAuthoringToolset::CreateAsset<UPREnemyPrototypeRegistryDataAsset>(PREnemyAuthoringToolset::Packages[0]);
	UBlueprint* DefaultAttributesBlueprint = PREnemyAuthoringToolset::CreateGameplayEffectBlueprint(PREnemyAuthoringToolset::Packages[1]);
	UStateTree* StateTree = PREnemyAuthoringToolset::CreateAsset<UStateTree>(PREnemyAuthoringToolset::Packages[2]);
	UBlueprint* BaseBlueprint = PREnemyAuthoringToolset::CreateBlueprint(PREnemyAuthoringToolset::Packages[3], APREnemyCharacter::StaticClass());
	UBlueprint* MeleeBlueprint = PREnemyAuthoringToolset::CreateBlueprint(PREnemyAuthoringToolset::Packages[4], BaseBlueprint->GeneratedClass);
	UPREnemyPrototypeDataAsset* Melee = PREnemyAuthoringToolset::CreateAsset<UPREnemyPrototypeDataAsset>(PREnemyAuthoringToolset::Packages[5]);
	UPREnemyAttackDataAsset* Strike = PREnemyAuthoringToolset::CreateAsset<UPREnemyAttackDataAsset>(PREnemyAuthoringToolset::Packages[6]);
	UBlueprint* StrikeAbilityBlueprint = PREnemyAuthoringToolset::CreateBlueprint(PREnemyAuthoringToolset::Packages[7], UPREnemyAttackGameplayAbility::StaticClass());
	UPRAbilitySetDataAsset* AbilitySet = PREnemyAuthoringToolset::CreateAsset<UPRAbilitySetDataAsset>(PREnemyAuthoringToolset::Packages[8]);
	UBlueprint* CooldownBlueprint = PREnemyAuthoringToolset::CreateGameplayEffectBlueprint(PREnemyAuthoringToolset::Packages[9]);
	UNiagaraSystem* VFX = PREnemyAuthoringToolset::CreateNiagaraSystem(PREnemyAuthoringToolset::Packages[10]);
	USoundWave* SFX = PREnemyAuthoringToolset::CreatePlaceholderSound(PREnemyAuthoringToolset::Packages[11]);
	if (!Registry || !DefaultAttributesBlueprint || !StateTree || !BaseBlueprint || !MeleeBlueprint || !Melee || !Strike || !StrikeAbilityBlueprint || !AbilitySet || !CooldownBlueprint || !VFX || !SFX)
	{
		Result->SetError(TEXT("Checkpoint A could not create its complete fixed manifest."));
		return Result;
	}

	Melee->EnemyId = TEXT("MeleeMinion");
	Melee->PrototypeTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion"));
	Melee->Mobility = EPRAbilityTargetMobility::Light;
	Melee->Attributes = {100.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f, 450.0f, 0.0f, 0.0f, 0.0f};
	Melee->Perception.AcquireRange = 900.0f; Melee->Perception.LoseRange = 1200.0f; Melee->Perception.PreferredMaxRange = 140.0f;
	Strike->AttackId = TEXT("MeleeStrike"); Strike->AttackTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.MeleeStrike"));
	Strike->Kind = EPREnemyAttackKind::MeleeSweep; Strike->MaxRange = 140.0f; Strike->BaseDamage = 10.0f; Strike->AttackPowerScale = 0.5f;
	Strike->Windup = 0.30f; Strike->ActiveWindow = 0.12f; Strike->Recovery = 0.40f; Strike->Cooldown = 1.10f; Strike->SweepRadius = 140.0f; Strike->SweepHalfAngleDegrees = 60.0f;
	Strike->VFX = VFX; Strike->SFX = SFX;
	Melee->AttackDefinitions.Add(Strike);
	Melee->InitialAbilitySet = AbilitySet;
	FPRAbilitySetEntry& Entry = AbilitySet->AbilityEntries.AddDefaulted_GetRef(); Entry.AbilityClass = StrikeAbilityBlueprint->GeneratedClass; Entry.AbilityData = Strike; Entry.AbilityLevel = 1; Entry.bGrantOnInitialization = true;
	FPREnemyPrototypeRegistryEntry& RegistryEntry = Registry->Entries.AddDefaulted_GetRef(); RegistryEntry.PrototypeTag = Melee->PrototypeTag; RegistryEntry.Prototype = Melee; RegistryEntry.EnemyClass = MeleeBlueprint->GeneratedClass;
	if (UClass* MeleeClass = MeleeBlueprint->GeneratedClass)
	{
		if (FObjectProperty* StateTreeProperty = FindFProperty<FObjectProperty>(MeleeClass, TEXT("EnemyStateTree")))
			StateTreeProperty->SetObjectPropertyValue_InContainer(MeleeClass->GetDefaultObject(), StateTree);
		if (FClassProperty* DamageEffectProperty = FindFProperty<FClassProperty>(MeleeClass, TEXT("DamageEffect")))
			DamageEffectProperty->SetPropertyValue_InContainer(MeleeClass->GetDefaultObject(), LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/ProjectR/Effects/GE_Damage.GE_Damage_C")));
		MeleeBlueprint->MarkPackageDirty();
	}
	if (UClass* StrikeClass = StrikeAbilityBlueprint->GeneratedClass)
	{
		if (FStructProperty* TagProperty = FindFProperty<FStructProperty>(StrikeClass, TEXT("AbilityTag")))
			*TagProperty->ContainerPtrToValuePtr<FGameplayTag>(StrikeClass->GetDefaultObject()) = Strike->AttackTag;
		StrikeAbilityBlueprint->MarkPackageDirty();
	}
	if (!PREnemyAuthoringToolset::ConfigureEffectBlueprints(DefaultAttributesBlueprint, CooldownBlueprint, MeleeBlueprint, StrikeAbilityBlueprint, Error))
	{
		Result->SetError(Error);
		return Result;
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":12,\"saved\":false,\"schema\":\"v0.2.1-A-fixed\"}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::RecoverAndSaveCheckpointAEnemyAssets()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	const TCHAR* ExistingPaths[] = {PREnemyAuthoringToolset::Packages[0], PREnemyAuthoringToolset::Packages[1], PREnemyAuthoringToolset::Packages[2],
		PREnemyAuthoringToolset::Packages[4], PREnemyAuthoringToolset::Packages[5], PREnemyAuthoringToolset::Packages[6], PREnemyAuthoringToolset::Packages[7],
		PREnemyAuthoringToolset::Packages[8], PREnemyAuthoringToolset::Packages[9]};
	for (const TCHAR* Path : ExistingPaths)
	{
		if (!LoadObject<UObject>(nullptr, *FString::Printf(TEXT("%s.%s"), Path, *FPackageName::GetLongPackageAssetName(Path))))
		{
			Result->SetError(FString::Printf(TEXT("Recovery refuses an unexpected partial manifest: %s"), Path));
			return Result;
		}
	}
	UBlueprint* Base = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_Base.BP_Enemy_Base"));
	UNiagaraSystem* VFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/ProjectR/Enemies/VFX/VFX_Enemy_MeleeStrike.VFX_Enemy_MeleeStrike"));
	USoundWave* SFX = LoadObject<USoundWave>(nullptr, TEXT("/Game/ProjectR/Enemies/Audio/SFX_Enemy_MeleeStrike.SFX_Enemy_MeleeStrike"));
	TArray<UObject*> AssetsToSave;
	if (!Base) { Base = PREnemyAuthoringToolset::CreateBlueprint(PREnemyAuthoringToolset::Packages[3], APREnemyCharacter::StaticClass()); if (!Base) { Result->SetError(TEXT("Recovery could not create BP_Enemy_Base.")); return Result; } FKismetEditorUtilities::CompileBlueprint(Base); AssetsToSave.Add(Base); }
	if (!VFX) { VFX = PREnemyAuthoringToolset::CreateNiagaraSystem(PREnemyAuthoringToolset::Packages[10]); if (!VFX) { Result->SetError(TEXT("Recovery could not create VFX_Enemy_MeleeStrike.")); return Result; } AssetsToSave.Add(VFX); }
	if (!SFX) { SFX = PREnemyAuthoringToolset::CreatePlaceholderSound(PREnemyAuthoringToolset::Packages[11]); if (!SFX) { Result->SetError(TEXT("Recovery could not create SFX_Enemy_MeleeStrike.")); return Result; } AssetsToSave.Add(SFX); }
	FString Error;
	const int32 SavedCount = AssetsToSave.Num();
	for (UObject* Asset : AssetsToSave)
	{
		if (!PREnemyAuthoringToolset::SavePackageObject(Asset, Error)) { Result->SetError(Error); return Result; }
	}
	Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"recovered\":%d,\"saved\":%d,\"mode\":\"individual\"}"), SavedCount, SavedCount));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::RepairCheckpointAEffectBlueprints()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	if (LoadObject<UObject>(nullptr, TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"))
		|| LoadObject<UObject>(nullptr, TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_MeleeStrike_Cooldown.GE_EnemyAttack_MeleeStrike_Cooldown")))
	{
		Result->SetError(TEXT("Effect repair requires the two exact old GameplayEffect packages to be absent."));
		return Result;
	}
	UBlueprint* DefaultAttributesBlueprint = PREnemyAuthoringToolset::CreateGameplayEffectBlueprint(PREnemyAuthoringToolset::Packages[1]);
	UBlueprint* CooldownBlueprint = PREnemyAuthoringToolset::CreateGameplayEffectBlueprint(PREnemyAuthoringToolset::Packages[9]);
	UBlueprint* MeleeBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_MeleeMinion.BP_Enemy_MeleeMinion"));
	UBlueprint* StrikeAbilityBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ProjectR/Enemies/Attacks/GA_Enemy_MeleeStrike.GA_Enemy_MeleeStrike"));
	FString Error;
	if (!PREnemyAuthoringToolset::ConfigureEffectBlueprints(DefaultAttributesBlueprint, CooldownBlueprint, MeleeBlueprint, StrikeAbilityBlueprint, Error))
	{
		Result->SetError(Error);
		return Result;
	}
	for (UObject* Asset : {static_cast<UObject*>(DefaultAttributesBlueprint), static_cast<UObject*>(CooldownBlueprint), static_cast<UObject*>(MeleeBlueprint), static_cast<UObject*>(StrikeAbilityBlueprint)})
	{
		if (!PREnemyAuthoringToolset::SavePackageObject(Asset, Error))
		{
			Result->SetError(Error);
			return Result;
		}
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":2,\"rewired\":2,\"saved\":4}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::ClearCheckpointAStaleEffectReference()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	UBlueprint* MeleeBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_MeleeMinion.BP_Enemy_MeleeMinion"));
	if (!MeleeBlueprint || !MeleeBlueprint->GeneratedClass)
	{
		Result->SetError(TEXT("BP_Enemy_MeleeMinion generated class is unavailable for the fixed stale-reference repair."));
		return Result;
	}
	FClassProperty* DefaultEffectProperty = FindFProperty<FClassProperty>(MeleeBlueprint->GeneratedClass, TEXT("DefaultAttributesEffect"));
	if (!DefaultEffectProperty)
	{
		Result->SetError(TEXT("BP_Enemy_MeleeMinion has no DefaultAttributesEffect CDO property."));
		return Result;
	}
	DefaultEffectProperty->SetPropertyValue_InContainer(MeleeBlueprint->GeneratedClass->GetDefaultObject(), nullptr);
	MeleeBlueprint->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(MeleeBlueprint);
	FString Error;
	if (!PREnemyAuthoringToolset::SavePackageObject(MeleeBlueprint, Error))
	{
		Result->SetError(Error);
		return Result;
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"cleared\":\"BP_Enemy_MeleeMinion.DefaultAttributesEffect\",\"saved\":1}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::ConfigureCheckpointAStateTree()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	UStateTree* Tree = LoadObject<UStateTree>(nullptr, TEXT("/Game/ProjectR/Enemies/AI/ST_Enemy_Base.ST_Enemy_Base"));
	FObjectProperty* EditorDataProperty = FindFProperty<FObjectProperty>(UStateTree::StaticClass(), TEXT("EditorData"));
	FObjectProperty* SchemaProperty = FindFProperty<FObjectProperty>(UStateTree::StaticClass(), TEXT("Schema"));
	if (!Tree || !EditorDataProperty || !SchemaProperty) { Result->SetError(TEXT("StateTree properties are unavailable.")); return Result; }
	if (EditorDataProperty->GetObjectPropertyValue_InContainer(Tree)) { Result->SetError(TEXT("StateTree already has editor data; refusing overwrite.")); return Result; }
	FStateTreeEditorModule& EditorModule = FStateTreeEditorModule::GetModule();
	UStateTreeEditorData* Data = NewObject<UStateTreeEditorData>(Tree,
		EditorModule.GetEditorDataClass(UStateTreeAIComponentSchema::StaticClass()), FName(), RF_Transactional);
	EditorDataProperty->SetObjectPropertyValue_InContainer(Tree, Data);
	Data->Schema = NewObject<UStateTreeAIComponentSchema>(Data, UStateTreeAIComponentSchema::StaticClass(), FName(), RF_Transactional);
	Data->EditorSchema = NewObject<UStateTreeEditorSchema>(Data,
		EditorModule.GetEditorSchemaClass(UStateTreeAIComponentSchema::StaticClass()), FName(), RF_Transactional);
	UStateTreeState& Root = Data->AddRootState();
	Root.AddChildState(TEXT("Acquire")).AddTask<FPRSTTaskAcquireTarget>();
	Root.AddChildState(TEXT("Pursue")).AddTask<FPRSTTaskMoveToRangeBand>();
	Root.AddChildState(TEXT("Attack")).AddTask<FPRSTTaskActivateAttack>();
	Root.AddChildState(TEXT("Wait")).AddTask<FPRSTTaskWaitForReevaluate>();
	FStateTreeCompilerLog Log;
	if (!UStateTreeEditingSubsystem::CompileStateTree(Tree, Log)) { Result->SetError(TEXT("StateTree compile failed for the fixed graph.")); return Result; }
	FString Error;
	if (!PREnemyAuthoringToolset::SavePackageObject(Tree, Error)) { Result->SetError(Error); return Result; }
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"states\":5,\"tasks\":4,\"compiled\":true,\"saved\":1}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::CorrectCheckpointAMeleeStrikeRange()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	UPREnemyAttackDataAsset* const Strike = LoadObject<UPREnemyAttackDataAsset>(
		nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_MeleeStrike.DA_EnemyAttack_MeleeStrike"));
	const FGameplayTag ExpectedAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.MeleeStrike"));
	if (!Strike
		|| Strike->AttackId != TEXT("MeleeStrike")
		|| Strike->AttackTag != ExpectedAttackTag
		|| !FMath::IsNearlyEqual(Strike->MinRange, 0.0f)
		|| !FMath::IsNearlyEqual(Strike->MaxRange, 140.0f)
		|| !FMath::IsFinite(Strike->SweepRadius)
		|| (!FMath::IsNearlyEqual(Strike->SweepRadius, 75.0f) && !FMath::IsNearlyEqual(Strike->SweepRadius, 140.0f)))
	{
		Result->SetError(TEXT("The fixed checkpoint-A melee-strike preimage does not match; no package was changed."));
		return Result;
	}

	if (FMath::IsNearlyEqual(Strike->SweepRadius, 140.0f))
	{
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"package\":\"/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_MeleeStrike\",\"sweepRadius\":140.0,\"saved\":0}"));
		return Result;
	}

	Strike->SweepRadius = 140.0f;
	Strike->MarkPackageDirty();
	FString Error;
	if (!PREnemyAuthoringToolset::SavePackageObject(Strike, Error))
	{
		Result->SetError(Error);
		return Result;
	}

	Result->SetValue(TEXT("{\"status\":\"PASS\",\"package\":\"/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_MeleeStrike\",\"sweepRadius\":140.0,\"saved\":1}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::CreateCheckpointBEnemyAssets()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	FString Error;
	if (!PREnemyAuthoringToolset::PreflightCheckpointB(Error))
	{
		Result->SetError(Error);
		return Result;
	}

	UPREnemyPrototypeRegistryDataAsset* Registry = LoadObject<UPREnemyPrototypeRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	UBlueprint* BaseBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_Base.BP_Enemy_Base"));
	if (!Registry || !BaseBlueprint || !BaseBlueprint->GeneratedClass)
	{
		Result->SetError(TEXT("Checkpoint B requires the saved checkpoint-A Registry and BP_Enemy_Base."));
		return Result;
	}

	UBlueprint* RangedBlueprint = PREnemyAuthoringToolset::CreateBlueprint(
		PREnemyAuthoringToolset::CheckpointBPackages[0], BaseBlueprint->GeneratedClass);
	UPREnemyPrototypeDataAsset* Ranged = PREnemyAuthoringToolset::CreateAsset<UPREnemyPrototypeDataAsset>(
		PREnemyAuthoringToolset::CheckpointBPackages[1]);
	UPREnemyAttackDataAsset* RangedShot = PREnemyAuthoringToolset::CreateAsset<UPREnemyAttackDataAsset>(
		PREnemyAuthoringToolset::CheckpointBPackages[2]);
	UBlueprint* RangedAbilityBlueprint = PREnemyAuthoringToolset::CreateBlueprint(
		PREnemyAuthoringToolset::CheckpointBPackages[3], UPREnemyAttackGameplayAbility::StaticClass());
	UPRAbilitySetDataAsset* RangedAbilitySet = PREnemyAuthoringToolset::CreateAsset<UPRAbilitySetDataAsset>(
		PREnemyAuthoringToolset::CheckpointBPackages[4]);
	UBlueprint* RangedCooldownBlueprint = PREnemyAuthoringToolset::CreateGameplayEffectBlueprint(
		PREnemyAuthoringToolset::CheckpointBPackages[5]);
	UNiagaraSystem* RangedVFX = PREnemyAuthoringToolset::CreateNiagaraSystem(
		PREnemyAuthoringToolset::CheckpointBPackages[6]);
	USoundWave* RangedSFX = PREnemyAuthoringToolset::CreatePlaceholderSound(
		PREnemyAuthoringToolset::CheckpointBPackages[7]);
	UBlueprint* ProjectileBlueprint = PREnemyAuthoringToolset::CreateBlueprint(
		PREnemyAuthoringToolset::CheckpointBPackages[8], APREnemyProjectile::StaticClass());
	if (!RangedBlueprint || !Ranged || !RangedShot || !RangedAbilityBlueprint || !RangedAbilitySet || !RangedCooldownBlueprint
		|| !RangedVFX || !RangedSFX || !ProjectileBlueprint)
	{
		Result->SetError(TEXT("Checkpoint B could not create its complete fixed manifest."));
		return Result;
	}
	if (!PREnemyAuthoringToolset::ConfigureCheckpointBAssets(Registry, RangedBlueprint, Ranged, RangedShot,
		RangedAbilityBlueprint, RangedAbilitySet, RangedCooldownBlueprint, RangedVFX, RangedSFX, ProjectileBlueprint, Error))
	{
		Result->SetError(Error);
		return Result;
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":9,\"registryAppended\":1,\"saved\":false,\"schema\":\"v0.2.1-B-fixed\"}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::CreateCheckpointCEnemyAssets()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	FString Error;
	if (!PREnemyAuthoringToolset::PreflightCheckpointC(Error))
	{
		Result->SetError(Error);
		return Result;
	}

	UPREnemyPrototypeRegistryDataAsset* Registry = LoadObject<UPREnemyPrototypeRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	UBlueprint* BaseBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_Base.BP_Enemy_Base"));
	if (!Registry || !BaseBlueprint || !BaseBlueprint->GeneratedClass)
	{
		Result->SetError(TEXT("Checkpoint C requires the saved checkpoint-A Registry and BP_Enemy_Base."));
		return Result;
	}

	UBlueprint* ShieldBlueprint = PREnemyAuthoringToolset::CreateBlueprint(
		PREnemyAuthoringToolset::CheckpointCPackages[0], BaseBlueprint->GeneratedClass);
	UPREnemyPrototypeDataAsset* Shield = PREnemyAuthoringToolset::CreateAsset<UPREnemyPrototypeDataAsset>(
		PREnemyAuthoringToolset::CheckpointCPackages[1]);
	UPREnemyAttackDataAsset* ShieldBash = PREnemyAuthoringToolset::CreateAsset<UPREnemyAttackDataAsset>(
		PREnemyAuthoringToolset::CheckpointCPackages[2]);
	UBlueprint* ShieldAbilityBlueprint = PREnemyAuthoringToolset::CreateBlueprint(
		PREnemyAuthoringToolset::CheckpointCPackages[3], UPREnemyAttackGameplayAbility::StaticClass());
	UPRAbilitySetDataAsset* ShieldAbilitySet = PREnemyAuthoringToolset::CreateAsset<UPRAbilitySetDataAsset>(
		PREnemyAuthoringToolset::CheckpointCPackages[4]);
	UBlueprint* ShieldCooldownBlueprint = PREnemyAuthoringToolset::CreateGameplayEffectBlueprint(
		PREnemyAuthoringToolset::CheckpointCPackages[5]);
	UNiagaraSystem* ShieldVFX = PREnemyAuthoringToolset::CreateNiagaraSystem(
		PREnemyAuthoringToolset::CheckpointCPackages[6]);
	USoundWave* ShieldSFX = PREnemyAuthoringToolset::CreatePlaceholderSound(
		PREnemyAuthoringToolset::CheckpointCPackages[7]);
	if (!ShieldBlueprint || !Shield || !ShieldBash || !ShieldAbilityBlueprint || !ShieldAbilitySet
		|| !ShieldCooldownBlueprint || !ShieldVFX || !ShieldSFX)
	{
		Result->SetError(TEXT("Checkpoint C could not create its complete fixed manifest."));
		return Result;
	}
	if (!PREnemyAuthoringToolset::ConfigureCheckpointCAssets(Registry, ShieldBlueprint, Shield, ShieldBash,
		ShieldAbilityBlueprint, ShieldAbilitySet, ShieldCooldownBlueprint, ShieldVFX, ShieldSFX, Error))
	{
		Result->SetError(Error);
		return Result;
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":8,\"registryAppended\":1,\"saved\":false,\"schema\":\"v0.2.1-C-fixed\"}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::SaveCheckpointCEnemyAssets()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	const TCHAR* ExactPaths[] = {
		PREnemyAuthoringToolset::CheckpointCPackages[0], PREnemyAuthoringToolset::CheckpointCPackages[1],
		PREnemyAuthoringToolset::CheckpointCPackages[2], PREnemyAuthoringToolset::CheckpointCPackages[3],
		PREnemyAuthoringToolset::CheckpointCPackages[4], PREnemyAuthoringToolset::CheckpointCPackages[5],
		PREnemyAuthoringToolset::CheckpointCPackages[6], PREnemyAuthoringToolset::CheckpointCPackages[7],
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry")};
	TArray<UObject*> AssetsToSave;
	AssetsToSave.Reserve(UE_ARRAY_COUNT(ExactPaths));
	for (const TCHAR* Path : ExactPaths)
	{
		UObject* Asset = LoadObject<UObject>(nullptr, *FString::Printf(TEXT("%s.%s"), Path, *FPackageName::GetLongPackageAssetName(Path)));
		if (!Asset)
		{
			Result->SetError(FString::Printf(TEXT("Checkpoint C exact save requires existing package %s."), Path));
			return Result;
		}
		AssetsToSave.Add(Asset);
	}
	FString Error;
	for (UObject* Asset : AssetsToSave)
	{
		if (!PREnemyAuthoringToolset::SavePackageObject(Asset, Error))
		{
			Result->SetError(Error);
			return Result;
		}
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"saved\":9,\"schema\":\"v0.2.1-C-exact-manifest\"}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::RepairCheckpointBProjectileBlueprint()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	const TCHAR* ProjectilePath = PREnemyAuthoringToolset::CheckpointBPackages[8];
	FString Filename;
	TArray<FAssetData> Existing;
	FAssetRegistryModule::GetRegistry().GetAssetsByPackageName(FName(ProjectilePath), Existing, true);
	if (FPackageName::DoesPackageExist(ProjectilePath, &Filename) || !Existing.IsEmpty())
	{
		Result->SetError(TEXT("Checkpoint B projectile repair requires the documented absent projectile package preimage."));
		return Result;
	}
	UBlueprint* ProjectileBlueprint = PREnemyAuthoringToolset::CreateBlueprint(ProjectilePath, APREnemyProjectile::StaticClass());
	if (!ProjectileBlueprint || !ProjectileBlueprint->GeneratedClass
		|| !ProjectileBlueprint->GeneratedClass->IsChildOf(APREnemyProjectile::StaticClass()))
	{
		Result->SetError(TEXT("Checkpoint B projectile repair could not create the fixed APREnemyProjectile Blueprint."));
		return Result;
	}
	FKismetEditorUtilities::CompileBlueprint(ProjectileBlueprint);
	ProjectileBlueprint->MarkPackageDirty();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":1,\"package\":\"/Game/ProjectR/Enemies/Blueprints/BP_EnemyProjectile_Ranged\",\"saved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::RepairCheckpointBRangedBindings()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	UBlueprint* RangedBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_RangedMinion.BP_Enemy_RangedMinion"));
	UBlueprint* RangedAbilityBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Attacks/GA_Enemy_RangedShot.GA_Enemy_RangedShot"));
	UBlueprint* DefaultAttributesBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"));
	UBlueprint* RangedCooldownBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_RangedShot_Cooldown.GE_EnemyAttack_RangedShot_Cooldown"));
	UStateTree* StateTree = LoadObject<UStateTree>(nullptr,
		TEXT("/Game/ProjectR/Enemies/AI/ST_Enemy_Base.ST_Enemy_Base"));
	UClass* DamageEffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/ProjectR/Effects/GE_Damage.GE_Damage_C"));
	if (!RangedBlueprint || !RangedAbilityBlueprint || !DefaultAttributesBlueprint || !DefaultAttributesBlueprint->GeneratedClass
		|| !RangedCooldownBlueprint || !RangedCooldownBlueprint->GeneratedClass || !StateTree || !DamageEffectClass
		|| !RangedBlueprint->GeneratedClass || !RangedAbilityBlueprint->GeneratedClass)
	{
		Result->SetError(TEXT("Checkpoint B ranged-binding repair requires the complete saved B preimage."));
		return Result;
	}
	UObject* RangedCDO = RangedBlueprint->GeneratedClass->GetDefaultObject();
	UObject* AbilityCDO = RangedAbilityBlueprint->GeneratedClass->GetDefaultObject();
	FObjectProperty* StateTreeProperty = FindFProperty<FObjectProperty>(RangedBlueprint->GeneratedClass, TEXT("EnemyStateTree"));
	FClassProperty* DefaultEffectProperty = FindFProperty<FClassProperty>(RangedBlueprint->GeneratedClass, TEXT("DefaultAttributesEffect"));
	FClassProperty* DamageEffectProperty = FindFProperty<FClassProperty>(RangedBlueprint->GeneratedClass, TEXT("DamageEffect"));
	FStructProperty* AbilityTagProperty = FindFProperty<FStructProperty>(RangedAbilityBlueprint->GeneratedClass, TEXT("AbilityTag"));
	FClassProperty* CooldownProperty = FindFProperty<FClassProperty>(RangedAbilityBlueprint->GeneratedClass, TEXT("CooldownGameplayEffectClass"));
	if (!RangedCDO || !AbilityCDO || !StateTreeProperty || !DefaultEffectProperty || !DamageEffectProperty || !AbilityTagProperty || !CooldownProperty)
	{
		Result->SetError(TEXT("Checkpoint B ranged-binding repair found an incompatible native CDO schema."));
		return Result;
	}
	StateTreeProperty->SetObjectPropertyValue_InContainer(RangedCDO, StateTree);
	DefaultEffectProperty->SetPropertyValue_InContainer(RangedCDO, DefaultAttributesBlueprint->GeneratedClass);
	DamageEffectProperty->SetPropertyValue_InContainer(RangedCDO, DamageEffectClass);
	*AbilityTagProperty->ContainerPtrToValuePtr<FGameplayTag>(AbilityCDO) = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.RangedShot"));
	CooldownProperty->SetPropertyValue_InContainer(AbilityCDO, RangedCooldownBlueprint->GeneratedClass);
	RangedBlueprint->MarkPackageDirty();
	RangedAbilityBlueprint->MarkPackageDirty();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"packages\":[\"/Game/ProjectR/Enemies/Blueprints/BP_Enemy_RangedMinion\",\"/Game/ProjectR/Enemies/Attacks/GA_Enemy_RangedShot\"],\"saved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPREnemyAuthoringToolset::RepairCheckpointBDefaultAttributeModifierOrder()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PREnemyAuthoringToolset::FScopedToolResultRoot ResultRoot(Result);
	UBlueprint* DefaultsBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"));
	UGameplayEffect* Defaults = DefaultsBlueprint && DefaultsBlueprint->GeneratedClass
		? DefaultsBlueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	if (!DefaultsBlueprint || !Defaults)
	{
		Result->SetError(TEXT("Checkpoint B default-attribute order repair requires the saved shared enemy default-attributes GameplayEffect."));
		return Result;
	}
	const TArray<FGameplayAttribute> RequiredOrder = {
		UPRAttributeSet::GetMaxHealthAttribute(), UPRAttributeSet::GetHealthAttribute(),
		UPRAttributeSet::GetMaxShieldAttribute(), UPRAttributeSet::GetShieldAttribute(),
		UPRAttributeSet::GetMaxEnergyAttribute(), UPRAttributeSet::GetEnergyAttribute(),
		UPRAttributeSet::GetAttackPowerAttribute(), UPRAttributeSet::GetMoveSpeedAttribute(),
		UPRAttributeSet::GetCritChanceAttribute(), UPRAttributeSet::GetPermissionAttribute(),
		UPRAttributeSet::GetResonanceAttribute() };
	if (Defaults->Modifiers.Num() != RequiredOrder.Num())
	{
		Result->SetError(TEXT("Checkpoint B default-attribute order repair requires the exact eleven-modifier A preimage."));
		return Result;
	}
	TArray<FGameplayModifierInfo> OrderedModifiers;
	OrderedModifiers.Reserve(RequiredOrder.Num());
	for (const FGameplayAttribute RequiredAttribute : RequiredOrder)
	{
		const FGameplayModifierInfo* Found = Defaults->Modifiers.FindByPredicate(
			[RequiredAttribute](const FGameplayModifierInfo& Modifier) { return Modifier.Attribute == RequiredAttribute; });
		if (!Found)
		{
			Result->SetError(TEXT("Checkpoint B default-attribute order repair found an incompatible modifier schema."));
			return Result;
		}
		OrderedModifiers.Add(*Found);
	}
	Defaults->Modifiers = MoveTemp(OrderedModifiers);
	DefaultsBlueprint->MarkPackageDirty();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"package\":\"/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes\",\"modifierOrder\":\"MaxThenCurrent\",\"saved\":false}"));
	return Result;
}
