// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Core/PRTagLibrary.h"

UPRPlayerSkillGameplayAbility::UPRPlayerSkillGameplayAbility()
{
	ActivationBlockedTags.AddTag(UPRTagLibrary::GetStateDeadTag());
	ActivationBlockedTags.AddTag(UPRTagLibrary::GetStateStunnedTag());
	ActivationOwnedTags.AddTag(UPRTagLibrary::GetAbilityStatePlayerSkillActiveTag());
}

bool UPRPlayerSkillGameplayAbility::CommitAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	FGameplayTagContainer* OptionalRelevantTags)
{
	if (!bSnapshotPrepared || bSnapshotCommitted)
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag());
		}
		return false;
	}
	if (!Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags))
	{
		return false;
	}
	bSnapshotCommitted = true;
	if (UPRPlayerSkillComponent* Component = GetPlayerSkillComponent())
	{
		Component->PlaySkillPresentation(GetSkillDataAsset());
	}
	return true;
}

void UPRPlayerSkillGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (UPRPlayerSkillComponent* Component = GetPlayerSkillComponent())
	{
		if (const UPRPlayerSkillDataAsset* SkillData = GetSkillDataAsset())
		{
			Component->StopSkillPresentation(SkillData->AbilityTag);
		}
		if (bWasCancelled)
		{
			Component->CancelExecution();
		}
		Component->ClearExecution();
	}
	ExecutionSnapshot = FPRPlayerSkillExecutionSnapshot();
	bSnapshotPrepared = false;
	bSnapshotCommitted = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const UPRPlayerSkillDataAsset* UPRPlayerSkillGameplayAbility::GetSkillDataAsset() const
{
	return Cast<UPRPlayerSkillDataAsset>(GetCurrentSourceObject());
}

UPRPlayerSkillComponent* UPRPlayerSkillGameplayAbility::GetPlayerSkillComponent() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return Avatar ? Avatar->FindComponentByClass<UPRPlayerSkillComponent>() : nullptr;
}

bool UPRPlayerSkillGameplayAbility::PrepareExecutionSnapshot(
	const FPRAbilityTargetQueryResult& TargetResult,
	const FVector Origin,
	const FVector AimDirection,
	const bool bCritical,
	FGameplayTag& OutFailureTag)
{
	OutFailureTag = FGameplayTag();
	const UPRPlayerSkillDataAsset* SkillData = GetSkillDataAsset();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UPRAbilitySystemComponent* ASC = ActorInfo
		? Cast<UPRAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get())
		: nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (!SkillData || !ASC || !Avatar || !Spec || bSnapshotCommitted
		|| Origin.ContainsNaN() || AimDirection.ContainsNaN())
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	FGameplayTag InputTag;
	int32 InputTagCount = 0;
	for (const FGameplayTag& Tag : Spec->GetDynamicSpecSourceTags())
	{
		if (Tag.ToString().StartsWith(TEXT("Input.")))
		{
			InputTag = Tag;
			++InputTagCount;
		}
	}
	if (InputTagCount != 1 || InputTag != SkillData->InputTag)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailNoTargetTag();
		return false;
	}

	FVector PlanarAim(AimDirection.X, 0.0f, AimDirection.Z);
	if (SkillData->TargetPolicy != EPRPlayerSkillTargetPolicy::Self
		&& !PlanarAim.Normalize())
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	ExecutionSnapshot = FPRPlayerSkillExecutionSnapshot();
	ExecutionSnapshot.AbilityTag = SkillData->AbilityTag;
	ExecutionSnapshot.InputTag = InputTag;
	ExecutionSnapshot.SourceActor = Avatar;
	ExecutionSnapshot.PrimaryTarget = TargetResult.PrimaryTarget;
	ExecutionSnapshot.Targets = TargetResult.Targets;
	ExecutionSnapshot.Origin = Origin;
	ExecutionSnapshot.AimDirection = PlanarAim;
	ExecutionSnapshot.AttackPower = ASC->GetNumericAttribute(UPRAttributeSet::GetAttackPowerAttribute());
	ExecutionSnapshot.BaseDamage = SkillData->BaseDamage;
	ExecutionSnapshot.AttackPowerScale = SkillData->AttackPowerScale;
	ExecutionSnapshot.EnergyCost = SkillData->EnergyCost;
	ExecutionSnapshot.CooldownDuration = SkillData->CooldownDuration;
	ExecutionSnapshot.bCritical = bCritical;
	bSnapshotPrepared = true;
	return true;
}

const FPRPlayerSkillExecutionSnapshot* UPRPlayerSkillGameplayAbility::GetExecutionSnapshot() const
{
	return bSnapshotPrepared ? &ExecutionSnapshot : nullptr;
}
