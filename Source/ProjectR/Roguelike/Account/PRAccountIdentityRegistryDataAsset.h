// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PRAccountIdentityRegistryDataAsset.generated.h"

class UPRAccountIdentityDataAsset;

UCLASS(BlueprintType)
class PROJECTR_API UPRAccountIdentityRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	bool IsRegistryReady() const;
	const UPRAccountIdentityDataAsset* FindIdentity(FPrimaryAssetId IdentityId) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Account")
	TArray<TSoftObjectPtr<UPRAccountIdentityDataAsset>> Identities;
};
