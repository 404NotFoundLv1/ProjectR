// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "PRAllocatorTypes.generated.h"

/** Fixed, presentation-safe identifiers for the three Allocator phase effects. */
UENUM(BlueprintType)
enum class EPRAllocatorCounterResult : uint8
{
	None,
	ResourceLockCountered,
	RewardDeprivationCountered,
	PriceAuditBroken,
	DegradedNoOp
};
