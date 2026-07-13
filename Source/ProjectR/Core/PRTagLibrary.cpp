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
