// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemies/PREnemyTypes.h"

#include "PREnemyBrainComponent.generated.h"

class APREnemyCharacter;
class UPREnemyAttackDataAsset;

/** Event-driven state owner for a single initialized enemy. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPREnemyBrainComponent();

	void InitializeBrain(APREnemyCharacter* InEnemy);
	void StopBrain();
	void SetBrainState(EPREnemyBrainState InState);
	void SetAttackPhase(EPREnemyAttackPhase InPhase, FGameplayTag InAttackTag = FGameplayTag());
	const FPREnemyRuntimeState& GetRuntimeState() const;
	void ReevaluateTarget();
	void UpdateRangeMovement();
	bool TryActivateCurrentAttack();
	bool BeginAttack(const UPREnemyAttackDataAsset* Attack, AActor* Target);
	void CancelAttack();

private:
	TWeakObjectPtr<APREnemyCharacter> Enemy;
	FPREnemyRuntimeState RuntimeState;
	TWeakObjectPtr<AActor> CurrentTarget;
	FGuid AttackToken;
	FVector LockedAttackDirection = FVector::ZeroVector;
	double AttackCooldownEndTime = 0.0;
	FTimerHandle ReevaluateTimer;
	FTimerHandle AttackTimer;
	bool bRunning = false;

	void FinishWindup();
	void FinishActive();
	void FinishRecovery();
	void FinishCooldown();
	AActor* FindBestTarget() const;
	bool IsCurrentAttackTargetValid() const;
	const UPREnemyAttackDataAsset* FindAttackForTarget(AActor* Target) const;
};
