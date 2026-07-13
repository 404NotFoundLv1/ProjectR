// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/PRGameplayAbility.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffect.h"
#include "ProjectR.h"

UPRGameplayAbility::UPRGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UPRGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!bCanActivateWhileDead && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		&& ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(UPRTagLibrary::GetAbilityActivateFailTagsBlockedTag());
			OptionalRelevantTags->AddTag(UPRTagLibrary::GetStateDeadTag());
		}
		return false;
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UPRGameplayAbility::CommitAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	FGameplayTagContainer* OptionalRelevantTags)
{
	FGameplayTagContainer FailureTags;
	FGameplayTagContainer* RelevantTags = OptionalRelevantTags ? OptionalRelevantTags : &FailureTags;
	UPRAbilitySystemComponent* ASC = ActorInfo
		? Cast<UPRAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get())
		: nullptr;
	if (!ASC)
	{
		UE_LOG(LogProjectR, Error, TEXT("Ability %s cannot commit without a ProjectR ASC."), *GetPathName());
		return false;
	}

	if (!CheckCooldown(Handle, ActorInfo, RelevantTags))
	{
		ASC->NotifyProjectRCommitFailed(this, *RelevantTags);
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return false;
	}
	if (!CheckCost(Handle, ActorInfo, RelevantTags))
	{
		ASC->NotifyProjectRCommitFailed(this, *RelevantTags);
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return false;
	}

	FActiveGameplayEffectHandle CooldownHandle;
	if (const UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect())
	{
		CooldownHandle = ApplyGameplayEffectToOwner(
			Handle, ActorInfo, ActivationInfo, CooldownEffect, GetAbilityLevel(Handle, ActorInfo));
		if (!CooldownHandle.WasSuccessfullyApplied())
		{
			RelevantTags->AddTag(UPRTagLibrary::GetAbilityActivateFailCooldownTag());
			ASC->NotifyProjectRCommitFailed(this, *RelevantTags);
			CancelAbility(Handle, ActorInfo, ActivationInfo, true);
			return false;
		}
	}

	if (const UGameplayEffect* CostEffect = GetCostGameplayEffect())
	{
		const FActiveGameplayEffectHandle CostHandle = ApplyGameplayEffectToOwner(
			Handle, ActorInfo, ActivationInfo, CostEffect, GetAbilityLevel(Handle, ActorInfo));
		if (!CostHandle.WasSuccessfullyApplied())
		{
			RelevantTags->AddTag(UPRTagLibrary::GetAbilityActivateFailCostTag());
			if (CooldownHandle.IsValid() && !ASC->RemoveActiveGameplayEffect(CooldownHandle))
			{
				UE_LOG(LogProjectR, Error,
					TEXT("Ability %s failed to roll back its commit cooldown."), *GetPathName());
			}
			ASC->NotifyProjectRCommitFailed(this, *RelevantTags);
			CancelAbility(Handle, ActorInfo, ActivationInfo, true);
			return false;
		}
	}

	K2_CommitExecute();
	ASC->NotifyAbilityCommit(this);
	return true;
}

const FGameplayTag& UPRGameplayAbility::GetProjectRAbilityTag() const
{
	return AbilityTag;
}

EPRAbilityActivationPolicy UPRGameplayAbility::GetActivationPolicy() const
{
	return ActivationPolicy;
}

bool UPRGameplayAbility::CanActivateWhileDead() const
{
	return bCanActivateWhileDead;
}

bool UPRGameplayAbility::ShouldCancelOnDeath() const
{
	return bCancelOnDeath;
}
