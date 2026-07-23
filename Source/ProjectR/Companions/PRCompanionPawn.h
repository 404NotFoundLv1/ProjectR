// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"

#include "PRCompanionPawn.generated.h"

/** Non-combat visual/runtime pawn for the single selected Companion. */
UCLASS()
class PROJECTR_API APRCompanionPawn : public ACharacter
{
	GENERATED_BODY()

public:
	APRCompanionPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	void ConfigureRuntime(class UPRCompanionRuntimeDataAsset* InRuntimeData, FGameplayTag InCompanionId);
	FGameplayTag GetCompanionId() const;
	class UPRCompanionComponent* GetCompanionComponent() const;

private:
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Companion") TObjectPtr<class UPRCompanionComponent> CompanionComponent;
	FGameplayTag CompanionId;
};
