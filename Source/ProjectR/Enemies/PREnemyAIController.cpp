// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyAIController.h"

#include "Core/PRTagLibrary.h"
#include "StateTree.h"
#include "Components/StateTreeAIComponent.h"

APREnemyAIController::APREnemyAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	EnemyStateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("EnemyStateTreeComponent"));
	EnemyStateTreeComponent->SetStartLogicAutomatically(false);
}

bool APREnemyAIController::ConfigureAndStartStateTree(UStateTree* InStateTree)
{
	if (!EnemyStateTreeComponent || !InStateTree || !GetPawn())
	{
		return false;
	}
	if (EnemyStateTreeComponent->IsRunning() && PendingStateTree == InStateTree)
	{
		return true;
	}
	if (EnemyStateTreeComponent->IsRunning())
	{
		EnemyStateTreeComponent->StopLogic(TEXT("Replacing Enemy StateTree"));
	}
	PendingStateTree = InStateTree;
	EnemyStateTreeComponent->SetStateTree(PendingStateTree);
	EnemyStateTreeComponent->StartLogic();
	return EnemyStateTreeComponent->IsRunning();
}

bool APREnemyAIController::IsEnemyStateTreeRunning() const
{
	return EnemyStateTreeComponent && EnemyStateTreeComponent->IsRunning();
}

void APREnemyAIController::StopEnemyStateTree()
{
	if (EnemyStateTreeComponent)
	{
		EnemyStateTreeComponent->StopLogic(TEXT("Enemy lifecycle stopped"));
	}
}

void APREnemyAIController::SendEnemyReevaluateEvent()
{
	if (EnemyStateTreeComponent)
	{
		EnemyStateTreeComponent->SendStateTreeEvent(FStateTreeEvent(UPRTagLibrary::GetEnemyAIEventReevaluateTag()));
	}
}

void APREnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (PendingStateTree)
	{
		ConfigureAndStartStateTree(PendingStateTree);
	}
}

void APREnemyAIController::OnUnPossess()
{
	StopEnemyStateTree();
	Super::OnUnPossess();
}
