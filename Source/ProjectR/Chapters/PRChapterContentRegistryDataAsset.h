// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Chapters/PRChapterTypes.h"
#include "Engine/DataAsset.h"

#include "PRChapterContentRegistryDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRChapterContentRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsDefinitionValid() const;

	static FPrimaryAssetId GetAllocatorChapterId();
	static FName GetAllocatorContentId();
	static FName GetAllocatorBossId();
	static FName GetAllocatorProofId();
	static const TArray<FName>& GetAllocatorDirectiveIds();
	static FName GetDirectiveForSeed(int32 Seed);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId ChapterId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FName ContentId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId RoomContentRegistryId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId EnemyContentRegistryId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FName BossId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FName ProofId;
};
