// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** One-way formatting helpers for identifiers and values written to logs. */
struct PROJECTR_API FPRLogSanitizer
{
	static FString ToOpaqueGuidToken(const FGuid& Value);
	static FString RedactedValue();
};
