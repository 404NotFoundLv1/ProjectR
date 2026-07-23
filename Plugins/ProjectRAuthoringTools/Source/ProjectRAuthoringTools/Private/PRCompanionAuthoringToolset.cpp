// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRCompanionAuthoringToolset.h"

#include "Abilities/PRAttributeSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Companions/PRCompanionPawn.h"
#include "Companions/PRCompanionRuntimeDataAsset.h"
#include "Companions/PRCompanionTypes.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "GameplayEffect.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace PRCompanionAuthoring
{
struct FRuntimeDefinition
{
	const TCHAR* RuntimePackage;
	const TCHAR* PawnPackage;
	FGameplayTag CompanionId;
	EPRCompanionSupportType SupportType;
	float Interval;
	float Range;
	float Magnitude;
	const TCHAR* EffectPath;
};

constexpr const TCHAR* ShieldEffectPackage = TEXT("/Game/ProjectR/Companions/Effects/GE_Companion_Axiom_Shield");
constexpr const TCHAR* StunnedEffectClassPath = TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Stunned.GE_State_Stunned_C");

bool PackageExists(const TCHAR* PackagePath)
{
	return FPackageName::DoesPackageExist(PackagePath);
}

const TCHAR* ManifestPackages[] = {
	TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Axiom"),
	TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Kindle"),
	TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Null"),
	TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Axiom"),
	TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Kindle"),
	TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Null"),
	ShieldEffectPackage
};

bool VerifyCreatePreflight(FString& OutError)
{
	for (const TCHAR* PackagePath : ManifestPackages)
	{
		if (PackageExists(PackagePath))
		{
			OutError = FString::Printf(TEXT("v0.3.1 manifest collision: %s already exists; creation will not overwrite it."), PackagePath);
			return false;
		}
	}
	return true;
}

UBlueprint* CreateBlueprint(const TCHAR* PackagePath, UClass* ParentClass, FString& OutError)
{
	const FString PackageName(PackagePath);
	UPackage* Package = CreatePackage(*PackageName);
	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass, Package, AssetName, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), NAME_None);
	if (!Blueprint)
	{
		OutError = FString::Printf(TEXT("Could not create Blueprint %s."), PackagePath);
		return nullptr;
	}
	FAssetRegistryModule::AssetCreated(Blueprint);
	Blueprint->MarkPackageDirty();
	return Blueprint;
}

UPRCompanionRuntimeDataAsset* CreateRuntimeAsset(const TCHAR* PackagePath, FString& OutError)
{
	const FString PackageName(PackagePath);
	UPackage* Package = CreatePackage(*PackageName);
	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
	UPRCompanionRuntimeDataAsset* Asset = NewObject<UPRCompanionRuntimeDataAsset>(Package, AssetName, RF_Public | RF_Standalone);
	if (!Asset)
	{
		OutError = FString::Printf(TEXT("Could not create runtime DataAsset %s."), PackagePath);
		return nullptr;
	}
	FAssetRegistryModule::AssetCreated(Asset);
	return Asset;
}

bool VerifyOnlyManifestPackagesAreDirty(FString& OutError)
{
	TSet<FString> AllowedPackages;
	for (const TCHAR* PackagePath : ManifestPackages) AllowedPackages.Add(PackagePath);
	TArray<UPackage*> DirtyPackages;
	FEditorFileUtils::GetDirtyContentPackages(DirtyPackages);
	for (UPackage* DirtyPackage : DirtyPackages)
	{
		if (DirtyPackage && !AllowedPackages.Contains(DirtyPackage->GetName()))
		{
			OutError = FString::Printf(TEXT("Unexpected dirty package blocks exact v0.3.1 save: %s."), *DirtyPackage->GetName());
			return false;
		}
	}
	return true;
}

bool SaveManifestObject(const TCHAR* PackagePath, FString& OutError)
{
	const FString PackageName(PackagePath);
	const FString ObjectPath = PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName);
	UObject* Asset = LoadObject<UObject>(nullptr, *ObjectPath);
	if (!Asset)
	{
		OutError = FString::Printf(TEXT("Exact v0.3.1 save could not load %s."), PackagePath);
		return false;
	}
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.Error = GError;
	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	if (!UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args))
	{
		OutError = FString::Printf(TEXT("Exact v0.3.1 manifest save failed: %s."), PackagePath);
		return false;
	}
	return true;
}

void ConfigureShieldEffect(UGameplayEffect& Effect)
{
	Effect.DurationPolicy = EGameplayEffectDurationType::Instant;
	Effect.Period = FScalableFloat(0.0f);
	Effect.Modifiers.Reset();
	Effect.Executions.Reset();
	Effect.GameplayCues.Reset();
	FGameplayModifierInfo& Modifier = Effect.Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UPRAttributeSet::GetShieldAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(20.0f));
}
}

UToolCallAsyncResultString* UPRCompanionAuthoringToolset::CreateV031CompanionManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	FString Error;
	if (!PRCompanionAuthoring::VerifyCreatePreflight(Error))
	{
		Result->SetError(Error);
		return Result;
	}

	UClass* StunnedEffectClass = LoadClass<UGameplayEffect>(nullptr, PRCompanionAuthoring::StunnedEffectClassPath);
	if (!StunnedEffectClass)
	{
		Result->SetError(TEXT("The frozen GE_State_Stunned class is unavailable."));
		return Result;
	}

	UBlueprint* AxiomPawn = PRCompanionAuthoring::CreateBlueprint(
		TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Axiom"), APRCompanionPawn::StaticClass(), Error);
	UBlueprint* KindlePawn = AxiomPawn ? PRCompanionAuthoring::CreateBlueprint(
		TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Kindle"), APRCompanionPawn::StaticClass(), Error) : nullptr;
	UBlueprint* NullPawn = KindlePawn ? PRCompanionAuthoring::CreateBlueprint(
		TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Null"), APRCompanionPawn::StaticClass(), Error) : nullptr;
	UBlueprint* ShieldEffectBlueprint = NullPawn ? PRCompanionAuthoring::CreateBlueprint(
		PRCompanionAuthoring::ShieldEffectPackage, UGameplayEffect::StaticClass(), Error) : nullptr;
	if (!ShieldEffectBlueprint || !ShieldEffectBlueprint->GeneratedClass)
	{
		Result->SetError(Error.IsEmpty() ? TEXT("Could not create the fixed Axiom Shield GameplayEffect.") : Error);
		return Result;
	}
	UGameplayEffect* ShieldEffect = ShieldEffectBlueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	if (!ShieldEffect)
	{
		Result->SetError(TEXT("The fixed Axiom Shield GameplayEffect CDO is unavailable."));
		return Result;
	}
	PRCompanionAuthoring::ConfigureShieldEffect(*ShieldEffect);

	const PRCompanionAuthoring::FRuntimeDefinition Definitions[] = {
		{ TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Axiom"), TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Axiom"), FPRCompanionContract::AxiomTag(), EPRCompanionSupportType::Shield, 8.0f, 0.0f, 20.0f, PRCompanionAuthoring::ShieldEffectPackage },
		{ TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Kindle"), TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Kindle"), FPRCompanionContract::KindleTag(), EPRCompanionSupportType::LightDamage, 4.0f, 800.0f, 12.0f, nullptr },
		{ TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Null"), TEXT("/Game/ProjectR/Companions/Blueprints/BP_Companion_Null"), FPRCompanionContract::NullTag(), EPRCompanionSupportType::ControlMark, 6.0f, 700.0f, 0.0f, PRCompanionAuthoring::StunnedEffectClassPath }};
	for (const PRCompanionAuthoring::FRuntimeDefinition& Definition : Definitions)
	{
		UPRCompanionRuntimeDataAsset* RuntimeAsset = PRCompanionAuthoring::CreateRuntimeAsset(Definition.RuntimePackage, Error);
		UBlueprint* PawnBlueprint = Definition.CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag()) ? AxiomPawn
			: (Definition.CompanionId.MatchesTagExact(FPRCompanionContract::KindleTag()) ? KindlePawn : NullPawn);
		if (!RuntimeAsset || !PawnBlueprint || !PawnBlueprint->GeneratedClass)
		{
			Result->SetError(Error.IsEmpty() ? TEXT("Could not configure a fixed Companion runtime DataAsset.") : Error);
			return Result;
		}
		RuntimeAsset->CompanionId = Definition.CompanionId;
		RuntimeAsset->PawnClass = PawnBlueprint->GeneratedClass;
		RuntimeAsset->SupportType = Definition.SupportType;
		RuntimeAsset->BaseSupportInterval = Definition.Interval;
		RuntimeAsset->SupportRange = Definition.Range;
		RuntimeAsset->SupportMagnitude = Definition.Magnitude;
		if (Definition.CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag()))
		{
			RuntimeAsset->SupportEffectClass = ShieldEffectBlueprint->GeneratedClass;
		}
		else if (Definition.CompanionId.MatchesTagExact(FPRCompanionContract::NullTag()))
		{
			RuntimeAsset->SupportEffectClass = StunnedEffectClass;
		}
		else
		{
			RuntimeAsset->SupportEffectClass = nullptr;
		}
		RuntimeAsset->MarkPackageDirty();
	}

	// Compilation is deliberately performed by the official Blueprint tool after
	// this synchronous writer returns. Compiling here can collect the MCP return
	// object before SetValue(), which makes a successful manifest creation look
	// like an Editor failure.
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":7,\"saved\":false,\"manifest\":\"v0.3.1\"}"));
	return Result;
}

UToolCallAsyncResultString* UPRCompanionAuthoringToolset::SaveV031CompanionManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	FString Error;
	if (!PRCompanionAuthoring::VerifyOnlyManifestPackagesAreDirty(Error))
	{
		Result->SetError(Error);
		return Result;
	}
	for (const TCHAR* PackagePath : PRCompanionAuthoring::ManifestPackages)
	{
		if (!PRCompanionAuthoring::SaveManifestObject(PackagePath, Error))
		{
			Result->SetError(Error);
			return Result;
		}
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"saved\":7,\"mapsSaved\":false}"));
	return Result;
}
