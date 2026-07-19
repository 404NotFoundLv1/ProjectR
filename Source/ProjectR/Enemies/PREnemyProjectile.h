// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayTagContainer.h"

#include "PREnemyProjectile.generated.h"

class APREnemyCharacter;
class UPREnemyAttackDataAsset;
class USphereComponent;

/** Internal movement bridge that exposes each swept projectile segment to its owner. */
UCLASS(NotBlueprintable)
class PROJECTR_API UPREnemyProjectileMovementComponent final : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.0f, const FVector& MoveDelta = FVector::ZeroVector) override;
};

/** Authority-only ranged attack projectile. It never owns combat rules or player-specific dependencies. */
UCLASS(Abstract, Blueprintable)
class PROJECTR_API APREnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	APREnemyProjectile();

	/** Copies the immutable ranged-shot snapshot after the Brain has validated and registered its attack token. */
	bool InitializeProjectile(
		APREnemyCharacter* SourceEnemy,
		AActor* TargetAvatar,
		const UPREnemyAttackDataAsset* AttackData,
		const FGuid& InAttackToken,
		const FVector& InLockedDirection,
		float InPlaneY);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class UPREnemyProjectileMovementComponent;

	UFUNCTION()
	void HandleWatchedActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);
	void HandleLifetimeExpired();
	void ValidateRuntime();
	bool HandleMovementSegment(const FVector& SegmentStart, const FVector& SegmentEnd, const FHitResult* Impact);
	bool TryConsumeNearestAttackProxy(const FVector& SegmentStart, const FVector& SegmentEnd);
	bool ResolveTargetHit(const FHitResult& Impact);
	bool IsRuntimeValid() const;
	bool TryResolveAttackToken();
	void ReleaseAttackToken();
	void FinishProjectile(bool bConsumeToken);

	UPROPERTY(VisibleAnywhere, Category="ProjectR|Enemy")
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Enemy")
	TObjectPtr<UPREnemyProjectileMovementComponent> ProjectileMovement;

	TWeakObjectPtr<APREnemyCharacter> SourceEnemy;
	TWeakObjectPtr<AActor> TargetAvatar;
	FGuid AttackToken;
	FName AttackId;
	FGameplayTag AbilityTag;
	FGameplayTagContainer DamageTags;
	FVector LockedDirection = FVector::ZeroVector;
	float PlaneY = 0.0f;
	float RawDamage = 0.0f;
	FTimerHandle LifetimeTimer;
	FTimerHandle ValidationTimer;
	bool bInitialized = false;
	bool bResolved = false;
};
