// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Blueprint/UserWidget.h"

#include "PRQTERegistryDataAsset.generated.h"

class UPRQTEDataAsset;

UCLASS(BlueprintType)
class PROJECTR_API UPRQTERegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	const TArray<TSoftObjectPtr<UPRQTEDataAsset>>& GetEntries() const { return Entries; }

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") TArray<TSoftObjectPtr<UPRQTEDataAsset>> Entries;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") TSoftClassPtr<UUserWidget> PromptWidgetClass;
};
