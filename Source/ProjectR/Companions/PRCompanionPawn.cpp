// Copyright Epic Games, Inc. All Rights Reserved.

#include "Companions/PRCompanionPawn.h"

#include "Companions/PRCompanionComponent.h"
#include "Companions/PRCompanionRuntimeDataAsset.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

APRCompanionPawn::APRCompanionPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	CompanionComponent = CreateDefaultSubobject<UPRCompanionComponent>(TEXT("CompanionComponent"));
	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
	GetCharacterMovement()->DefaultWaterMovementMode = MOVE_Flying;
	GetCharacterMovement()->MaxFlySpeed = 550.0f;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->SetPlaneConstraintEnabled(true);
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector::YAxisVector);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
}

void APRCompanionPawn::ConfigureRuntime(UPRCompanionRuntimeDataAsset* InRuntimeData, const FGameplayTag InCompanionId)
{
	CompanionId = InCompanionId;
	if (CompanionComponent)
	{
		CompanionComponent->InitializeCompanion(InRuntimeData, InCompanionId);
	}
}

FGameplayTag APRCompanionPawn::GetCompanionId() const
{
	return CompanionId;
}

UPRCompanionComponent* APRCompanionPawn::GetCompanionComponent() const
{
	return CompanionComponent;
}
