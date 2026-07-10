// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "PRMapId.generated.h"

/** Stable identifiers for the formal ProjectR maps introduced in v0.0.3. */
UENUM(BlueprintType)
enum class EPRMapId : uint8
{
	MainMenu,
	RealityHub,
	NetworkPrototype,
	CombatGym,
	BossGym
};
