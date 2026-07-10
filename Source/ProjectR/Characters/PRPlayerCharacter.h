// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"

#include "PRPlayerCharacter.generated.h"

class UCameraComponent;

/** Formal player character with a fixed side-view camera scaffold. */
UCLASS(Abstract)
class PROJECTR_API APRPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APRPlayerCharacter();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> SideViewCameraComponent;
};
