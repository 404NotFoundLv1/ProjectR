// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "PRCompanionDialogueRegistryDataAsset.generated.h"

class UPRCompanionDialogueDataAsset;
class UPRCompanionDialogueWidget;

UCLASS(BlueprintType)
class PROJECTR_API UPRCompanionDialogueRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool ValidateRegistry(FString& OutError) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") TSoftObjectPtr<UPRCompanionDialogueDataAsset> Axiom;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") TSoftObjectPtr<UPRCompanionDialogueDataAsset> Kindle;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") TSoftObjectPtr<UPRCompanionDialogueDataAsset> Null;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") TSoftClassPtr<UPRCompanionDialogueWidget> WidgetClass;
};
