// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "PRInputConfigDataAsset.generated.h"

class FDataValidationContext;
class UInputAction;
class UInputMappingContext;

/** Stable association between a semantic input tag and its Enhanced Input action. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRTaggedInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Input")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Input")
	TObjectPtr<UInputAction> InputAction = nullptr;
};

/** Data-driven input contract consumed by the formal ProjectR player framework. */
UCLASS(BlueprintType)
class PROJECTR_API UPRInputConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Input", meta=(ClampMin="0"))
	int32 MappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Input")
	TArray<FPRTaggedInputAction> TaggedInputActions;

	/** Returns the configured action, or null with an explicit error for invalid or missing tags. */
	const UInputAction* FindInputActionForTag(FGameplayTag InputTag) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
