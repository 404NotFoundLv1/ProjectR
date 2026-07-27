// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRProgressionAuthoringToolset.h"

#include "Abilities/PRAttributeSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "GameplayEffect.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Roguelike/Progression/PRProgressionNodeDataAsset.h"
#include "Roguelike/Progression/PRProgressionRegistryDataAsset.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace PRProgressionAuthoring
{
const FPrimaryAssetType NodeType(TEXT("ProgressionNode"));

struct FRoot
{
	explicit FRoot(UToolCallAsyncResultString* InResult) : Result(InResult) { check(Result); Result->AddToRoot(); }
	~FRoot() { Result->RemoveFromRoot(); }
	UToolCallAsyncResultString* Result;
};

struct FNodeDefinition
{
	const TCHAR* Name;
	EPRProgressionTree Tree;
	EPRProgressionEffectKind EffectKind;
	int32 CounterproofCost;
	EPRProgressionRelationshipMetric RelationshipMetric;
	EPRProgressionRelationshipScope RelationshipScope;
	int32 RelationshipMinimum;
	std::initializer_list<const TCHAR*> Prerequisites;
};

const FNodeDefinition Nodes[] = {
	{TEXT("AINearDeathProtection"), EPRProgressionTree::CompanionAI, EPRProgressionEffectKind::EntitlementOnly, 2, EPRProgressionRelationshipMetric::None, EPRProgressionRelationshipScope::None, 0, {TEXT("AIQTE"), TEXT("AISupport")}},
	{TEXT("AIOverloadRepair"), EPRProgressionTree::CompanionAI, EPRProgressionEffectKind::EntitlementOnly, 2, EPRProgressionRelationshipMetric::None, EPRProgressionRelationshipScope::None, 0, {TEXT("AIQTE"), TEXT("AISupport")}},
	{TEXT("AIQTE"), EPRProgressionTree::CompanionAI, EPRProgressionEffectKind::EntitlementOnly, 1, EPRProgressionRelationshipMetric::None, EPRProgressionRelationshipScope::None, 0, {}},
	{TEXT("AISupport"), EPRProgressionTree::CompanionAI, EPRProgressionEffectKind::CompanionSupportInterval, 1, EPRProgressionRelationshipMetric::None, EPRProgressionRelationshipScope::None, 0, {}},
	{TEXT("BondAdvancedCombo"), EPRProgressionTree::Bond, EPRProgressionEffectKind::EntitlementOnly, 2, EPRProgressionRelationshipMetric::Evaluation, EPRProgressionRelationshipScope::PrimaryCompanion, 60, {TEXT("AIQTE"), TEXT("PlayerSkillEnhancement")}},
	{TEXT("BondStory"), EPRProgressionTree::Bond, EPRProgressionEffectKind::EntitlementOnly, 1, EPRProgressionRelationshipMetric::Trust, EPRProgressionRelationshipScope::AnyCompanion, 60, {}},
	{TEXT("BondTripleResonance"), EPRProgressionTree::Bond, EPRProgressionEffectKind::EntitlementOnly, 4, EPRProgressionRelationshipMetric::Trust, EPRProgressionRelationshipScope::AllCompanions, 70, {TEXT("BondAdvancedCombo"), TEXT("BondStory"), TEXT("BondVoice")}},
	{TEXT("BondVoice"), EPRProgressionTree::Bond, EPRProgressionEffectKind::EntitlementOnly, 1, EPRProgressionRelationshipMetric::Affection, EPRProgressionRelationshipScope::AnyCompanion, 60, {TEXT("BondStory")}},
	{TEXT("PlayerMaxEnergy"), EPRProgressionTree::Player, EPRProgressionEffectKind::PlayerMaxEnergy, 1, EPRProgressionRelationshipMetric::None, EPRProgressionRelationshipScope::None, 0, {}},
	{TEXT("PlayerMaxHealth"), EPRProgressionTree::Player, EPRProgressionEffectKind::PlayerMaxHealth, 1, EPRProgressionRelationshipMetric::None, EPRProgressionRelationshipScope::None, 0, {}},
	{TEXT("PlayerSkillEnhancement"), EPRProgressionTree::Player, EPRProgressionEffectKind::EntitlementOnly, 3, EPRProgressionRelationshipMetric::None, EPRProgressionRelationshipScope::None, 0, {TEXT("PlayerSkillSlot")}},
	{TEXT("PlayerSkillSlot"), EPRProgressionTree::Player, EPRProgressionEffectKind::EntitlementOnly, 2, EPRProgressionRelationshipMetric::None, EPRProgressionRelationshipScope::None, 0, {TEXT("PlayerMaxEnergy"), TEXT("PlayerMaxHealth")}}
};

const TCHAR* const RegistryPackage = TEXT("/Game/ProjectR/Data/Progression/DA_ProgressionRegistry");
const TCHAR* const HealthEffectPackage = TEXT("/Game/ProjectR/Effects/Progression/GE_Progression_PlayerMaxHealth");
const TCHAR* const EnergyEffectPackage = TEXT("/Game/ProjectR/Effects/Progression/GE_Progression_PlayerMaxEnergy");

FString NodePackagePath(const FNodeDefinition& Definition)
{
	return FString::Printf(TEXT("/Game/ProjectR/Data/Progression/Nodes/DA_ProgressionNode_%s"), Definition.Name);
}

FString ObjectPath(const FString& PackagePath)
{
	return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
}

bool IsOccupied(const FString& PackagePath)
{
	return FPackageName::DoesPackageExist(PackagePath) || FindObject<UObject>(nullptr, *ObjectPath(PackagePath)) != nullptr;
}

bool SaveExact(UObject* Asset, FString& OutError)
{
	if (!Asset) { OutError = TEXT("Missing fixed manifest asset."); return false; }
	FSavePackageArgs Arguments;
	Arguments.TopLevelFlags = RF_Public | RF_Standalone;
	Arguments.Error = GError;
	const FString PackageName = Asset->GetOutermost()->GetName();
	if (!UPackage::SavePackage(Asset->GetOutermost(), Asset,
		*FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()), Arguments))
	{
		OutError = FString::Printf(TEXT("Failed to save exact manifest package: %s"), *PackageName);
		return false;
	}
	if (Asset->GetOutermost()->IsDirty())
	{
		OutError = FString::Printf(TEXT("Manifest package remained dirty after exact save: %s"), *PackageName);
		return false;
	}
	return true;
}

UPRProgressionNodeDataAsset* CreateNode(const FNodeDefinition& Definition)
{
	const FString PackagePath = NodePackagePath(Definition);
	UPackage* Package = CreatePackage(*PackagePath);
	UPRProgressionNodeDataAsset* Node = NewObject<UPRProgressionNodeDataAsset>(Package,
		FName(FPackageName::GetLongPackageAssetName(PackagePath)), RF_Public | RF_Standalone);
	if (!Node) return nullptr;
	Node->NodeId = FPrimaryAssetId(NodeType, FName(Definition.Name));
	Node->Tree = Definition.Tree;
	Node->EffectKind = Definition.EffectKind;
	Node->CounterproofCost = Definition.CounterproofCost;
	Node->MemoryFragmentCost = 0;
	Node->RelationshipRequirement.Metric = Definition.RelationshipMetric;
	Node->RelationshipRequirement.Scope = Definition.RelationshipScope;
	Node->RelationshipRequirement.MinimumValue = Definition.RelationshipMinimum;
	Node->PrerequisiteNodeIds.Reset();
	for (const TCHAR* Prerequisite : Definition.Prerequisites)
	{
		Node->PrerequisiteNodeIds.Add(FPrimaryAssetId(NodeType, FName(Prerequisite)));
	}
	Node->DisplayName = FText::FromString(Definition.Name);
	Node->Description = FText::FromString(TEXT("Fixed v0.4.4 progression node."));
	Node->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Node);
	return Node;
}

UBlueprint* CreateEffectBlueprint(const TCHAR* PackagePath, const FGameplayAttribute Attribute)
{
	const FString LongPackageName(PackagePath);
	UPackage* Package = CreatePackage(*LongPackageName);
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(UGameplayEffect::StaticClass(), Package,
		FName(FPackageName::GetLongPackageAssetName(LongPackageName)), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	if (!Blueprint || !Blueprint->GeneratedClass) return nullptr;
	UGameplayEffect* Effect = Blueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	if (!Effect) return nullptr;
	Effect->Modify();
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	Effect->Period = FScalableFloat(0.0f);
	Effect->Modifiers.Reset();
	Effect->Executions.Reset();
	Effect->GameplayCues.Reset();
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.0f));
	Blueprint->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	FAssetRegistryModule::AssetCreated(Blueprint);
	return Blueprint;
}

bool ValidateEffect(const TCHAR* PackagePath, const FGameplayAttribute Attribute)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath(PackagePath));
	UGameplayEffect* Effect = Blueprint && Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	if (!Effect || Effect->DurationPolicy != EGameplayEffectDurationType::Infinite || Effect->Modifiers.Num() != 1) return false;
	const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
	float StaticMagnitude = 0.0f;
	return Modifier.Attribute == Attribute && Modifier.ModifierOp == EGameplayModOp::Additive
		&& Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, StaticMagnitude)
		&& FMath::IsNearlyEqual(StaticMagnitude, 10.0f);
}

bool ValidateManifest(FString& OutError)
{
	UPRProgressionRegistryDataAsset* Registry = LoadObject<UPRProgressionRegistryDataAsset>(nullptr, *ObjectPath(RegistryPackage));
	if (!Registry || !Registry->IsRegistryReady()) { OutError = TEXT("Fixed registry is missing or violates the twelve-node contract."); return false; }
	if (!ValidateEffect(HealthEffectPackage, UPRAttributeSet::GetMaxHealthAttribute())) { OutError = TEXT("MaxHealth effect does not contain the fixed +10 infinite modifier."); return false; }
	if (!ValidateEffect(EnergyEffectPackage, UPRAttributeSet::GetMaxEnergyAttribute())) { OutError = TEXT("MaxEnergy effect does not contain the fixed +10 infinite modifier."); return false; }
	return true;
}
}

UToolCallAsyncResultString* UPRProgressionAuthoringToolset::CreateAndSaveFixedProgressionManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PRProgressionAuthoring::FRoot Root(Result);
	using namespace PRProgressionAuthoring;
	TArray<FString> Packages = { RegistryPackage, HealthEffectPackage, EnergyEffectPackage };
	for (const FNodeDefinition& Definition : Nodes) Packages.Add(NodePackagePath(Definition));
	for (const FString& PackagePath : Packages)
	{
		if (IsOccupied(PackagePath))
		{
			Result->SetError(FString::Printf(TEXT("v0.4.4 exact create collision: %s"), *PackagePath));
			return Result;
		}
	}

	TArray<UPRProgressionNodeDataAsset*> CreatedNodes;
	for (const FNodeDefinition& Definition : Nodes)
	{
		UPRProgressionNodeDataAsset* Node = CreateNode(Definition);
		if (!Node || !Node->IsNodeDefinitionValid()) { Result->SetError(TEXT("Fixed progression node creation failed.")); return Result; }
		CreatedNodes.Add(Node);
	}
	UPackage* RegistryOuter = CreatePackage(RegistryPackage);
	UPRProgressionRegistryDataAsset* Registry = NewObject<UPRProgressionRegistryDataAsset>(RegistryOuter,
		FName(FPackageName::GetLongPackageAssetName(RegistryPackage)), RF_Public | RF_Standalone);
	if (!Registry) { Result->SetError(TEXT("Fixed progression registry creation failed.")); return Result; }
	Registry->Nodes.Reset();
	for (UPRProgressionNodeDataAsset* Node : CreatedNodes) Registry->Nodes.Add(Node);
	Registry->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Registry);
	if (!Registry->IsRegistryReady()) { Result->SetError(TEXT("Fixed progression registry configuration failed.")); return Result; }

	UBlueprint* HealthEffect = CreateEffectBlueprint(HealthEffectPackage, UPRAttributeSet::GetMaxHealthAttribute());
	UBlueprint* EnergyEffect = CreateEffectBlueprint(EnergyEffectPackage, UPRAttributeSet::GetMaxEnergyAttribute());
	if (!HealthEffect || !EnergyEffect) { Result->SetError(TEXT("Fixed progression GameplayEffect creation failed.")); return Result; }

	FString SaveError;
	for (UPRProgressionNodeDataAsset* Node : CreatedNodes)
	{
		if (!SaveExact(Node, SaveError)) { Result->SetError(SaveError); return Result; }
	}
	if (!SaveExact(Registry, SaveError) || !SaveExact(HealthEffect, SaveError) || !SaveExact(EnergyEffect, SaveError))
	{
		Result->SetError(SaveError);
		return Result;
	}
	if (!ValidateManifest(SaveError)) { Result->SetError(SaveError); return Result; }
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":15,\"saved\":15,\"dirty\":0,\"mapsSaved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRProgressionAuthoringToolset::ValidateFixedProgressionManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PRProgressionAuthoring::FRoot Root(Result);
	FString Error;
	if (!PRProgressionAuthoring::ValidateManifest(Error)) { Result->SetError(Error); return Result; }
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"validated\":15,\"mapsSaved\":false}"));
	return Result;
}
