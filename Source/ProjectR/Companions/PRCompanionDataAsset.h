// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "PRCompanionDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRCompanionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion")
	FGameplayTag CompanionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion")
	FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion")
	FText RoleLabel;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion")
	FText PersonaSummary;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion")
	FLinearColor AccentColor = FLinearColor::White;
};

