// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/PRPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/PRPlayerController.h"
#include "Core/PRTagLibrary.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/PRInputConfigDataAsset.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "ProjectR.h"

namespace
{
constexpr float FacingTurnDurationSeconds = 0.12f;
constexpr float FacingTurnTimerIntervalSeconds = 1.0f / 120.0f;
constexpr float FacingTurnEaseExponent = 2.0f;
}

APRPlayerCharacter::APRPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->SetupAttachment(GetCapsuleComponent());
	SideViewCameraComponent->SetRelativeLocation(FVector(0.0, 600.0, 100.0));
	SideViewCameraComponent->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
	SideViewCameraComponent->SetFieldOfView(60.0f);
	SideViewCameraComponent->bUsePawnControlRotation = false;
}

void APRPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitialMeshRelativeRotation = GetMesh()->GetRelativeRotation();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float SpawnY = GetActorLocation().Y;
	Movement->bSnapToPlaneAtStart = false;
	Movement->SetPlaneConstraintOrigin(FVector(0.0, SpawnY, 0.0));
	Movement->SetPlaneConstraintNormal(FVector::YAxisVector);
	Movement->SetPlaneConstraintEnabled(true);
	Movement->SnapUpdatedComponentToPlane();
}

void APRPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FacingTurnTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void APRPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (BoundInputComponent.Get() == PlayerInputComponent)
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!IsValid(EnhancedInputComponent))
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerCharacter %s requires UEnhancedInputComponent."), *GetPathName());
		return;
	}

	const UInputAction* MoveAction = nullptr;
	const UInputAction* JumpAction = nullptr;
	const UInputAction* AttackAction = nullptr;
	const UInputAction* DodgeAction = nullptr;
	const UInputAction* InteractAction = nullptr;
	const UInputAction* ExecuteAction = nullptr;
	if (!ResolveRequiredInputActions(
		MoveAction,
		JumpAction,
		AttackAction,
		DodgeAction,
		InteractAction,
		ExecuteAction))
	{
		return;
	}

	EnhancedInputComponent->BindAction(
		MoveAction, ETriggerEvent::Triggered, this, &APRPlayerCharacter::HandleMoveInput);
	EnhancedInputComponent->BindAction(
		MoveAction, ETriggerEvent::Completed, this, &APRPlayerCharacter::HandleMoveCompleted);
	EnhancedInputComponent->BindAction(
		JumpAction, ETriggerEvent::Started, this, &APRPlayerCharacter::HandleJumpStarted);
	EnhancedInputComponent->BindAction(
		JumpAction, ETriggerEvent::Completed, this, &APRPlayerCharacter::HandleJumpStopped);
	EnhancedInputComponent->BindAction(
		JumpAction, ETriggerEvent::Canceled, this, &APRPlayerCharacter::HandleJumpStopped);

	const TPair<const UInputAction*, FGameplayTag> SemanticActions[] = {
		{AttackAction, UPRTagLibrary::GetInputAttackTag()},
		{DodgeAction, UPRTagLibrary::GetInputDodgeTag()},
		{InteractAction, UPRTagLibrary::GetInputInteractTag()},
		{ExecuteAction, UPRTagLibrary::GetInputExecuteTag()}};
	for (const TPair<const UInputAction*, FGameplayTag>& Entry : SemanticActions)
	{
		EnhancedInputComponent->BindAction(
			Entry.Key,
			ETriggerEvent::Started,
			this,
			&APRPlayerCharacter::HandleSemanticPressed,
			Entry.Value);
		EnhancedInputComponent->BindAction(
			Entry.Key,
			ETriggerEvent::Completed,
			this,
			&APRPlayerCharacter::HandleSemanticReleased,
			Entry.Value);
		EnhancedInputComponent->BindAction(
			Entry.Key,
			ETriggerEvent::Canceled,
			this,
			&APRPlayerCharacter::HandleSemanticReleased,
			Entry.Value);
	}

	BoundInputComponent = PlayerInputComponent;
}

void APRPlayerCharacter::HandleInputTagPressed(FGameplayTag InputTag)
{
	UE_LOG(LogProjectR, Display, TEXT("ProjectR semantic input Pressed: %s"), *InputTag.ToString());
}

void APRPlayerCharacter::HandleInputTagReleased(FGameplayTag InputTag)
{
	UE_LOG(LogProjectR, Display, TEXT("ProjectR semantic input Released: %s"), *InputTag.ToString());
}

void APRPlayerCharacter::HandleMoveInput(const FInputActionValue& Value)
{
	const float MoveAxis = Value.Get<float>();
	if (FMath::IsNearlyZero(MoveAxis))
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement->IsFalling()
		&& !FMath::IsNearlyZero(Movement->Velocity.X)
		&& Movement->Velocity.X * MoveAxis < 0.0f)
	{
		Movement->Velocity.X = FMath::Abs(Movement->Velocity.X) * FMath::Sign(MoveAxis);
	}

	const float MoveDirection = FMath::Sign(MoveAxis);
	if (!FMath::IsNearlyEqual(MoveDirection, LastNonZeroMoveDirection))
	{
		LastNonZeroMoveDirection = MoveDirection;
		UpdateFacing(LastNonZeroMoveDirection);
	}
	AddMovementInput(FVector::XAxisVector, MoveAxis);
}

void APRPlayerCharacter::HandleMoveCompleted(const FInputActionValue& Value)
{
	// Enhanced Input owns the transient axis state; retain LastNonZeroMoveDirection for facing.
}

void APRPlayerCharacter::HandleJumpStarted()
{
	Jump();
}

void APRPlayerCharacter::HandleJumpStopped()
{
	StopJumping();
}

void APRPlayerCharacter::HandleSemanticPressed(const FInputActionValue& Value, FGameplayTag InputTag)
{
	HandleInputTagPressed(InputTag);
}

void APRPlayerCharacter::HandleSemanticReleased(const FInputActionValue& Value, FGameplayTag InputTag)
{
	HandleInputTagReleased(InputTag);
}

void APRPlayerCharacter::UpdateFacing(float MoveAxis)
{
	if (FMath::IsNearlyZero(MoveAxis))
	{
		return;
	}

	const float TargetYaw = MoveAxis > 0.0f
		? InitialMeshRelativeRotation.Yaw
		: FRotator::NormalizeAxis(InitialMeshRelativeRotation.Yaw + 180.0f);
	FacingTurnStartYaw = GetMesh()->GetRelativeRotation().Yaw;
	FacingTurnTargetYaw = FacingTurnStartYaw
		+ FMath::FindDeltaAngleDegrees(FacingTurnStartYaw, TargetYaw);
	FacingTurnStartTimeSeconds = GetWorld()->GetTimeSeconds();

	GetWorldTimerManager().ClearTimer(FacingTurnTimerHandle);
	GetWorldTimerManager().SetTimer(
		FacingTurnTimerHandle,
		this,
		&APRPlayerCharacter::UpdateFacingTurn,
		FacingTurnTimerIntervalSeconds,
		true);
}

void APRPlayerCharacter::UpdateFacingTurn()
{
	const float ElapsedSeconds = GetWorld()->GetTimeSeconds() - FacingTurnStartTimeSeconds;
	const float Alpha = FMath::Clamp(ElapsedSeconds / FacingTurnDurationSeconds, 0.0f, 1.0f);
	const float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, FacingTurnEaseExponent);

	FRotator NewRotation = GetMesh()->GetRelativeRotation();
	NewRotation.Yaw = FMath::Lerp(FacingTurnStartYaw, FacingTurnTargetYaw, EasedAlpha);
	GetMesh()->SetRelativeRotation(NewRotation);

	if (Alpha >= 1.0f)
	{
		GetWorldTimerManager().ClearTimer(FacingTurnTimerHandle);
	}
}

bool APRPlayerCharacter::ResolveRequiredInputActions(
	const UInputAction*& MoveAction,
	const UInputAction*& JumpAction,
	const UInputAction*& AttackAction,
	const UInputAction*& DodgeAction,
	const UInputAction*& InteractAction,
	const UInputAction*& ExecuteAction) const
{
	const APRPlayerController* ProjectRController = Cast<APRPlayerController>(GetController());
	if (!IsValid(ProjectRController))
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerCharacter %s has no APRPlayerController."), *GetPathName());
		return false;
	}

	const UPRInputConfigDataAsset* InputConfig = ProjectRController->GetInputConfig();
	if (!IsValid(InputConfig))
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerController %s has no InputConfig."), *ProjectRController->GetPathName());
		return false;
	}

	MoveAction = InputConfig->FindInputActionForTag(UPRTagLibrary::GetInputMoveTag());
	JumpAction = InputConfig->FindInputActionForTag(UPRTagLibrary::GetInputJumpTag());
	AttackAction = InputConfig->FindInputActionForTag(UPRTagLibrary::GetInputAttackTag());
	DodgeAction = InputConfig->FindInputActionForTag(UPRTagLibrary::GetInputDodgeTag());
	InteractAction = InputConfig->FindInputActionForTag(UPRTagLibrary::GetInputInteractTag());
	ExecuteAction = InputConfig->FindInputActionForTag(UPRTagLibrary::GetInputExecuteTag());
	return MoveAction && JumpAction && AttackAction && DodgeAction && InteractAction && ExecuteAction;
}
