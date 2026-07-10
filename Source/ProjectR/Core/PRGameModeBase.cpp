// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/PRGameModeBase.h"

#include "Characters/PRPlayerCharacter.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR.h"

APRGameModeBase::APRGameModeBase()
{
	DefaultPawnClass = APRPlayerCharacter::StaticClass();
	PlayerControllerClass = APRPlayerController::StaticClass();
	PlayerStateClass = APRPlayerState::StaticClass();
}

void APRGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	UE_LOG(
		LogProjectR,
		Log,
		TEXT("Framework spawn: Controller=%s PlayerState=%s Pawn=%s"),
		*GetNameSafe(NewPlayer),
		*GetNameSafe(NewPlayer != nullptr ? NewPlayer->PlayerState : nullptr),
		*GetNameSafe(NewPlayer != nullptr ? NewPlayer->GetPawn() : nullptr));
}
