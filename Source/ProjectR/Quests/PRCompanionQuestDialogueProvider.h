// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Quests/PRCompanionQuestTypes.h"
class PROJECTR_API FPRCompanionQuestDialogueProvider
{
public:
	static bool GetCompletionLine(FName QuestId, FPRCompanionQuestDialogueLine& OutLine);
	static bool GetEntitlementLine(FName EntitlementId, FPRCompanionQuestDialogueLine& OutLine);
};
