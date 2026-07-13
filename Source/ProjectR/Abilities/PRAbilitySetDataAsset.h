// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/PRAbilityTypes.h"
#include "Engine/DataAsset.h"

#include "PRAbilitySetDataAsset.generated.h"

/** Serializable set of abilities; runtime grant state remains owned by the ASC. */
UCLASS(BlueprintType)
class PROJECTR_API UPRAbilitySetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	const TArray<FPRAbilitySetEntry>& GetAbilityEntries() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Abilities")
	TArray<FPRAbilitySetEntry> AbilityEntries;
};
