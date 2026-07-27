// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PRAccountIdentityDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRAccountIdentityDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	bool IsIdentityDefinitionValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Account") FPrimaryAssetId IdentityId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Account") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Account") FText Advantage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Account") FText Defect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Account") FText RecommendedPlaystyle;
};
