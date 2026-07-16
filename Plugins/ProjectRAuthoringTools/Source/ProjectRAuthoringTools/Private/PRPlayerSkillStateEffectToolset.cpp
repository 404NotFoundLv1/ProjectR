// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRPlayerSkillStateEffectToolset.h"

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Misc/PackageName.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"

namespace PRPlayerSkillStateEffectToolset
{
UGameplayEffect* CreateEffect(const TCHAR* PackagePath, const FGameplayTag& Tag, const float Duration, FString& Error)
{
	if (FindObject<UGameplayEffect>(nullptr, PackagePath))
	{
		Error = FString::Printf(TEXT("Manifest package already exists: %s"), PackagePath);
		return nullptr;
	}
	const FString LongName(PackagePath);
	const FString AssetName = FPackageName::GetLongPackageAssetName(LongName);
	UPackage* Package = CreatePackage(*LongName);
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(Package, *AssetName, RF_Public | RF_Standalone);
	if (!Effect)
	{
		Error = FString::Printf(TEXT("Could not create %s"), PackagePath);
		return nullptr;
	}
	Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
	Effect->Period = FScalableFloat(0.0f);
	Effect->Modifiers.Reset();
	Effect->Executions.Reset();
	Effect->GameplayCues.Reset();
	FInheritedTagContainer Tags;
	Tags.AddTag(Tag);
	Effect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(Tags);
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Effect);
	return Effect;
}
}

UToolCallAsyncResultString* UPRPlayerSkillStateEffectToolset::CreateCheckpointCStateEffects()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	FString Error;
	UGameplayEffect* Stunned = PRPlayerSkillStateEffectToolset::CreateEffect(
		TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Stunned"), UPRTagLibrary::GetStateStunnedTag(), 0.75f, Error);
	UGameplayEffect* Invulnerable = Stunned ? PRPlayerSkillStateEffectToolset::CreateEffect(
		TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Invulnerable"), UPRTagLibrary::GetStateInvulnerableTag(), 0.22f, Error) : nullptr;
	if (!Stunned || !Invulnerable)
	{
		Result->SetError(Error);
		return Result;
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":2,\"saved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRPlayerSkillStateEffectToolset::ConfigureCheckpointDGuardingBinding()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UBlueprint* AbilityBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/ProjectR/Abilities/Skills/GA_Skill_CounterProofWall.GA_Skill_CounterProofWall"));
	UClass* GuardingClass = LoadClass<UGameplayEffect>(
		nullptr,
		TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Guarding.GE_State_Guarding_C"));
	if (!AbilityBlueprint || !AbilityBlueprint->GeneratedClass || !GuardingClass)
	{
		Result->SetError(TEXT("Checkpoint D Guarding binding assets are unavailable."));
		return Result;
	}
	UGameplayEffect* GuardingEffect = GuardingClass->GetDefaultObject<UGameplayEffect>();
	UBlueprint* GuardingBlueprint = Cast<UBlueprint>(GuardingClass->ClassGeneratedBy);
	if (!GuardingEffect || !GuardingBlueprint)
	{
		Result->SetError(TEXT("Checkpoint D Guarding effect Blueprint default object is unavailable."));
		return Result;
	}
	GuardingEffect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	GuardingEffect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f));
	GuardingEffect->Period = FScalableFloat(0.0f);
	GuardingEffect->Modifiers.Reset();
	GuardingEffect->Executions.Reset();
	GuardingEffect->GameplayCues.Reset();
	FInheritedTagContainer Tags;
	Tags.AddTag(UPRTagLibrary::GetStateGuardingTag());
	GuardingEffect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(Tags);
	GuardingBlueprint->MarkPackageDirty();
	FClassProperty* Property = FindFProperty<FClassProperty>(
		AbilityBlueprint->GeneratedClass,
		TEXT("GuardingEffectClass"));
	UObject* CDO = AbilityBlueprint->GeneratedClass->GetDefaultObject();
	if (!Property || !CDO || !GuardingClass->IsChildOf(UGameplayEffect::StaticClass()))
	{
		Result->SetError(TEXT("Checkpoint D Guarding binding property is unavailable."));
		return Result;
	}
	Property->SetPropertyValue_InContainer(CDO, GuardingClass);
	AbilityBlueprint->MarkPackageDirty();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"configured\":1,\"saved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRPlayerSkillStateEffectToolset::ConfigureCheckpointDAbilitySet()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UPRAbilitySetDataAsset* AbilitySet = LoadObject<UPRAbilitySetDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/DA_DefaultAbilitySet.DA_DefaultAbilitySet"));
	UPRPlayerSkillDataAsset* VectorHook = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_VectorHook.DA_Skill_VectorHook"));
	UPRPlayerSkillDataAsset* CounterProofWall = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_CounterProofWall.DA_Skill_CounterProofWall"));
	if (!AbilitySet || !VectorHook || !CounterProofWall
		|| !VectorHook->AbilityClass || !CounterProofWall->AbilityClass
		|| AbilitySet->AbilityEntries.Num() != 4)
	{
		Result->SetError(TEXT("Checkpoint D AbilitySet preimage or formal D skill data is unavailable."));
		return Result;
	}
	const TCHAR* FrozenTags[] = {
		TEXT("Skill.ShadowThrust"), TEXT("Skill.FireSlash"),
		TEXT("Skill.ThunderDrop"), TEXT("Skill.AfterimageDodge")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(FrozenTags); ++Index)
	{
		const UPRPlayerSkillDataAsset* FrozenData =
			Cast<UPRPlayerSkillDataAsset>(AbilitySet->AbilityEntries[Index].AbilityData);
		if (!FrozenData || FrozenData->AbilityTag.ToString() != FString(FrozenTags[Index]))
		{
			Result->SetError(TEXT("Checkpoint D refuses to modify a non-frozen AbilitySet preimage."));
			return Result;
		}
	}
	auto AppendEntry = [AbilitySet](UPRPlayerSkillDataAsset* Data)
	{
		FPRAbilitySetEntry& Entry = AbilitySet->AbilityEntries.AddDefaulted_GetRef();
		Entry.AbilityClass = Data->AbilityClass;
		Entry.AbilityLevel = 1;
		Entry.InputTag = Data->InputTag;
		Entry.bGrantOnInitialization = true;
		Entry.GrantedSpecTags.Reset();
		Entry.AbilityData = Data;
	};
	AbilitySet->Modify();
	AppendEntry(VectorHook);
	AppendEntry(CounterProofWall);
	AbilitySet->MarkPackageDirty();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"entries\":6,\"saved\":false}"));
	return Result;
}
