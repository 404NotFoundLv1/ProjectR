// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "PRCompanionRegistryDataAsset.generated.h"

class UPRCompanionDataAsset;

UCLASS(BlueprintType)
class PROJECTR_API UPRCompanionRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	const TArray<TSoftObjectPtr<UPRCompanionDataAsset>>& GetCompanions() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion")
	TArray<TSoftObjectPtr<UPRCompanionDataAsset>> Companions;
};

