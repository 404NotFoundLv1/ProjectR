// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "PRMemoryPersonaDataAsset.generated.h"

USTRUCT(BlueprintType)
struct PROJECTR_API FPRMemoryPlayerOptionDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Memory") FName OptionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Memory") FText DisplayText;
};

UCLASS(BlueprintType)
class PROJECTR_API UPRMemoryPersonaDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsDefinitionValid() const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Memory") FGameplayTag CompanionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Memory") FName ProviderCompanionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Memory") TArray<FName> EmotionIds;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Memory") TArray<FText> SummaryTemplates;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Memory") TArray<FPRMemoryPlayerOptionDefinition> PlayerOptions;
};
