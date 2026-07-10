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
