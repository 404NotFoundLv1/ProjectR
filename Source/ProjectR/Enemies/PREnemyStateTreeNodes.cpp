// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyStateTreeNodes.h"

#include "AIController.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyCharacter.h"
#include "StateTreeExecutionContext.h"

namespace PREnemyStateTree
{
UPREnemyBrainComponent* GetBrain(FStateTreeExecutionContext& Context)
{
	if (const AAIController* Controller = Cast<AAIController>(Context.GetOwner()))
	{
		if (const APREnemyCharacter* Enemy = Cast<APREnemyCharacter>(Controller->GetPawn()))
		{
			return Enemy->GetEnemyBrain();
		}
	}
	return nullptr;
}
}

EStateTreeRunStatus FPRSTTaskAcquireTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	if (UPREnemyBrainComponent* Brain = PREnemyStateTree::GetBrain(Context))
	{
		Brain->ReevaluateTarget();
		return EStateTreeRunStatus::Running;
	}
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FPRSTTaskMoveToRangeBand::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	if (UPREnemyBrainComponent* Brain = PREnemyStateTree::GetBrain(Context))
	{
		Brain->UpdateRangeMovement();
		return EStateTreeRunStatus::Running;
	}
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FPRSTTaskActivateAttack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	if (UPREnemyBrainComponent* Brain = PREnemyStateTree::GetBrain(Context))
	{
		Brain->TryActivateCurrentAttack();
		return EStateTreeRunStatus::Running;
	}
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FPRSTTaskWaitForReevaluate::EnterState(FStateTreeExecutionContext&, const FStateTreeTransitionResult&) const
{
	return EStateTreeRunStatus::Running;
}

bool FPRSTConditionEnemyState::TestCondition(FStateTreeExecutionContext& Context) const
{
	const UPREnemyBrainComponent* Brain = PREnemyStateTree::GetBrain(Context);
	return Brain && Brain->GetRuntimeState().BrainState == ExpectedState;
}
