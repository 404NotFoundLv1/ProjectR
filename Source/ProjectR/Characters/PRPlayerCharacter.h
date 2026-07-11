// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"

#include "PRPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputComponent;
struct FInputActionValue;

/** Formal player character with a fixed side-view camera scaffold. */
UCLASS(Abstract)
class PROJECTR_API APRPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APRPlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Stable native extension hook consumed by future tag-based ability routing. */
	virtual void HandleInputTagPressed(FGameplayTag InputTag);

	/** Stable native extension hook consumed by future tag-based ability routing. */
	virtual void HandleInputTagReleased(FGameplayTag InputTag);

private:
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleMoveCompleted(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpStopped();
	void HandleSemanticPressed(const FInputActionValue& Value, FGameplayTag InputTag);
	void HandleSemanticReleased(const FInputActionValue& Value, FGameplayTag InputTag);
	void UpdateFacing(float MoveAxis);
	void UpdateFacingTurn();
	bool ResolveRequiredInputActions(
		const UInputAction*& MoveAction,
		const UInputAction*& JumpAction,
		const UInputAction*& AttackAction,
		const UInputAction*& DodgeAction,
		const UInputAction*& InteractAction,
		const UInputAction*& ExecuteAction) const;

	TWeakObjectPtr<UInputComponent> BoundInputComponent;
	FRotator InitialMeshRelativeRotation = FRotator::ZeroRotator;
	float LastNonZeroMoveDirection = 1.0f;
	FTimerHandle FacingTurnTimerHandle;
	double FacingTurnStartTimeSeconds = 0.0;
	float FacingTurnStartYaw = 0.0f;
	float FacingTurnTargetYaw = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> SideViewCameraComponent;
};
