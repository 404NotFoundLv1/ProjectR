// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/GameModeBase.h"

#include "PRGameModeBase.generated.h"

/** Base GameMode for formal ProjectR worlds. */
UCLASS(Abstract)
class PROJECTR_API APRGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	APRGameModeBase();

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};
