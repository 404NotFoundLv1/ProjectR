// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core/PRRelationshipTypes.h"
#include "Engine/DataAsset.h"

#include "PRRoomEventDataAsset.generated.h"

USTRUCT(BlueprintType)
struct PROJECTR_API FPRRoomEventChoice
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FName ChoiceId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FText Description;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FPRRelationshipDelta RelationshipDelta;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") bool bRequiresQTESuccess = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") bool bBoostEpicWeight = false;
};

UCLASS(BlueprintType)
class PROJECTR_API UPRRoomEventDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool IsEventDefinitionValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FPrimaryAssetId EventId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") FText Description;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Roguelike") TArray<FPRRoomEventChoice> Choices;
};
