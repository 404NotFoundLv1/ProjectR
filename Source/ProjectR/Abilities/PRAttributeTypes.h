// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"

/** Immutable payload published when a ProjectR gameplay attribute changes. */
struct PROJECTR_API FPRAttributeChange
{
	FGameplayAttribute Attribute;
	float OldValue = 0.0f;
	float NewValue = 0.0f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPRAttributeChangedNative, const FPRAttributeChange&);
