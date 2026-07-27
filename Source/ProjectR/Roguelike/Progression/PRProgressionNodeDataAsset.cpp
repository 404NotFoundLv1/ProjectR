// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/Progression/PRProgressionNodeDataAsset.h"

#include "Misc/DataValidation.h"

namespace PRProgressionNodePrivate
{
const FPrimaryAssetType NodeType(TEXT("ProgressionNode"));

bool IsSortedUniqueValid(const TArray<FPrimaryAssetId>& Values)
{
	FString Previous;
	for (const FPrimaryAssetId& Value : Values)
	{
		if (!Value.IsValid() || Value.PrimaryAssetType != NodeType) return false;
		const FString Current = Value.ToString();
		if (!Previous.IsEmpty() && Previous >= Current) return false;
		Previous = Current;
	}
	return true;
}
}

FPrimaryAssetId UPRProgressionNodeDataAsset::GetPrimaryAssetId() const
{
	return NodeId.IsValid() ? NodeId : FPrimaryAssetId(PRProgressionNodePrivate::NodeType, GetFName());
}

bool UPRProgressionNodeDataAsset::IsNodeDefinitionValid() const
{
	const bool bNoRelationshipRequirement = RelationshipRequirement.Metric == EPRProgressionRelationshipMetric::None
		&& RelationshipRequirement.Scope == EPRProgressionRelationshipScope::None
		&& RelationshipRequirement.MinimumValue == 0;
	const bool bRelationshipRequirementValid = bNoRelationshipRequirement ||
		(RelationshipRequirement.Metric != EPRProgressionRelationshipMetric::None
			&& RelationshipRequirement.Scope != EPRProgressionRelationshipScope::None
			&& RelationshipRequirement.MinimumValue >= 0 && RelationshipRequirement.MinimumValue <= 100);
	return NodeId.IsValid() && NodeId.PrimaryAssetType == PRProgressionNodePrivate::NodeType
		&& CounterproofCost >= 0 && MemoryFragmentCost >= 0 && MemoryFragmentCost == 0
		&& PRProgressionNodePrivate::IsSortedUniqueValid(PrerequisiteNodeIds) && bRelationshipRequirementValid
		&& !DisplayName.IsEmpty() && !Description.IsEmpty();
}

EDataValidationResult UPRProgressionNodeDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!IsNodeDefinitionValid())
	{
		Context.AddError(FText::FromString(TEXT("Progression node must use a fixed ProgressionNode id, canonical prerequisites, zero Memory cost, and bounded fixed metadata.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
