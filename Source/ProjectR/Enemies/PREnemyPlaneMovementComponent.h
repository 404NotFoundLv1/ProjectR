// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "PREnemyPlaneMovementComponent.generated.h"

/** CharacterMovement-backed X/Z plane motion; its engine tick is the only enemy movement tick. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPREnemyPlaneMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPREnemyPlaneMovementComponent();

	void SetPlaneY(float InPlaneY);
	void SetDesiredDirectionX(float InDirectionX);
	void StopEnemyMovement();
	bool IsPlatformSafeInDirection(float DirectionX, float ProbeForward, float ProbeDepth) const;

protected:
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

private:
	float PlaneY = 0.0f;
	float DesiredDirectionX = 0.0f;
};
