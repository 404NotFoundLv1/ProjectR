// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRInputAutomationToolset.h"

#include "Camera/CameraComponent.h"
#include "Characters/PRPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Core/PRPlayerController.h"
#include "Dom/JsonObject.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputDeviceLibrary.h"
#include "Input/PRInputConfigDataAsset.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Slate/SceneViewport.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UnrealClient.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

namespace PRInputAutomationToolset
{
enum class ESmokePhase : uint8
{
	MoveRight,
	MoveLeft,
	PrepareKeyboardJump,
	KeyboardJump,
	GamepadJump,
	SemanticKeyboard,
	SemanticGamepad,
	Complete
};

class FPIEInputSmokeRunner : public TSharedFromThis<FPIEInputSmokeRunner>
{
public:
	static UToolCallAsyncResultString* Start(
		float MoveDurationSeconds,
		float JumpTimeoutSeconds,
		float MaxYDriftCm)
	{
		TSharedRef<FPIEInputSmokeRunner> Runner = MakeShared<FPIEInputSmokeRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->MoveDuration = MoveDurationSeconds;
		Runner->JumpTimeout = JumpTimeoutSeconds;
		Runner->AllowedYDrift = MaxYDriftCm;
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}

		Runner->PhaseStartTime = FPlatformTime::Seconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](float DeltaSeconds) mutable
			{
				const bool bContinue = RunnerPtr->Tick(DeltaSeconds);
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

	~FPIEInputSmokeRunner()
	{
		ReleaseAllInputs();
	}

private:
	bool Initialize()
	{
		if (MoveDuration <= 0.0f || JumpTimeout <= 0.0f || AllowedYDrift < 0.0f)
		{
			Fail(TEXT("Input smoke arguments must be positive, and MaxYDriftCm cannot be negative."));
			return false;
		}

		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld))
		{
			Fail(TEXT("RunPIEInputSmoke requires an active in-process PIE world."));
			return false;
		}

		Controller = Cast<APRPlayerController>(PlayWorld->GetFirstPlayerController());
		Character = Controller.IsValid() ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		Viewport = PlayWorld->GetGameViewport();
		if (!Controller.IsValid() || !Character.IsValid() || !Viewport.IsValid())
		{
			Fail(TEXT("PIE must possess APRPlayerCharacter with APRPlayerController in a game viewport."));
			return false;
		}
		InputViewport = Viewport->GetGameViewport();
		InputDevice = UInputDeviceLibrary::GetDefaultInputDevice();
		if (!InputViewport || !InputDevice.IsValid())
		{
			Fail(TEXT("PIE game viewport or default input device is unavailable for simulated input."));
			return false;
		}

		const UPRInputConfigDataAsset* InputConfig = Controller->GetInputConfig();
		const UInputMappingContext* MappingContext = IsValid(InputConfig)
			? InputConfig->DefaultMappingContext.Get()
			: nullptr;
		ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer();
		UEnhancedInputLocalPlayerSubsystem* Subsystem = IsValid(LocalPlayer)
			? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
			: nullptr;
		bMappingContextActive = IsValid(Subsystem)
			&& IsValid(MappingContext)
			&& Subsystem->HasMappingContext(MappingContext);
		if (!bMappingContextActive)
		{
			Fail(TEXT("The configured ProjectR MappingContext is not active in PIE."));
			return false;
		}

		Camera = Character->FindComponentByClass<UCameraComponent>();
		if (!Camera.IsValid() || !IsValid(Character->GetMesh()))
		{
			Fail(TEXT("APRPlayerCharacter is missing its camera or skeletal mesh component."));
			return false;
		}

		InitialLocation = Character->GetActorLocation();
		InitialActorRotation = Character->GetActorRotation();
		InitialCameraTransform = Camera->GetRelativeTransform();
		InitialCameraFOV = Camera->FieldOfView;
		MaxYDriftObserved = 0.0f;
		Phase = ESmokePhase::PrepareKeyboardJump;
		SendKey(EKeys::D, IE_Pressed, 1.0f);
		return true;
	}

	bool Tick(float DeltaSeconds)
	{
		if (!Controller.IsValid() || !Character.IsValid() || !Viewport.IsValid()
			|| !GEditor || GEditor->PlayWorld != Character->GetWorld())
		{
			Fail(TEXT("PIE ended or the controlled ProjectR objects became invalid during input smoke."));
			return false;
		}

		SampleCharacter();
		const double PhaseElapsed = FPlatformTime::Seconds() - PhaseStartTime;
		switch (Phase)
		{
		case ESmokePhase::MoveRight:
			if (PhaseElapsed >= MoveDuration)
			{
				SendKey(EKeys::D, IE_Released, 1.0f);
				RightDeltaX = Character->GetActorLocation().X - RightStartX;
				RightMeshYaw = Character->GetMesh()->GetRelativeRotation().Yaw;
				LeftStartX = Character->GetActorLocation().X;
				SendKey(EKeys::A, IE_Pressed, 1.0f);
				Advance(ESmokePhase::MoveLeft);
			}
			break;

		case ESmokePhase::MoveLeft:
			if (PhaseElapsed >= MoveDuration)
			{
				SendKey(EKeys::A, IE_Released, 1.0f);
				LeftDeltaX = Character->GetActorLocation().X - LeftStartX;
				LeftMeshYaw = Character->GetMesh()->GetRelativeRotation().Yaw;
				GamepadJumpStartZ = Character->GetActorLocation().Z;
				SendKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, 1.0f);
				SendAxis(EKeys::Gamepad_LeftX, -1.0f);
				Advance(ESmokePhase::GamepadJump);
			}
			break;

		case ESmokePhase::PrepareKeyboardJump:
			if (PhaseElapsed >= 0.35)
			{
				KeyboardJumpStartZ = Character->GetActorLocation().Z;
				SendKey(EKeys::SpaceBar, IE_Pressed, 1.0f);
				Advance(ESmokePhase::KeyboardJump);
			}
			break;

		case ESmokePhase::KeyboardJump:
			if (!bKeyboardJumpReleased && PhaseElapsed >= 0.10)
			{
				SendKey(EKeys::SpaceBar, IE_Released, 1.0f);
				bKeyboardJumpReleased = true;
			}
			if (!bKeyboardMoveReleased && PhaseElapsed >= MoveDuration)
			{
				SendKey(EKeys::D, IE_Released, 1.0f);
				bKeyboardMoveReleased = true;
			}
			KeyboardJumpRise = FMath::Max(
				KeyboardJumpRise,
				Character->GetActorLocation().Z - KeyboardJumpStartZ);
			if (KeyboardJumpRise > 10.0f
				&& Character->GetCharacterMovement()->IsMovingOnGround()
				&& PhaseElapsed > 0.20)
			{
				RightStartX = Character->GetActorLocation().X;
				SendKey(EKeys::D, IE_Pressed, 1.0f);
				Advance(ESmokePhase::MoveRight);
			}
			else if (PhaseElapsed > JumpTimeout)
			{
				Fail(TEXT("Keyboard jump did not rise and return to ground before timeout."));
				return false;
			}
			break;

		case ESmokePhase::GamepadJump:
			if (PhaseElapsed < MoveDuration)
			{
				SendAxis(EKeys::Gamepad_LeftX, -1.0f);
			}
			else if (!bGamepadAxisReleased)
			{
				SendAxis(EKeys::Gamepad_LeftX, 0.0f);
				bGamepadAxisReleased = true;
			}
			if (!bGamepadJumpReleased && PhaseElapsed >= 0.10)
			{
				SendKey(EKeys::Gamepad_FaceButton_Bottom, IE_Released, 1.0f);
				bGamepadJumpReleased = true;
			}
			GamepadJumpRise = FMath::Max(
				GamepadJumpRise,
				Character->GetActorLocation().Z - GamepadJumpStartZ);
			if (GamepadJumpRise > 10.0f
				&& Character->GetCharacterMovement()->IsMovingOnGround()
				&& PhaseElapsed > 0.20)
			{
				SendAxis(EKeys::Gamepad_LeftX, 0.0f);
				SemanticIndex = 0;
				bSemanticPressed = false;
				Advance(ESmokePhase::SemanticKeyboard);
			}
			else if (PhaseElapsed > JumpTimeout)
			{
				Fail(TEXT("Gamepad jump did not rise and return to ground before timeout."));
				return false;
			}
			break;

		case ESmokePhase::SemanticKeyboard:
			if (RunSemanticSequence(KeyboardSemanticKeys, PhaseElapsed))
			{
				SemanticIndex = 0;
				bSemanticPressed = false;
				Advance(ESmokePhase::SemanticGamepad);
			}
			break;

		case ESmokePhase::SemanticGamepad:
			if (RunSemanticSequence(GamepadSemanticKeys, PhaseElapsed))
			{
				Advance(ESmokePhase::Complete);
			}
			break;

		case ESmokePhase::Complete:
			return Finish();
		}

		return true;
	}

	template <int32 Count>
	bool RunSemanticSequence(const FKey (&Keys)[Count], double PhaseElapsed)
	{
		if (SemanticIndex >= Count)
		{
			return true;
		}

		if (!bSemanticPressed)
		{
			SendKey(Keys[SemanticIndex], IE_Pressed, 1.0f);
			bSemanticPressed = true;
			PhaseStartTime = FPlatformTime::Seconds();
		}
		else if (PhaseElapsed >= 0.08)
		{
			SendKey(Keys[SemanticIndex], IE_Released, 1.0f);
			++SemanticIndex;
			bSemanticPressed = false;
			PhaseStartTime = FPlatformTime::Seconds();
		}
		return SemanticIndex >= Count;
	}

	void SampleCharacter()
	{
		MaxYDriftObserved = FMath::Max(
			MaxYDriftObserved,
			FMath::Abs(Character->GetActorLocation().Y - InitialLocation.Y));
	}

	void Advance(ESmokePhase NextPhase)
	{
		Phase = NextPhase;
		PhaseStartTime = FPlatformTime::Seconds();
	}

	bool Finish()
	{
		ReleaseAllInputs();
		SampleCharacter();
		LeftMeshYaw = Character->GetMesh()->GetRelativeRotation().Yaw;
		const bool bRightMoved = RightDeltaX > 1.0f;
		const bool bLeftMoved = LeftDeltaX < -1.0f;
		const bool bKeyboardJumped = KeyboardJumpRise > 10.0f;
		const bool bGamepadJumped = GamepadJumpRise > 10.0f;
		const bool bPlaneHeld = MaxYDriftObserved <= AllowedYDrift;
		const auto AngularDistance = [](float From, float To)
		{
			return FMath::Abs(FMath::FindDeltaAngleDegrees(From, To));
		};
		const bool bRightFacing = AngularDistance(RightMeshYaw, -90.0f) <= 0.5f;
		const bool bLeftFacing = AngularDistance(LeftMeshYaw, 90.0f) <= 0.5f;
		const bool bActorRotationStable = Character->GetActorRotation().Equals(InitialActorRotation, 0.01f);
		const bool bCameraStable = Camera->GetRelativeTransform().Equals(InitialCameraTransform, 0.01f)
			&& FMath::IsNearlyEqual(Camera->FieldOfView, InitialCameraFOV, 0.01f);
		const bool bStandardPassed = bMappingContextActive && bRightMoved && bLeftMoved
			&& bKeyboardJumped && bGamepadJumped && bPlaneHeld
			&& bRightFacing && bLeftFacing
			&& bActorRotationStable && bCameraStable;
		const bool bPassed = bStandardPassed;

		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("status"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
		Json->SetBoolField(TEXT("mappingContextActive"), bMappingContextActive);
		Json->SetNumberField(TEXT("rightDeltaX"), RightDeltaX);
		Json->SetNumberField(TEXT("leftDeltaX"), LeftDeltaX);
		Json->SetNumberField(TEXT("keyboardJumpRise"), KeyboardJumpRise);
		Json->SetNumberField(TEXT("gamepadJumpRise"), GamepadJumpRise);
		Json->SetNumberField(TEXT("maxYDriftCm"), MaxYDriftObserved);
		Json->SetNumberField(TEXT("rightMeshYaw"), RightMeshYaw);
		Json->SetNumberField(TEXT("leftMeshYaw"), LeftMeshYaw);
		Json->SetBoolField(TEXT("actorRotationStable"), bActorRotationStable);
		Json->SetBoolField(TEXT("cameraStable"), bCameraStable);

		FString JsonString;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
		FJsonSerializer::Serialize(Json, Writer);
		if (bPassed)
		{
			Result->SetValue(JsonString);
		}
		else
		{
			Result->SetError(JsonString);
		}
		return false;
	}

	void Fail(const FString& Message)
	{
		ReleaseAllInputs();
		if (Result.IsValid())
		{
			Result->SetError(Message);
		}
	}

	void SendKey(const FKey& Key, EInputEvent Event, float Amount)
	{
		if (HasLiveInputViewport())
		{
			Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(
				Key,
				Event,
				Amount,
				-1,
				InputDevice,
				false,
				InputViewport));
		}
	}

	void SendAxis(const FKey& Key, float Amount)
	{
		SendKey(Key, IE_Axis, Amount);
	}

	void ReleaseAllInputs()
	{
		if (!HasLiveInputViewport())
		{
			return;
		}

		const FKey KeysToRelease[] = {
			EKeys::A,
			EKeys::D,
			EKeys::SpaceBar,
			EKeys::LeftMouseButton,
			EKeys::LeftShift,
			EKeys::E,
			EKeys::F,
			EKeys::Gamepad_FaceButton_Bottom,
			EKeys::Gamepad_FaceButton_Left,
			EKeys::Gamepad_FaceButton_Right,
			EKeys::Gamepad_FaceButton_Top,
			EKeys::Gamepad_RightShoulder};
		for (const FKey& Key : KeysToRelease)
		{
			SendKey(Key, IE_Released, 1.0f);
		}
		SendAxis(EKeys::Gamepad_LeftX, 0.0f);
	}

	bool HasLiveInputViewport() const
	{
		return Viewport.IsValid()
			&& InputViewport
			&& Viewport->GetGameViewport() == InputViewport;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<APRPlayerController> Controller;
	TWeakObjectPtr<APRPlayerCharacter> Character;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	FViewport* InputViewport = nullptr;
	FInputDeviceId InputDevice = INPUTDEVICEID_NONE;
	TWeakObjectPtr<UCameraComponent> Camera;
	ESmokePhase Phase = ESmokePhase::PrepareKeyboardJump;
	double PhaseStartTime = 0.0;
	float MoveDuration = 0.30f;
	float JumpTimeout = 2.0f;
	float AllowedYDrift = 0.1f;
	FVector InitialLocation = FVector::ZeroVector;
	FRotator InitialActorRotation = FRotator::ZeroRotator;
	FTransform InitialCameraTransform = FTransform::Identity;
	float InitialCameraFOV = 0.0f;
	float RightStartX = 0.0f;
	float LeftStartX = 0.0f;
	float RightDeltaX = 0.0f;
	float LeftDeltaX = 0.0f;
	float RightMeshYaw = 0.0f;
	float LeftMeshYaw = 0.0f;
	float KeyboardJumpStartZ = 0.0f;
	float KeyboardJumpRise = 0.0f;
	float GamepadJumpStartZ = 0.0f;
	float GamepadJumpRise = 0.0f;
	float MaxYDriftObserved = 0.0f;
	bool bMappingContextActive = false;
	bool bKeyboardJumpReleased = false;
	bool bKeyboardMoveReleased = false;
	bool bGamepadJumpReleased = false;
	bool bGamepadAxisReleased = false;
	bool bSemanticPressed = false;
	int32 SemanticIndex = 0;
	const FKey KeyboardSemanticKeys[4] = {
		EKeys::LeftMouseButton, EKeys::LeftShift, EKeys::E, EKeys::F};
	const FKey GamepadSemanticKeys[4] = {
		EKeys::Gamepad_FaceButton_Left,
		EKeys::Gamepad_FaceButton_Right,
		EKeys::Gamepad_FaceButton_Top,
		EKeys::Gamepad_RightShoulder};
};
} // namespace PRInputAutomationToolset

UToolCallAsyncResultString* UPRInputAutomationToolset::RunPIEInputSmoke(
	float MoveDurationSeconds,
	float JumpTimeoutSeconds,
	float MaxYDriftCm)
{
	return PRInputAutomationToolset::FPIEInputSmokeRunner::Start(
		MoveDurationSeconds,
		JumpTimeoutSeconds,
		MaxYDriftCm);
}
