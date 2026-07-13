// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "PRAbilityAutomationToolset.h"
#include "PRCombatAutomationToolset.h"
#include "PRInputAutomationToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

class FProjectRAuthoringToolsModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UPRAbilityAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRCombatAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRInputAutomationToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		UToolsetRegistry::UnregisterToolsetClass(UPRInputAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRCombatAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRAbilityAutomationToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FProjectRAuthoringToolsModule, ProjectRAuthoringTools)
