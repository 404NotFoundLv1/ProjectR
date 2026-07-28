// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "PRRealityHubTerminalRegistryDataAsset.generated.h"

class UPRDirectorRuleRegistryDataAsset;
class UPRRealityHubTerminalDataAsset;
enum class EPRRealityHubTerminal : uint8;

/** The sole whitelist for Hub terminal configuration and local forecast candidates. */
UCLASS(BlueprintType)
class PROJECTR_API UPRRealityHubTerminalRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	bool IsRegistryReady() const;
	const UPRRealityHubTerminalDataAsset* FindTerminal(EPRRealityHubTerminal Terminal) const;
	void GetForecastRuleIds(TArray<FGameplayTag>& OutRuleIds) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|RealityHub") TArray<TSoftObjectPtr<UPRRealityHubTerminalDataAsset>> Terminals;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|RealityHub") TSoftObjectPtr<UPRDirectorRuleRegistryDataAsset> DirectorRuleRegistry;
};
