// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "QTE/PRQTETypes.h"

#include "PRQTEDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTR_API UPRQTEDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FName QTEId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag CompanionId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag QTEType;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE", meta=(MultiLine="true")) FText PromptText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTagContainer AcceptedInputTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE", meta=(ClampMin="0.01")) float WindowSeconds = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE", meta=(ClampMin="0.0", ClampMax="1.0")) float PerfectWindowFraction = 0.35f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE", meta=(ClampMin="0.0")) float BaseCooldownSeconds = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE", meta=(ClampMin="0", ClampMax="100")) int32 MinimumTrust = 40;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE", meta=(ClampMin="0", ClampMax="100")) int32 MaximumOverload = 90;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") int32 Priority = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") EPRQTETriggerSource TriggerSource = EPRQTETriggerSource::CombatEvent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") EPRQTETriggerKind TriggerKind = EPRQTETriggerKind::ShieldBroken;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag RequiredAbilityTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FGameplayTag RequiredResponseTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") EPRQTETargetScope TriggerTargetScope = EPRQTETargetScope::AnyEnemy;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE", meta=(ClampMin="0.0", ClampMax="1.0")) float TriggerHealthFraction = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE", meta=(ClampMin="0")) int32 RequiredDistinctTargetCount = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") bool bRequireFirstTargetShieldBreak = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") TArray<FPRQTEEffectDefinition> SuccessEffects;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|QTE") FPRQTEDeltaSet RelationshipDeltas;
};
