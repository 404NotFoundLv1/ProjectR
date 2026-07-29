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
	static FPrimaryAssetId GetAllocatorBossPrototypeId();
	static FName GetAllocatorProofId();
	static const TArray<FName>& GetAllocatorDirectiveIds();
	static FName GetDirectiveForSeed(int32 Seed);
	static FPrimaryAssetId GetWardenChapterId();
	static FName GetWardenContentId();
	static FName GetWardenBossId();
	static FPrimaryAssetId GetWardenBossPrototypeId();
	static FName GetWardenProofId();
	static FPrimaryAssetId GetWardenRoomRegistryId();
	static FPrimaryAssetId GetWardenEnemyRegistryId();
	static FPrimaryAssetId GetWardenFinalRoomId();
	static const TArray<FName>& GetWardenDirectiveIds();
	static FPrimaryAssetId GetPacifierChapterId();
	static FName GetPacifierContentId();
	static FName GetPacifierBossId();
	static FPrimaryAssetId GetPacifierBossPrototypeId();
	static FName GetPacifierProofId();
	static FPrimaryAssetId GetPacifierRoomRegistryId();
	static FPrimaryAssetId GetPacifierEnemyRegistryId();
	static FPrimaryAssetId GetPacifierFinalRoomId();
	static const TArray<FName>& GetPacifierDirectiveIds();
	/** Resolves only the closed Allocator/Warden/Pacifier seed mapping; unknown content is rejected. */
	static FName GetDirectiveForContentAndSeed(FName ContentId, int32 Seed);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId ChapterId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FName ContentId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId RoomContentRegistryId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FPrimaryAssetId EnemyContentRegistryId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FName BossId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Chapter") FName ProofId;
};
