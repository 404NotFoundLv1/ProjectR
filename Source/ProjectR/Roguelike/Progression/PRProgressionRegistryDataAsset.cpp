// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/Progression/PRProgressionRegistryDataAsset.h"

#include "Misc/DataValidation.h"
#include "Roguelike/Progression/PRProgressionNodeDataAsset.h"

namespace PRProgressionRegistryPrivate
{
const FPrimaryAssetType NodeType(TEXT("ProgressionNode"));

struct FExpectedNode
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

const FExpectedNode ExpectedNodes[] = {
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

bool MatchesExpectedNode(const UPRProgressionNodeDataAsset& Node, const FExpectedNode& Expected)
{
	if (Node.GetPrimaryAssetId() != FPrimaryAssetId(NodeType, FName(Expected.Name))
		|| Node.Tree != Expected.Tree || Node.EffectKind != Expected.EffectKind
		|| Node.CounterproofCost != Expected.CounterproofCost || Node.MemoryFragmentCost != 0
		|| Node.RelationshipRequirement.Metric != Expected.RelationshipMetric
		|| Node.RelationshipRequirement.Scope != Expected.RelationshipScope
		|| Node.RelationshipRequirement.MinimumValue != Expected.RelationshipMinimum
		|| Node.PrerequisiteNodeIds.Num() != static_cast<int32>(Expected.Prerequisites.size()))
	{
		return false;
	}

	int32 Index = 0;
	for (const TCHAR* Prerequisite : Expected.Prerequisites)
	{
		if (Node.PrerequisiteNodeIds[Index++] != FPrimaryAssetId(NodeType, FName(Prerequisite))) return false;
	}
	return true;
}
}

FPrimaryAssetId UPRProgressionRegistryDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRProgressionRegistry")), GetFName());
}

const UPRProgressionNodeDataAsset* UPRProgressionRegistryDataAsset::FindNode(const FPrimaryAssetId NodeId) const
{
	for (const TSoftObjectPtr<UPRProgressionNodeDataAsset>& Reference : Nodes)
	{
		const UPRProgressionNodeDataAsset* Node = Reference.LoadSynchronous();
		if (Node && Node->GetPrimaryAssetId() == NodeId) return Node;
	}
	return nullptr;
}

bool UPRProgressionRegistryDataAsset::IsRegistryReady() const
{
	if (Nodes.Num() != UE_ARRAY_COUNT(PRProgressionRegistryPrivate::ExpectedNodes)) return false;
	FString Previous;
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const UPRProgressionNodeDataAsset* Node = Nodes[Index].LoadSynchronous();
		if (!Node || !Node->IsNodeDefinitionValid()) return false;
		const FPrimaryAssetId Id = Node->GetPrimaryAssetId();
		if (Id.PrimaryAssetType != PRProgressionRegistryPrivate::NodeType
			|| !PRProgressionRegistryPrivate::MatchesExpectedNode(*Node, PRProgressionRegistryPrivate::ExpectedNodes[Index])) return false;
		const FString Current = Id.ToString();
		if (!Previous.IsEmpty() && Previous >= Current) return false;
		Previous = Current;
	}
	return true;
}

EDataValidationResult UPRProgressionRegistryDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!IsRegistryReady())
	{
		Context.AddError(FText::FromString(TEXT("Progression registry must contain the exact twelve fixed nodes in PrimaryAssetId order.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
