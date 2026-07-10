// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core/PRMapId.h"
#include "Engine/GameInstance.h"

#include "PRGameInstance.generated.h"

class UWorld;

/** Owns stable project-wide lifecycle and formal map travel entry points. */
UCLASS()
class PROJECTR_API UPRGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Opens the formal map identified by MapId. Returns false when the request is invalid. */
	UFUNCTION(BlueprintCallable, Category="ProjectR|Travel", meta=(DisplayName="Open ProjectR Map"))
	bool OpenMap(EPRMapId MapId);

private:
	static TSoftObjectPtr<UWorld> ResolveMap(EPRMapId MapId);
};
