// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "PRInputAutomationToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

class FProjectRAuthoringToolsModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UPRInputAutomationToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		UToolsetRegistry::UnregisterToolsetClass(UPRInputAutomationToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FProjectRAuthoringToolsModule, ProjectRAuthoringTools)
