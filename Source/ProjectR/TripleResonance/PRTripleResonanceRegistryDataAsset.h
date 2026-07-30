// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PRTripleResonanceRegistryDataAsset.generated.h"

class UPRTripleResonanceDataAsset;
class UPRQTEDataAsset;

/** Closed asset registry: it contains the one ability definition and the three fixed ResultOnly QTE assets. */
UCLASS(BlueprintType)
class PROJECTR_API UPRTripleResonanceRegistryDataAsset final : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("TripleResonanceRegistry"), TEXT("TripleResonance")); }
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|TripleResonance") TSoftObjectPtr<UPRTripleResonanceDataAsset> Definition;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|TripleResonance") TArray<TSoftObjectPtr<UPRQTEDataAsset>> ExternalQTEs;
};
