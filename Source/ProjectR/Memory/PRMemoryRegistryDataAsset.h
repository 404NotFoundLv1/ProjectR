// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PRMemoryRegistryDataAsset.generated.h"

class UPRMemoryPersonaDataAsset;

UCLASS(BlueprintType)
class PROJECTR_API UPRMemoryRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsRegistryReady() const;
	const UPRMemoryPersonaDataAsset* FindPersona(FName ProviderCompanionId) const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Memory") TArray<TSoftObjectPtr<UPRMemoryPersonaDataAsset>> Personas;
};
