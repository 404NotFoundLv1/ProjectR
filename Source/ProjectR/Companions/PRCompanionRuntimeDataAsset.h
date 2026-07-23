// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Companions/PRCompanionTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"

#include "PRCompanionRuntimeDataAsset.generated.h"

class APRCompanionPawn;

/** Fixed, data-only runtime configuration for one canonical Companion identity. */
UCLASS(BlueprintType)
class PROJECTR_API UPRCompanionRuntimeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion") FGameplayTag CompanionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion") TSoftClassPtr<APRCompanionPawn> PawnClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion") EPRCompanionSupportType SupportType = EPRCompanionSupportType::Shield;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion", meta=(ClampMin="0.01")) float BaseSupportInterval = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion", meta=(ClampMin="0.0")) float SupportRange = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion", meta=(ClampMin="0.0")) float SupportMagnitude = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Companion") TSubclassOf<UGameplayEffect> SupportEffectClass;
};
