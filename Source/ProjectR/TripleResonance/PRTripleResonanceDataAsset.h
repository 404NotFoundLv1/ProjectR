// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "PRTripleResonanceDataAsset.generated.h"

class UGameplayEffect;
class UPRGA_TripleResonance;

/** Fixed v0.7.2 configuration only; runtime authority stays in UPRTripleResonanceSubsystem. */
UCLASS(BlueprintType)
class PROJECTR_API UPRTripleResonanceDataAsset final : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("TripleResonance"), TEXT("TripleResonance")); }
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|TripleResonance") TSoftClassPtr<UPRGA_TripleResonance> AbilityClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|TripleResonance") TSoftClassPtr<UGameplayEffect> ChargeEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|TripleResonance") TSoftClassPtr<UGameplayEffect> CostEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|TripleResonance") TSoftClassPtr<UGameplayEffect> CooldownEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|TripleResonance") float Damage = 120.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|TripleResonance") float ExecutionHealthFraction = 0.20f;
};
