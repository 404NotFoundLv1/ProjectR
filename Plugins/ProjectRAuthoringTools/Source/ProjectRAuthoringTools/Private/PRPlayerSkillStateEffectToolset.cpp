// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRPlayerSkillStateEffectToolset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Misc/PackageName.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
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
