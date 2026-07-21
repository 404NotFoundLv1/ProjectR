// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "PRCombatHUDAuthoringToolset.generated.h"
/** Fixed v0.2.3 HUD manifest writer; it accepts no caller input and never saves maps. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPRCombatHUDAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()
public:
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* CreateCombatHUDManifest();
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* RepairCombatHUDWidgetTrees();
	/** Recreates only the two approved v0.2.3 Boss WidgetBlueprint packages after their exact-path deletion. */
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* RecreateBossWidgetBlueprints();
	/** Removes the fixed Boss widget references from WBP_CombatHUD before exact-path recreation. */
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* DetachBossWidgetReferences();
	/** Inspects the fixed v0.2.3 HUD composition in an already-running PIE session; it never writes assets or maps. */
	UFUNCTION(meta=(AICallable)) static class UToolCallAsyncResultString* InspectActiveCombatHUDPIE();
};
