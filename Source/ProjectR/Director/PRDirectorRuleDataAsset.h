// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorTypes.h"
#include "Engine/DataAsset.h"

#include "PRDirectorRuleDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRDirectorRuleDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") FGameplayTag RuleId;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") int32 MaximumLevel = 3;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") FGameplayTagContainer AllowedReasonTags;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") TArray<FPRDirectorParameterDefinition> ParameterSchema;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") FText DefaultVisibleReason;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Director") FText CounterDescription;
	bool IsRuleDefinitionValid() const;
};
