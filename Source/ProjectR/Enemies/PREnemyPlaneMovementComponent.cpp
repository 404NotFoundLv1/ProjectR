// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyPlaneMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UPREnemyPlaneMovementComponent::UPREnemyPlaneMovementComponent()
{
	bConstrainToPlane = true;
	SetPlaneConstraintNormal(FVector::YAxisVector);
	bSnapToPlaneAtStart = true;
}

void UPREnemyPlaneMovementComponent::SetPlaneY(const float InPlaneY)
{
	PlaneY = InPlaneY;
	SetPlaneConstraintOrigin(FVector(0.0f, PlaneY, 0.0f));
	SnapUpdatedComponentToPlane();
}

void UPREnemyPlaneMovementComponent::SetDesiredDirectionX(const float InDirectionX)
{
	DesiredDirectionX = FMath::Clamp(InDirectionX, -1.0f, 1.0f);
}

void UPREnemyPlaneMovementComponent::StopEnemyMovement()
{
	DesiredDirectionX = 0.0f;
	StopMovementImmediately();
}

bool UPREnemyPlaneMovementComponent::IsPlatformSafeInDirection(
	const float DirectionX,
	const float ProbeForward,
	const float ProbeDepth) const
{
	const ACharacter* Character = CharacterOwner;
	const UWorld* World = GetWorld();
	if (!Character || !World || FMath::IsNearlyZero(DirectionX) || ProbeForward <= 0.0f || ProbeDepth <= 0.0f)
	{
		return false;
	}
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.0f;
	const FVector Origin = Character->GetActorLocation() + FVector(DirectionX * ProbeForward, 0.0f, -HalfHeight + 2.0f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PREnemyEdgeProbe), false, Character);
	return World->LineTraceSingleByChannel(Hit, Origin, Origin - FVector(0.0f, 0.0f, ProbeDepth), ECC_WorldStatic, Params);
}

void UPREnemyPlaneMovementComponent::CalcVelocity(
	const float DeltaTime,
	const float Friction,
	const bool bFluid,
	const float BrakingDeceleration)
{
	if (!FMath::IsNearlyZero(DesiredDirectionX) && CharacterOwner)
	{
		Acceleration = FVector(DesiredDirectionX * GetMaxAcceleration(), 0.0f, 0.0f);
	}
	else
	{
		Acceleration = FVector::ZeroVector;
	}
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	Velocity.Y = 0.0f;
}
