// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "PRCombatMitigationInterface.generated.h"

struct FPRDamageRequest;
enum class EPRCombatMitigationResult : uint8;

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UPRCombatMitigationInterface : public UInterface
{
	GENERATED_BODY()
};

/** Pure native mitigation decision; Combat remains the only attribute/event owner. */
class PROJECTR_API IPRCombatMitigationInterface
{
	GENERATED_BODY()

public:
	virtual EPRCombatMitigationResult EvaluateIncomingDamage(
		const FPRDamageRequest& Request,
		FGameplayTagContainer& OutResponseTags) const = 0;
};
