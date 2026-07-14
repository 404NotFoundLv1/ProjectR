// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Core/PRCombatFeedbackInterface.h"
#include "Core/PRCombatMitigationInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"

#include "PRPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputComponent;
class UPRAbilitySystemComponent;
class UPRPlayerSkillComponent;
class APRPlayerState;
struct FPRDamageRequest;
struct FOnAttributeChangeData;
struct FInputActionValue;

/** Formal player character with a fixed side-view camera scaffold. */
UCLASS(Abstract)
class PROJECTR_API APRPlayerCharacter
	: public ACharacter
	, public IAbilitySystemInterface
	, public IPRCombatFeedbackInterface
	, public IPRCombatMitigationInterface
{
	GENERATED_BODY()

public:
	APRPlayerCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void HandleCombatHitReaction() override;
	virtual void HandleCombatLifeStateChanged(bool bIsDead) override;
	virtual EPRCombatMitigationResult EvaluateIncomingDamage(
		const FPRDamageRequest& Request,
		FGameplayTagContainer& OutResponseTags) const override;
	UPRPlayerSkillComponent* GetPlayerSkillComponent() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Restart() override;
	virtual void OnRep_PlayerState() override;
	virtual void UnPossessed() override;
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
	bool InitializeAbilitySystem();
	void CleanupAbilitySystem();
	void BindMoveSpeed(UPRAbilitySystemComponent* AbilitySystemComponent);
	void UnbindMoveSpeed();
	void HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData);
	void SyncMoveSpeed(float NewMoveSpeed);
	void RestoreMovementAfterHitReaction();
	bool ResolveRequiredInputActions(
		const UInputAction*& MoveAction,
		const UInputAction*& JumpAction,
		const UInputAction*& AttackAction,
		const UInputAction*& DodgeAction,
		const UInputAction*& InteractAction,
		const UInputAction*& ExecuteAction,
		const UInputAction*& ShadowThrustAction,
		const UInputAction*& FireSlashAction,
		const UInputAction*& ThunderDropAction,
		const UInputAction*& VectorHookAction,
		const UInputAction*& CounterProofWallAction) const;

	TWeakObjectPtr<UInputComponent> BoundInputComponent;
	FRotator InitialMeshRelativeRotation = FRotator::ZeroRotator;
	float LastNonZeroMoveDirection = 1.0f;
	FTimerHandle FacingTurnTimerHandle;
	double FacingTurnStartTimeSeconds = 0.0;
	float FacingTurnStartYaw = 0.0f;
	float FacingTurnTargetYaw = 0.0f;
	TWeakObjectPtr<UPRAbilitySystemComponent> BoundAbilitySystemComponent;
	TWeakObjectPtr<APRPlayerState> BoundPlayerState;
	FDelegateHandle MoveSpeedDelegateHandle;
	FTimerHandle HitReactionTimerHandle;
	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
	uint8 SavedCustomMovementMode = 0;
	bool bHitReactionActive = false;
	bool bCombatDeathLocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> SideViewCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ProjectR|Skill", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPRPlayerSkillComponent> PlayerSkillComponent;
};
