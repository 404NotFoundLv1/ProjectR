// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemies/PREnemyTypes.h"

#include "PREnemyBrainComponent.generated.h"

class APREnemyCharacter;
class UPREnemyAttackDataAsset;
class APREnemyProjectile;

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
	bool CanBeginAttack(const UPREnemyAttackDataAsset* Attack, AActor* Target) const;
	bool BeginAttack(const UPREnemyAttackDataAsset* Attack, AActor* Target);
	void CancelAttack();

private:
	friend class APREnemyProjectile;

	TWeakObjectPtr<APREnemyCharacter> Enemy;
	FPREnemyRuntimeState RuntimeState;
	TWeakObjectPtr<AActor> CurrentTarget;
	TWeakObjectPtr<const UPREnemyAttackDataAsset> ActiveAttack;
	FGuid AttackToken;
	TSet<FGuid> ActiveProjectileTokens;
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
	bool HasLineOfSightTo(AActor* Target) const;
	const UPREnemyAttackDataAsset* FindConfiguredAttack() const;
	const UPREnemyAttackDataAsset* FindAttackForTarget(AActor* Target) const;
	bool SpawnProjectile(const UPREnemyAttackDataAsset* Attack, AActor* Target);
	bool TryResolveProjectileToken(const FGuid& Token);
	void ReleaseProjectileToken(const FGuid& Token);
};
