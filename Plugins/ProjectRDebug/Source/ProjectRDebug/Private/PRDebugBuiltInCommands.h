// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PRDebugCommandTypes.h"

/** Fixed whitelist implemented only through existing ProjectR runtime contracts. */
struct FPRDebugBuiltInCommands
{
	static TArray<FPRDebugCommandDescriptor> BuildDescriptors();
	static FPRDebugCommandResult Execute(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);

private:
	static FPRDebugCommandResult BuildStatus(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);
	static FPRDebugCommandResult DamageSelf(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);
	static FPRDebugCommandResult ReviveSelf(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);
	static FPRDebugCommandResult ReadSaveState(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);
	static FPRDebugCommandResult TravelToCombatGym(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);
};
