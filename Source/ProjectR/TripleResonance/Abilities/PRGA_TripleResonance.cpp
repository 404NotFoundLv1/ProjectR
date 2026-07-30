// Copyright ProjectR. All Rights Reserved.

#include "TripleResonance/Abilities/PRGA_TripleResonance.h"

#include "AbilitySystemComponent.h"
#include "Core/PRTagLibrary.h"
#include "Engine/GameInstance.h"
#include "GameplayEffect.h"
#include "TripleResonance/PRTripleResonanceSubsystem.h"

UPRGA_TripleResonance::UPRGA_TripleResonance()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.TripleResonance"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_TripleResonance::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
	UPRTripleResonanceSubsystem* Triple = ActorInfo && ActorInfo->AvatarActor.IsValid() && ActorInfo->AvatarActor->GetWorld()
		? ActorInfo->AvatarActor->GetWorld()->GetGameInstance()->GetSubsystem<UPRTripleResonanceSubsystem>() : nullptr;
	return Triple && Triple->CanExecuteGrantedAbility(ActorInfo->AvatarActor.Get());
}

void UPRGA_TripleResonance::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData*)
{
	UPRTripleResonanceSubsystem* Triple = ActorInfo && ActorInfo->AvatarActor.IsValid() && ActorInfo->AvatarActor->GetWorld()
		? ActorInfo->AvatarActor->GetWorld()->GetGameInstance()->GetSubsystem<UPRTripleResonanceSubsystem>() : nullptr;
	if (!Triple || !ResonanceChargeEffect || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FActiveGameplayEffectHandle ChargeHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo,
		ResonanceChargeEffect->GetDefaultObject<UGameplayEffect>(), 1.0f);
	if (!ChargeHandle.IsValid() || !CommitAbility(Handle, ActorInfo, ActivationInfo) || !Triple->ExecuteGrantedAbility(ActorInfo->AvatarActor.Get()))
	{
		if (ChargeHandle.IsValid()) ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ChargeHandle);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ChargeHandle);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UGameplayEffect* UPRGA_TripleResonance::GetCostGameplayEffect() const
{
	return ResonanceCostEffect ? ResonanceCostEffect->GetDefaultObject<UGameplayEffect>() : nullptr;
}

UGameplayEffect* UPRGA_TripleResonance::GetCooldownGameplayEffect() const
{
	return ResonanceCooldownEffect ? ResonanceCooldownEffect->GetDefaultObject<UGameplayEffect>() : nullptr;
}

const FGameplayTagContainer* UPRGA_TripleResonance::GetCooldownTags() const
{
	static FGameplayTagContainer Tags;
	Tags.Reset();
	if (const FGameplayTagContainer* ParentTags = Super::GetCooldownTags()) Tags.AppendTags(*ParentTags);
	Tags.AddTag(UPRTagLibrary::GetCooldownSkillTripleResonanceTag());
	return &Tags;
}
