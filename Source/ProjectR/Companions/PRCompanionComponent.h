// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Companions/PRCompanionTypes.h"
#include "Components/ActorComponent.h"

#include "PRCompanionComponent.generated.h"

class APREnemyCharacter;
class APRCompanionPawn;
class UPRCompanionRuntimeDataAsset;

/** Timer-driven follow and deterministic support for the active non-combat Companion Pawn. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRCompanionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRCompanionComponent();
	static float CalculateHealthFloor(FGameplayTag PrototypeTag, float MaxHealth);
	static float ClampNoKillDamage(float RequestedDamage, float CurrentShield, float CurrentHealth, float HealthFloor);
	void InitializeCompanion(UPRCompanionRuntimeDataAsset* InRuntimeData, FGameplayTag InCompanionId);
	void ShutdownCompanion();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TickFollow();
	void ScheduleNextFollow();
	void ScheduleNextSupport();
	void ExecuteSupport();
	APREnemyCharacter* FindSupportTarget() const;
	bool IsSupportTargetEligible(const APREnemyCharacter& Target) const;
	float GetEffectiveInterval(float& OutOverload) const;
	bool ResolveBossHealthFloor(APREnemyCharacter& Target, float& OutHealthFloor) const;
	void PublishSupportEvent(FPRCompanionSupportEvent&& Event) const;

	UPROPERTY(Transient) TObjectPtr<UPRCompanionRuntimeDataAsset> RuntimeData;
	FGameplayTag CompanionId;
	FVector LastFollowDirection = FVector(-1.0f, 0.0f, 0.0f);
	FTimerHandle FollowTimer;
	FTimerHandle SupportTimer;
	bool bRuntimeInitialized = false;
};
