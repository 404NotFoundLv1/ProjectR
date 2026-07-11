// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/PRPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Input/PRInputConfigDataAsset.h"
#include "InputMappingContext.h"
#include "ProjectR.h"

const UPRInputConfigDataAsset* APRPlayerController::GetInputConfig() const
{
	return DefaultInputConfig;
}

void APRPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureDefaultMappingContext();
}

void APRPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	EnsureDefaultMappingContext();
}

void APRPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveOwnedMappingContext();
	Super::EndPlay(EndPlayReason);
}

bool APRPlayerController::EnsureDefaultMappingContext()
{
	if (!IsLocalController())
	{
		return false;
	}

	if (!IsValid(DefaultInputConfig))
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerController %s has no DefaultInputConfig."), *GetPathName());
		return false;
	}

	UInputMappingContext* DesiredContext = DefaultInputConfig->DefaultMappingContext;
	if (!IsValid(DesiredContext))
	{
		UE_LOG(
			LogProjectR,
			Error,
			TEXT("InputConfig %s has no DefaultMappingContext."),
			*DefaultInputConfig->GetPathName());
		return false;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		UE_LOG(LogProjectR, Error, TEXT("Local PlayerController %s has no LocalPlayer."), *GetPathName());
		return false;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!IsValid(Subsystem))
	{
		UE_LOG(
			LogProjectR,
			Error,
			TEXT("LocalPlayer for %s has no EnhancedInput subsystem."),
			*GetPathName());
		return false;
	}

	if (AppliedMappingContext != DesiredContext)
	{
		RemoveOwnedMappingContext();
		AppliedMappingContext = DesiredContext;
	}

	if (Subsystem->HasMappingContext(DesiredContext))
	{
		return true;
	}

	Subsystem->AddMappingContext(DesiredContext, DefaultInputConfig->MappingPriority);
	AppliedMappingContext = DesiredContext;
	bOwnsAppliedMappingContext = true;
	UE_LOG(
		LogProjectR,
		Display,
		TEXT("ProjectR Input MappingContext added once: %s (Priority=%d)."),
		*DesiredContext->GetPathName(),
		DefaultInputConfig->MappingPriority);
	return true;
}

void APRPlayerController::RemoveOwnedMappingContext()
{
	if (!bOwnsAppliedMappingContext || !IsValid(AppliedMappingContext))
	{
		AppliedMappingContext = nullptr;
		bOwnsAppliedMappingContext = false;
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->RemoveMappingContext(AppliedMappingContext);
		}
	}

	AppliedMappingContext = nullptr;
	bOwnsAppliedMappingContext = false;
}
