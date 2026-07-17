// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIController.h"

#include "PREnemyAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;

/** Minimal, event-driven controller for the fixed enemy StateTree schema. */
UCLASS()
class PROJECTR_API APREnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	APREnemyAIController();
	/** Starts only a compiled StateTree on the currently possessed enemy. */
	bool ConfigureAndStartStateTree(UStateTree* InStateTree);
	void StopEnemyStateTree();
	void SendEnemyReevaluateEvent();
	bool IsEnemyStateTreeRunning() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Enemy")
	TObjectPtr<UStateTreeAIComponent> EnemyStateTreeComponent;
	TObjectPtr<UStateTree> PendingStateTree;
};
