// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PRProgressionRegistryDataAsset.generated.h"

class UPRProgressionNodeDataAsset;

UCLASS(BlueprintType)
class PROJECTR_API UPRProgressionRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	bool IsRegistryReady() const;
	const UPRProgressionNodeDataAsset* FindNode(FPrimaryAssetId NodeId) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Progression")
	TArray<TSoftObjectPtr<UPRProgressionNodeDataAsset>> Nodes;
};
