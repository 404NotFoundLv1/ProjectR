// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PRInputTypes.generated.h"

/** Value-only semantic input fact emitted before the existing ASC routing path. */
USTRUCT(BlueprintType)
struct PROJECTR_API FPRSemanticInputEvent
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Input") FGameplayTag InputTag;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Input") bool bPressed = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category="ProjectR|Input") double WorldTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRSemanticInputEventNative, const FPRSemanticInputEvent&);
