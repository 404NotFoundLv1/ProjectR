// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "PRPlayerSkillFragment.generated.h"

/** Inline validated extension point for current-version skill-specific tuning only. */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRPlayerSkillFragment : public UObject
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRFireSlashSkillFragment final : public UPRPlayerSkillFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Burning", meta=(ClampMin="0"))
	float BurningBaseDamage = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Burning", meta=(ClampMin="0"))
	float BurningAttackPowerScale = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Burning", meta=(ClampMin="0"))
	float BurningTickInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Burning", meta=(ClampMin="1"))
	int32 BurningTickCount = 3;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRThunderDropSkillFragment final : public UPRPlayerSkillFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Targeting", meta=(ClampMin="0"))
	float FallbackDistance = 450.0f;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRAfterimageDodgeSkillFragment final : public UPRPlayerSkillFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Timing", meta=(ClampMin="0"))
	float PerfectTimingWindow = 0.14f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Timing", meta=(ClampMin="0"))
	float DecoyLifetime = 1.5f;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRVectorHookSkillFragment final : public UPRPlayerSkillFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Movement", meta=(ClampMin="0"))
	float StopDistance = 120.0f;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRCounterProofWallSkillFragment final : public UPRPlayerSkillFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ProjectR|Skill|Timing", meta=(ClampMin="0"))
	float PerfectTimingWindow = 0.15f;
};
