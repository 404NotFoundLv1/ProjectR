// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Roguelike/Progression/PRProgressionTypes.h"

#include "PRProgressionNodeDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRProgressionNodeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	bool IsNodeDefinitionValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") FPrimaryAssetId NodeId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") EPRProgressionTree Tree = EPRProgressionTree::Player;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") EPRProgressionEffectKind EffectKind = EPRProgressionEffectKind::EntitlementOnly;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") int32 CounterproofCost = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") int32 MemoryFragmentCost = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") TArray<FPrimaryAssetId> PrerequisiteNodeIds;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") FPRProgressionRelationshipRequirement RelationshipRequirement;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression") FText Description;
};
