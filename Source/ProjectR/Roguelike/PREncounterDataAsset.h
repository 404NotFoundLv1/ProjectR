// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Roguelike/PRRoomTypes.h"

#include "PREncounterDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPREncounterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsEncounterDefinitionValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId EncounterId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") EPRRoomEncounterKind Kind = EPRRoomEncounterKind::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPREncounterSpawnDefinition> SpawnDefinitions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FGameplayTag ExpectedBossId;
};
