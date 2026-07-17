// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyAttackGameplayAbility.h"

#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UPREnemyAttackGameplayAbility::UPREnemyAttackGameplayAbility()
{
	ActivationPolicy = EPRAbilityActivationPolicy::ServerTriggered;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Stunned")));
}

bool UPREnemyAttackGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
		|| !ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return false;
	}
	const APREnemyCharacter* Enemy = Cast<APREnemyCharacter>(ActorInfo->AvatarActor.Get());
	const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent.IsValid()
		? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
	return Enemy && Enemy->IsEnemyInitialized() && Spec && Cast<UPREnemyAttackDataAsset>(Spec->SourceObject) != nullptr;
}

void UPREnemyAttackGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*)
{
	APREnemyCharacter* Enemy = ActorInfo ? Cast<APREnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const FGameplayAbilitySpec* Spec = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
	const UPREnemyAttackDataAsset* Attack = Spec ? Cast<UPREnemyAttackDataAsset>(Spec->SourceObject) : nullptr;
	UPREnemyBrainComponent* Brain = Enemy ? Enemy->GetEnemyBrain() : nullptr;
	AActor* Target = Brain ? Brain->GetRuntimeState().Target.Get() : nullptr;
	if (!Enemy || !Attack || !Brain || !Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ActiveBrain = Brain;
	if (!Brain->BeginAttack(Attack, Target))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UAbilityTask_WaitDelay* TimelineTask = UAbilityTask_WaitDelay::WaitDelay(this, Attack->Windup + Attack->ActiveWindow + Attack->Recovery);
	TimelineTask->OnFinish.AddDynamic(this, &UPREnemyAttackGameplayAbility::HandleTimelineFinished);
	TimelineTask->ReadyForActivation();
}

void UPREnemyAttackGameplayAbility::HandleTimelineFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UPREnemyAttackGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (bWasCancelled && ActiveBrain.IsValid())
	{
		ActiveBrain->CancelAttack();
	}
	ActiveBrain.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
