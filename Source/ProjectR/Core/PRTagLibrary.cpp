// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/PRTagLibrary.h"

FGameplayTag UPRTagLibrary::RequestRequiredTag(const FName& TagName)
{
	return FGameplayTag::RequestGameplayTag(TagName, true);
}

const FGameplayTag& UPRTagLibrary::GetInputMoveTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Move"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputLookTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Look"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputJumpTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Jump"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputAttackTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Attack"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputDodgeTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Dodge"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputInteractTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Interact"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputExecuteTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Execute"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputQTERejectTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.QTE.Reject"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputSkillShadowThrustTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Skill.ShadowThrust"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputSkillFireSlashTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Skill.FireSlash"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputSkillThunderDropTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Skill.ThunderDrop"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputSkillVectorHookTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Skill.VectorHook"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetInputSkillCounterProofWallTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Input.Skill.CounterProofWall"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetSkillBasicAttackTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Skill.BasicAttack"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailCanActivateTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.CanActivate"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailCooldownTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.Cooldown"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailCostTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.Cost"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailNetworkingTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.Networking"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailTagsBlockedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.TagsBlocked"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailTagsMissingTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.TagsMissing"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailNoTargetTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.NoTarget"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailObstructedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.Obstructed"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.ActivateFail.InvalidMovement"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetAbilityStatePlayerSkillActiveTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Ability.State.PlayerSkillActive"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatDataDamageTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Data.Damage"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatEventDamageTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Event.Damage"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatEventDamageRejectedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Event.DamageRejected"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatEventDeathTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Event.Death"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatEventReviveTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Event.Revive"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatEventAbilityOutcomeTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Event.AbilityOutcome"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponseDisplacementAppliedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.DisplacementApplied"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponseDecoyCreatedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.DecoyCreated"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponseDecoyConsumedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.DecoyConsumed"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponseGuardBlockedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.GuardBlocked"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponsePerfectTimingTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.PerfectTiming"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponseHealthDamagedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.HealthDamaged"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponseShieldAbsorbedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.ShieldAbsorbed"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponseShieldBrokenTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.ShieldBroken"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetCombatResponsePredictionBlockedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Combat.Response.PredictionBlocked"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetStateAliveTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("State.Alive"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetStateDeadTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("State.Dead"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetStateInCombatTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("State.InCombat"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetStateInvulnerableTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("State.Invulnerable"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetStateStunnedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("State.Stunned"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetStateBurningTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("State.Burning"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetStateGuardingTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("State.Guarding"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataHealthTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.Health"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataMaxHealthTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.MaxHealth"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataShieldTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.Shield"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataMaxShieldTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.MaxShield"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataEnergyTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.Energy"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataMaxEnergyTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.MaxEnergy"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataAttackPowerTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.AttackPower"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataMoveSpeedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.MoveSpeed"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataCritChanceTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.CritChance"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataPermissionTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.Permission"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyDataResonanceTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.Data.Resonance"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetEnemyAIEventReevaluateTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("Enemy.AI.Event.Reevaluate"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetQTEResultSuccessTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("QTE.Result.Success"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetQTEResultFailureTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("QTE.Result.Failure"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetQTEResultRejectedTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("QTE.Result.Rejected"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetQTEResultTimeoutTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("QTE.Result.Timeout"));
	return Tag;
}

const FGameplayTag& UPRTagLibrary::GetQTEResultCancelledTag()
{
	static const FGameplayTag Tag = RequestRequiredTag(TEXT("QTE.Result.Cancelled"));
	return Tag;
}
