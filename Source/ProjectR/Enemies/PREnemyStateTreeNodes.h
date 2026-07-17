// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Enemies/PREnemyTypes.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"

#include "PREnemyStateTreeNodes.generated.h"

/**
 * StateTree requires every authored task to expose a reflected instance-data
 * struct, even when the task has no per-instance fields.  Keeping this empty
 * preserves the event-driven Brain ownership model while making the four
 * fixed ST_Enemy_Base tasks valid editor nodes.
 */
USTRUCT()
struct PROJECTR_API FPRSTTaskEmptyInstanceData
{
	GENERATED_BODY()
};

/** Fixed task vocabulary used only by ST_Enemy_Base. */
USTRUCT(meta=(DisplayName="PR Acquire Target"))
struct PROJECTR_API FPRSTTaskAcquireTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FPRSTTaskEmptyInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT(meta=(DisplayName="PR Move To Range Band"))
struct PROJECTR_API FPRSTTaskMoveToRangeBand : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FPRSTTaskEmptyInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT(meta=(DisplayName="PR Activate Attack"))
struct PROJECTR_API FPRSTTaskActivateAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FPRSTTaskEmptyInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT(meta=(DisplayName="PR Wait Reevaluate"))
struct PROJECTR_API FPRSTTaskWaitForReevaluate : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FPRSTTaskEmptyInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT(meta=(DisplayName="PR Enemy State Is"))
struct PROJECTR_API FPRSTConditionEnemyState : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category="Input")
	EPREnemyBrainState ExpectedState = EPREnemyBrainState::Dormant;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
