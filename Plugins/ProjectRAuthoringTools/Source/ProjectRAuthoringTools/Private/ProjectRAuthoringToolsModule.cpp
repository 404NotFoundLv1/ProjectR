// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "PRAbilityAutomationToolset.h"
#include "PRCombatAutomationToolset.h"
#include "PRInputAutomationToolset.h"
#include "PRPlayerSkillAutomationToolset.h"
#include "PRPlayerSkillStateEffectToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

class FProjectRAuthoringToolsModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UPRAbilityAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRCombatAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRInputAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRPlayerSkillAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRPlayerSkillStateEffectToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		UToolsetRegistry::UnregisterToolsetClass(UPRPlayerSkillStateEffectToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRPlayerSkillAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRInputAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRCombatAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRAbilityAutomationToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FProjectRAuthoringToolsModule, ProjectRAuthoringTools)
