// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core/PRNetworkRunGameMode.h"

#include "PRBossPrototypeGameMode.generated.h"

/** Authority-only, fixed-registry entry point for the Auditor prototype encounter. */
UCLASS()
class PROJECTR_API APRBossPrototypeGameMode : public APRNetworkRunGameMode
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	void SpawnAuditorWhenRegistryReady();
	FDelegateHandle RegistryReadyHandle;
};
