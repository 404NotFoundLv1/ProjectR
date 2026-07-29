// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PREnemyContentRegistryDataAsset.generated.h"

class APREnemyCharacter;
class UPREnemyPrototypeDataAsset;

USTRUCT(BlueprintType)
struct PROJECTR_API FPREnemyContentRegistryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy") FPrimaryAssetId PrototypeId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy") TSoftObjectPtr<UPREnemyPrototypeDataAsset> Prototype;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy") TSoftClassPtr<APREnemyCharacter> EnemyClass;
};

/** Closed PrimaryAssetId whitelist for one chapter's enemy content. */
UCLASS(BlueprintType)
class PROJECTR_API UPREnemyContentRegistryDataAsset final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsRegistryReady() const;
	const FPREnemyContentRegistryEntry* FindEntry(FPrimaryAssetId PrototypeId) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Enemy") TArray<FPREnemyContentRegistryEntry> Entries;
};
