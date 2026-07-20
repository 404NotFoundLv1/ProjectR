// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "PREnemyAuthoringToolset.generated.h"

/** Fixed, editor-only writer for the v0.2.1-A twelve-package manifest. */
UCLASS()
class PROJECTRAUTHORINGTOOLS_API UPREnemyAuthoringToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Approved recovery operation. Deletes only the fixed v0.2.1-A manifest
	 * packages and verifies both their asset-registry and on-disk absence.
	 */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* ResetCheckpointAEnemyManifest();

	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* CreateCheckpointAEnemyAssets();

	/** Recovers only the documented partial-save preimage and saves each package individually. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RecoverAndSaveCheckpointAEnemyAssets();

	/** Recreates only the two fixed GameplayEffect Blueprints after an approved exact-package repair. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RepairCheckpointAEffectBlueprints();

	/** Clears the one stale default-effect CDO reference before exact asset deletion and immediate repair. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* ClearCheckpointAStaleEffectReference();

	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* ConfigureCheckpointAStateTree();

	/** Corrects only the verified P0 melee-strike range preimage; no caller input is accepted. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* CorrectCheckpointAMeleeStrikeRange();

	/** Fixed, editor-only writer for the v0.2.1-B nine-package Ranged manifest and Registry append. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* CreateCheckpointBEnemyAssets();

	/** Repairs only the verified absent B projectile Blueprint; no path, class, or save input is accepted. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RepairCheckpointBProjectileBlueprint();

	/** Rebinds only the verified post-compile B Ranged CDO defaults; callers cannot select assets or values. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RepairCheckpointBRangedBindings();

	/** Fixed, editor-only writer for the v0.2.1-C eight-package Shield manifest and Registry append. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* CreateCheckpointCEnemyAssets();

	/** Saves exactly the fixed v0.2.1-C Shield manifest and Registry append. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* SaveCheckpointCEnemyAssets();

	/** Applies the approved v0.2.1-B shared-default GE order repair: each Max attribute precedes its Current attribute. */
	UFUNCTION(meta=(AICallable))
	static class UToolCallAsyncResultString* RepairCheckpointBDefaultAttributeModifierOrder();

};
