// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Dialogue/PRDialogueTypes.h"
#include "Engine/DataAsset.h"

#include "PRCompanionDialogueDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRCompanionDialogueDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool ValidateDefinition(FString& OutError) const;
	const FPRDialogueLineDefinition* FindLine(EPRDialogueTriggerKind TriggerKind) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") FGameplayTag CompanionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") FText SpeakerName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") TArray<FPRDialogueLineDefinition> Lines;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Dialogue") TArray<FPRDialogueChoiceDefinition> SafeChoices;
};
