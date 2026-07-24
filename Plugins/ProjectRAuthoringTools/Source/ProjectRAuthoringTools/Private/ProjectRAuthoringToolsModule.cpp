// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "PRAbilityAutomationToolset.h"
#include "PRCombatAutomationToolset.h"
#include "PRInputAutomationToolset.h"
#include "PRPlayerSkillAutomationToolset.h"
#include "PRPlayerSkillPresentationToolset.h"
#include "PRPlayerSkillStateEffectToolset.h"
#include "PREnemyAuthoringToolset.h"
#include "PREnemyAutomationToolset.h"
#include "PRBossAuthoringToolset.h"
#include "PRBalanceAutomationToolset.h"
#include "PRCombatHUDAuthoringToolset.h"
#include "PRCompanionAutomationToolset.h"
#include "PRCompanionAuthoringToolset.h"
#include "PRQTEAuthoringToolset.h"
#include "PRQTEAutomationToolset.h"
#include "PRDialogueAuthoringToolset.h"
#include "PRDialogueAutomationToolset.h"
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
		UToolsetRegistry::RegisterToolsetClass(UPRPlayerSkillPresentationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRPlayerSkillStateEffectToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPREnemyAuthoringToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPREnemyAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRBossAuthoringToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRBalanceAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRCombatHUDAuthoringToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRCompanionAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRCompanionAuthoringToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRQTEAuthoringToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRQTEAutomationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRDialogueAuthoringToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UPRDialogueAutomationToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		UToolsetRegistry::UnregisterToolsetClass(UPRQTEAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRDialogueAuthoringToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRDialogueAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRQTEAuthoringToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRCompanionAuthoringToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRCompanionAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRCombatHUDAuthoringToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRBalanceAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRBossAuthoringToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPREnemyAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPREnemyAuthoringToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRPlayerSkillStateEffectToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRPlayerSkillPresentationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRPlayerSkillAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRInputAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRCombatAutomationToolset::StaticClass());
		UToolsetRegistry::UnregisterToolsetClass(UPRAbilityAutomationToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FProjectRAuthoringToolsModule, ProjectRAuthoringTools)
