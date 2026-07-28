// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "RealityHub/PRRealityHubTypes.h"

#include "PRRealityHubTerminalDataAsset.generated.h"

/** Configuration-only description of one fixed Reality Hub terminal. */
UCLASS(BlueprintType)
class PROJECTR_API UPRRealityHubTerminalDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	bool IsTerminalDefinitionValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|RealityHub") EPRRealityHubTerminal Terminal = EPRRealityHubTerminal::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|RealityHub") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|RealityHub") FText Description;
};
