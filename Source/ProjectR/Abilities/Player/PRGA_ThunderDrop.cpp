// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_ThunderDrop.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/PRTagLibrary.h"
#include "GameFramework/Character.h"

namespace PRThunderDrop
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

FVector ResolvePlanarAim(const AActor& Avatar)
{
	FVector Aim = Avatar.GetActorForwardVector();
	if (const ACharacter* Character = Cast<ACharacter>(&Avatar))
	{
		if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Aim = Mesh->GetRightVector();
		}
	}
	Aim.Y = 0.0f;
	return Aim.GetSafeNormal();
}

bool ValidateImpact(
	UPRPlayerSkillSubsystem& Subsystem,
	AActor& Avatar,
	const UPRPlayerSkillDataAsset& Data,
	const FVector Aim,
	const FVector ImpactPoint,
	FPRAbilityTargetQueryResult& OutResult,
	FGameplayTag& OutFailureTag)
{
	FPRAbilityTargetQuery Query;
	Query.SourceActor = &Avatar;
	Query.AbilityTag = Data.AbilityTag;
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::GroundArea;
	Query.Origin = Avatar.GetActorLocation();
	Query.AimDirection = Aim;
	Query.AreaCenter = ImpactPoint;
	Query.Range = Data.Range;
	Query.Radius = Data.Radius;
	return Subsystem.QueryTargets(Query, OutResult, OutFailureTag);
}

bool SelectImpact(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UPRPlayerSkillDataAsset& Data,
	FVector& OutImpactPoint,
	FPRAbilityTargetQueryResult& OutResult,
	FGameplayTag& OutFailureTag)
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	const UPRThunderDropSkillFragment* Fragment = Cast<UPRThunderDropSkillFragment>(Data.SkillFragment);
	if (!Avatar || !Subsystem || !Fragment || !FMath::IsFinite(Fragment->FallbackDistance)
		|| Fragment->FallbackDistance <= 0.0f)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	const FVector Aim = ResolvePlanarAim(*Avatar);
	if (Aim.IsNearlyZero())
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}
	FPRAbilityTargetQuery Query;
	Query.SourceActor = Avatar;
	Query.AbilityTag = Data.AbilityTag;
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::GroundArea;
	Query.Origin = Avatar->GetActorLocation();
	Query.AimDirection = Aim;
	Query.AreaCenter = Query.Origin;
	Query.Range = Data.Range;
	Query.Radius = Data.Range;
	FPRAbilityTargetQueryResult Candidates;
	FGameplayTag CandidateFailure;
	if (Subsystem->QueryTargets(Query, Candidates, CandidateFailure) && !Candidates.Targets.IsEmpty())
	{
		if (AActor* Target = Candidates.Targets[0].Get())
		{
			if (const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(Target))
			{
				const FVector TargetPoint = TargetInterface->GetAbilityTargetPoint();
				if (ValidateImpact(*Subsystem, *Avatar, Data, Aim, TargetPoint, OutResult, OutFailureTag))
				{
					OutImpactPoint = OutResult.ResolvedPoint;
					return true;
				}
			}
		}
	}

	const FVector FallbackPoint = Avatar->GetActorLocation() + Aim * Fragment->FallbackDistance;
	if (ValidateImpact(*Subsystem, *Avatar, Data, Aim, FallbackPoint, OutResult, OutFailureTag))
	{
		OutImpactPoint = OutResult.ResolvedPoint;
		return true;
	}
	return false;
}
} // namespace PRThunderDrop

UPRGA_ThunderDrop::UPRGA_ThunderDrop()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_ThunderDrop::CanActivateAbility(
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
	const UPRPlayerSkillDataAsset* Data = PRThunderDrop::ResolveData(Handle, ActorInfo);
	FPRAbilityTargetQueryResult Result;
	FGameplayTag FailureTag;
	FVector ImpactPoint;
	if (!Data || Data->TargetPolicy != EPRPlayerSkillTargetPolicy::GroundArea
		|| !ResolveStunnedEffectClass()
		|| !PRThunderDrop::SelectImpact(ActorInfo, *Data, ImpactPoint, Result, FailureTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(FailureTag.IsValid()
				? FailureTag
				: UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag());
		}
		return false;
	}
	return true;
}

void UPRGA_ThunderDrop::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	FPRAbilityTargetQueryResult Result;
	FGameplayTag FailureTag;
	FVector ImpactPoint;
	if (!Data || !Component || !Avatar || !Subsystem || !ResolveStunnedEffectClass()
		|| !PRThunderDrop::SelectImpact(ActorInfo, *Data, ImpactPoint, Result, FailureTag)
		|| !PrepareExecutionSnapshot(
			Result,
			Avatar->GetActorLocation(),
			PRThunderDrop::ResolvePlanarAim(*Avatar),
			false,
			FailureTag)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo)
		|| !Subsystem->ScheduleThunderDrop(
			*GetExecutionSnapshot(),
			ImpactPoint,
			*const_cast<UPRPlayerSkillDataAsset*>(Data),
			ResolveStunnedEffectClass()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	PhaseExpiredHandle = Component->OnPhaseExpired().AddUObject(this, &UPRGA_ThunderDrop::HandlePhaseExpired);
	if (!Component->BeginPhase(Data->AbilityTag, EPRPlayerSkillPhase::Active, Data->ActiveDuration))
	{
		EndCurrentAbility(true);
	}
}

void UPRGA_ThunderDrop::EndAbility(
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
	UnbindPhaseDelegate();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bEnding = false;
}

void UPRGA_ThunderDrop::HandlePhaseExpired(
	const FGameplayTag Tag,
	const EPRPlayerSkillPhase Phase)
{
	if (!bEnding && Tag == AbilityTag && Phase == EPRPlayerSkillPhase::Active)
	{
		EndCurrentAbility(false);
	}
}

void UPRGA_ThunderDrop::EndCurrentAbility(const bool bWasCancelled)
{
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, bWasCancelled);
	}
}

void UPRGA_ThunderDrop::UnbindPhaseDelegate()
{
	if (PhaseExpiredHandle.IsValid())
	{
		if (UPRPlayerSkillComponent* Component = GetPlayerSkillComponent())
		{
			Component->OnPhaseExpired().Remove(PhaseExpiredHandle);
		}
		PhaseExpiredHandle.Reset();
	}
}

TSubclassOf<UGameplayEffect> UPRGA_ThunderDrop::ResolveStunnedEffectClass() const
{
	if (StunnedEffectClass)
	{
		return StunnedEffectClass;
	}
	const UPRGA_ThunderDrop* ClassDefault = GetClass()->GetDefaultObject<UPRGA_ThunderDrop>();
	return ClassDefault && ClassDefault != this ? ClassDefault->StunnedEffectClass : nullptr;
}
