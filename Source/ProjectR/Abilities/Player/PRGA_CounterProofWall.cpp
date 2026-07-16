// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_CounterProofWall.h"

#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/PRTagLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"

namespace PRCounterProofWall
{
const UPRPlayerSkillDataAsset* ResolveData(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo)
{
	const FGameplayAbilitySpec* Spec = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)
		: nullptr;
	return Spec ? Cast<UPRPlayerSkillDataAsset>(Spec->SourceObject.Get()) : nullptr;
}

FVector ResolvePlanarFacing(const ACharacter& Avatar)
{
	const USkeletalMeshComponent* Mesh = Avatar.GetMesh();
	FVector Facing = Mesh ? Mesh->GetRightVector() : FVector::ZeroVector;
	Facing.Y = 0.0f;
	return Facing.GetSafeNormal();
}
} // namespace PRCounterProofWall

UPRGA_CounterProofWall::UPRGA_CounterProofWall()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.CounterProofWall"));
	ActivationPolicy = EPRAbilityActivationPolicy::WhileInputActive;
}

bool UPRGA_CounterProofWall::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	const UPRPlayerSkillDataAsset* Data = PRCounterProofWall::ResolveData(Handle, ActorInfo);
	const UPRCounterProofWallSkillFragment* Fragment = Data
		? Cast<UPRCounterProofWallSkillFragment>(Data->SkillFragment)
		: nullptr;
	const ACharacter* Avatar = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Data || !Fragment || !Avatar || !ResolveGuardingEffectClass()
		|| Data->InputTag != UPRTagLibrary::GetInputSkillCounterProofWallTag()
		|| Data->TargetPolicy != EPRPlayerSkillTargetPolicy::Self
		|| !FMath::IsFinite(Data->ActiveDuration) || Data->ActiveDuration <= 0.0f
		|| !FMath::IsFinite(Data->HalfAngleDegrees) || Data->HalfAngleDegrees < 0.0f
		|| Data->HalfAngleDegrees > 180.0f
		|| !FMath::IsFinite(Fragment->PerfectTimingWindow) || Fragment->PerfectTimingWindow < 0.0f
		|| Fragment->PerfectTimingWindow > Data->ActiveDuration
		|| PRCounterProofWall::ResolvePlanarFacing(*Avatar).IsNearlyZero())
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag());
		}
		return false;
	}
	return true;
}

void UPRGA_CounterProofWall::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	GuardingASC.Reset();
	GuardComponent.Reset();
	GuardingEffectHandle = FActiveGameplayEffectHandle();
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	const UPRCounterProofWallSkillFragment* Fragment = Data
		? Cast<UPRCounterProofWallSkillFragment>(Data->SkillFragment)
		: nullptr;
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	ACharacter* Avatar = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	FGameplayTag FailureTag;
	FPRAbilityTargetQueryResult SelfResult;
	SelfResult.PrimaryTarget = Avatar;
	SelfResult.Targets.Add(Avatar);
	SelfResult.ResolvedPoint = Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector;
	if (!Data || !Fragment || !Component || !Avatar || !ASC || !World || !ResolveGuardingEffectClass()
		|| Data->InputTag != UPRTagLibrary::GetInputSkillCounterProofWallTag()
		|| Data->TargetPolicy != EPRPlayerSkillTargetPolicy::Self
		|| !PrepareExecutionSnapshot(
			SelfResult,
			Avatar->GetActorLocation(),
			PRCounterProofWall::ResolvePlanarFacing(*Avatar),
			false,
			FailureTag)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(ResolveGuardingEffectClass(), 1.0f, EffectContext);
	if (!EffectSpec.IsValid() || !EffectSpec.Data.IsValid())
	{
		EndCurrentAbility(true);
		return;
	}
	GuardingEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	if (!GuardingEffectHandle.IsValid())
	{
		EndCurrentAbility(true);
		return;
	}
	GuardingASC = ASC;
	GuardComponent = Component;
	PhaseExpiredHandle = Component->OnPhaseExpired().AddUObject(
		this, &UPRGA_CounterProofWall::HandlePhaseExpired);
	if (!Component->BeginPhase(Data->AbilityTag, EPRPlayerSkillPhase::Active, Data->ActiveDuration)
		|| !Component->BeginCounterProofGuard(
			Data->AbilityTag,
			Data->HalfAngleDegrees,
			World->GetTimeSeconds() + Fragment->PerfectTimingWindow))
	{
		EndCurrentAbility(true);
	}
}

void UPRGA_CounterProofWall::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (bEnding)
	{
		return;
	}
	bEnding = true;
	ClearRuntimeState();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bEnding = false;
}

void UPRGA_CounterProofWall::HandlePhaseExpired(
	const FGameplayTag Tag,
	const EPRPlayerSkillPhase Phase)
{
	if (!bEnding && Tag == AbilityTag && Phase == EPRPlayerSkillPhase::Active)
	{
		EndCurrentAbility(false);
	}
}

void UPRGA_CounterProofWall::EndCurrentAbility(const bool bWasCancelled)
{
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, bWasCancelled);
	}
}

void UPRGA_CounterProofWall::ClearRuntimeState()
{
	if (UPRPlayerSkillComponent* Component = GuardComponent.Get())
	{
		if (PhaseExpiredHandle.IsValid())
		{
			Component->OnPhaseExpired().Remove(PhaseExpiredHandle);
		}
		Component->ClearCounterProofGuard(AbilityTag);
	}
	PhaseExpiredHandle.Reset();
	GuardComponent.Reset();
	if (UAbilitySystemComponent* ASC = GuardingASC.Get(); ASC && GuardingEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(GuardingEffectHandle);
	}
	GuardingEffectHandle = FActiveGameplayEffectHandle();
	GuardingASC.Reset();
}

TSubclassOf<UGameplayEffect> UPRGA_CounterProofWall::ResolveGuardingEffectClass() const
{
	if (GuardingEffectClass)
	{
		return GuardingEffectClass;
	}
	const UPRGA_CounterProofWall* ClassDefault = GetClass()->GetDefaultObject<UPRGA_CounterProofWall>();
	return ClassDefault && ClassDefault != this ? ClassDefault->GuardingEffectClass : nullptr;
}
