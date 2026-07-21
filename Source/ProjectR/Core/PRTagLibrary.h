// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "PRTagLibrary.generated.h"

/** Checked C++ access to common ProjectR gameplay tags defined in DefaultGameplayTags.ini. */
UCLASS(Abstract)
class PROJECTR_API UPRTagLibrary : public UObject
{
	GENERATED_BODY()

public:
	static const FGameplayTag& GetInputMoveTag();
	static const FGameplayTag& GetInputLookTag();
	static const FGameplayTag& GetInputJumpTag();
	static const FGameplayTag& GetInputAttackTag();
	static const FGameplayTag& GetInputDodgeTag();
	static const FGameplayTag& GetInputInteractTag();
	static const FGameplayTag& GetInputExecuteTag();
	static const FGameplayTag& GetInputSkillShadowThrustTag();
	static const FGameplayTag& GetInputSkillFireSlashTag();
	static const FGameplayTag& GetInputSkillThunderDropTag();
	static const FGameplayTag& GetInputSkillVectorHookTag();
	static const FGameplayTag& GetInputSkillCounterProofWallTag();

	static const FGameplayTag& GetAbilityActivateFailCanActivateTag();
	static const FGameplayTag& GetAbilityActivateFailCooldownTag();
	static const FGameplayTag& GetAbilityActivateFailCostTag();
	static const FGameplayTag& GetAbilityActivateFailNetworkingTag();
	static const FGameplayTag& GetAbilityActivateFailTagsBlockedTag();
	static const FGameplayTag& GetAbilityActivateFailTagsMissingTag();
	static const FGameplayTag& GetAbilityActivateFailNoTargetTag();
	static const FGameplayTag& GetAbilityActivateFailObstructedTag();
	static const FGameplayTag& GetAbilityActivateFailInvalidMovementTag();
	static const FGameplayTag& GetAbilityStatePlayerSkillActiveTag();

	static const FGameplayTag& GetCombatDataDamageTag();
	static const FGameplayTag& GetCombatEventDamageTag();
	static const FGameplayTag& GetCombatEventDamageRejectedTag();
	static const FGameplayTag& GetCombatEventDeathTag();
	static const FGameplayTag& GetCombatEventReviveTag();
	static const FGameplayTag& GetCombatEventAbilityOutcomeTag();
	static const FGameplayTag& GetCombatResponseDisplacementAppliedTag();
	static const FGameplayTag& GetCombatResponseDecoyCreatedTag();
	static const FGameplayTag& GetCombatResponseDecoyConsumedTag();
	static const FGameplayTag& GetCombatResponseGuardBlockedTag();
	static const FGameplayTag& GetCombatResponsePerfectTimingTag();
	static const FGameplayTag& GetCombatResponseHealthDamagedTag();
	static const FGameplayTag& GetCombatResponseShieldAbsorbedTag();
	static const FGameplayTag& GetCombatResponseShieldBrokenTag();
	static const FGameplayTag& GetCombatResponsePredictionBlockedTag();

	static const FGameplayTag& GetStateAliveTag();
	static const FGameplayTag& GetStateDeadTag();
	static const FGameplayTag& GetStateInCombatTag();
	static const FGameplayTag& GetStateInvulnerableTag();
	static const FGameplayTag& GetStateStunnedTag();
	static const FGameplayTag& GetStateBurningTag();
	static const FGameplayTag& GetStateGuardingTag();

	static const FGameplayTag& GetEnemyDataHealthTag();
	static const FGameplayTag& GetEnemyDataMaxHealthTag();
	static const FGameplayTag& GetEnemyDataShieldTag();
	static const FGameplayTag& GetEnemyDataMaxShieldTag();
	static const FGameplayTag& GetEnemyDataEnergyTag();
	static const FGameplayTag& GetEnemyDataMaxEnergyTag();
	static const FGameplayTag& GetEnemyDataAttackPowerTag();
	static const FGameplayTag& GetEnemyDataMoveSpeedTag();
	static const FGameplayTag& GetEnemyDataCritChanceTag();
	static const FGameplayTag& GetEnemyDataPermissionTag();
	static const FGameplayTag& GetEnemyDataResonanceTag();
	static const FGameplayTag& GetEnemyAIEventReevaluateTag();

	static const FGameplayTag& GetQTEResultSuccessTag();
	static const FGameplayTag& GetQTEResultFailureTag();
	static const FGameplayTag& GetQTEResultRejectedTag();
	static const FGameplayTag& GetQTEResultTimeoutTag();

private:
	static FGameplayTag RequestRequiredTag(const FName& TagName);
};
