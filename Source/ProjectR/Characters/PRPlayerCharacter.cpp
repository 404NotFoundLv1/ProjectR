// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/PRPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"

APRPlayerCharacter::APRPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->SetupAttachment(GetCapsuleComponent());
	SideViewCameraComponent->SetRelativeLocation(FVector(0.0, 600.0, 100.0));
	SideViewCameraComponent->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
	SideViewCameraComponent->SetFieldOfView(60.0f);
	SideViewCameraComponent->bUsePawnControlRotation = false;
}
