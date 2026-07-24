// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Divergence/PRDivergenceTypes.h"
#include "Engine/DataAsset.h"

#include "PRDivergenceDataAsset.generated.h"

class UPRDivergenceCacheWidget;

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDivergenceChoiceDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence") EPRDivergenceChoice Choice = EPRDivergenceChoice::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence") FText DisplayText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence") FPRRelationshipDelta RelationshipDelta;
};

USTRUCT(BlueprintType)
struct PROJECTR_API FPRDivergencePresentationDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence") FGameplayTag CompanionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence") FText SpeakerText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence", meta=(MultiLine="true")) FText PromptText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence") TArray<FPRDivergenceChoiceDefinition> Choices;
};

/** Fixed, data-only v0.3.4 near-death choice definition. */
UCLASS(BlueprintType)
class PROJECTR_API UPRDivergenceDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	bool ValidateDefinition(FString& OutError) const;
	const FPRDivergencePresentationDefinition* FindPresentation(FGameplayTag CompanionId) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence", meta=(ClampMin="0", ClampMax="100")) int32 MinimumTrust = FPRDivergenceContract::MinimumTrust;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence", meta=(ClampMin="0", ClampMax="101")) int32 MaximumOverloadExclusive = FPRDivergenceContract::MaximumOverloadExclusive;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence", meta=(ClampMin="0.01")) float ChoiceWindowSeconds = FPRDivergenceContract::ChoiceWindowSeconds;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence", meta=(ClampMin="0.01", ClampMax="1.0")) float RescueHealthFraction = FPRDivergenceContract::RescueHealthFraction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence", meta=(ClampMin="0.01", ClampMax="1.0")) float ChallengeHealthFraction = FPRDivergenceContract::ChallengeHealthFraction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence", meta=(ClampMin="0.0", ClampMax="1.0")) float ReviveShieldFraction = FPRDivergenceContract::ReviveShieldFraction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence") TSoftClassPtr<UPRDivergenceCacheWidget> WidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Divergence") TArray<FPRDivergencePresentationDefinition> Presentations;
};
