// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/PRInputConfigDataAsset.h"

#include "Core/PRTagLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/DataValidation.h"
#include "ProjectR.h"

#define LOCTEXT_NAMESPACE "PRInputConfigDataAsset"

const UInputAction* UPRInputConfigDataAsset::FindInputActionForTag(FGameplayTag InputTag) const
{
	if (!InputTag.IsValid())
	{
		UE_LOG(LogProjectR, Error, TEXT("InputConfig %s received an invalid InputTag."), *GetPathName());
		return nullptr;
	}

	for (const FPRTaggedInputAction& Entry : TaggedInputActions)
	{
		if (Entry.InputTag == InputTag)
		{
			if (!IsValid(Entry.InputAction))
			{
				UE_LOG(
					LogProjectR,
					Error,
					TEXT("InputConfig %s maps %s to a null InputAction."),
					*GetPathName(),
					*InputTag.ToString());
				return nullptr;
			}

			return Entry.InputAction;
		}
	}

	UE_LOG(
		LogProjectR,
		Error,
		TEXT("InputConfig %s has no configured InputAction for %s."),
		*GetPathName(),
		*InputTag.ToString());
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UPRInputConfigDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	bool bIsValid = true;
	if (!IsValid(DefaultMappingContext))
	{
		Context.AddError(LOCTEXT("MissingMappingContext", "DefaultMappingContext must be configured."));
		bIsValid = false;
	}

	TSet<FGameplayTag> SeenTags;
	TSet<const UInputAction*> SeenActions;
	for (int32 Index = 0; Index < TaggedInputActions.Num(); ++Index)
	{
		const FPRTaggedInputAction& Entry = TaggedInputActions[Index];
		if (!Entry.InputTag.IsValid())
		{
			Context.AddError(FText::Format(
				LOCTEXT("InvalidTag", "TaggedInputActions[{0}] has an invalid InputTag."),
				FText::AsNumber(Index)));
			bIsValid = false;
		}
		else if (SeenTags.Contains(Entry.InputTag))
		{
			Context.AddError(FText::Format(
				LOCTEXT("DuplicateTag", "InputTag {0} is configured more than once."),
				FText::FromString(Entry.InputTag.ToString())));
			bIsValid = false;
		}
		else
		{
			SeenTags.Add(Entry.InputTag);
		}

		if (!IsValid(Entry.InputAction))
		{
			Context.AddError(FText::Format(
				LOCTEXT("MissingAction", "TaggedInputActions[{0}] has a null InputAction."),
				FText::AsNumber(Index)));
			bIsValid = false;
			continue;
		}

		if (SeenActions.Contains(Entry.InputAction))
		{
			Context.AddError(FText::Format(
				LOCTEXT("DuplicateAction", "InputAction {0} is configured more than once."),
				FText::FromString(Entry.InputAction->GetPathName())));
			bIsValid = false;
		}
		else
		{
			SeenActions.Add(Entry.InputAction);
		}

		if (Entry.InputTag == UPRTagLibrary::GetInputMoveTag()
			&& Entry.InputAction->ValueType != EInputActionValueType::Axis1D)
		{
			Context.AddError(LOCTEXT("MoveType", "Input.Move must use an Axis1D InputAction."));
			bIsValid = false;
		}

		const bool bIsCurrentButton =
			Entry.InputTag == UPRTagLibrary::GetInputJumpTag()
			|| Entry.InputTag == UPRTagLibrary::GetInputAttackTag()
			|| Entry.InputTag == UPRTagLibrary::GetInputDodgeTag()
			|| Entry.InputTag == UPRTagLibrary::GetInputInteractTag()
			|| Entry.InputTag == UPRTagLibrary::GetInputExecuteTag();
		if (bIsCurrentButton && Entry.InputAction->ValueType != EInputActionValueType::Boolean)
		{
			Context.AddError(FText::Format(
				LOCTEXT("ButtonType", "{0} must use a Boolean InputAction."),
				FText::FromString(Entry.InputTag.ToString())));
			bIsValid = false;
		}
	}

	return bIsValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif

#undef LOCTEXT_NAMESPACE
