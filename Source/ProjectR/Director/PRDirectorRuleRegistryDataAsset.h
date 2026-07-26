// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorRuleDataAsset.h"
#include "Engine/DataAsset.h"

#include "PRDirectorRuleRegistryDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRDirectorRuleRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") TArray<TSoftObjectPtr<UPRDirectorRuleDataAsset>> Rules;
	bool IsRegistryReady() const;
	const UPRDirectorRuleDataAsset* FindRule(FGameplayTag RuleId) const;
};
