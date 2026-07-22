// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_BasicAttack.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"
#include "Abilities/Player/PRBasicAttackDataAsset.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "AbilitySystemInterface.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "GameFramework/Character.h"

namespace PRBasicAttack
{
const UPRBasicAttackDataAsset* ResolveData(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo)
{
	const FGameplayAbilitySpec* Spec = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)
		: nullptr;
	return Spec ? Cast<UPRBasicAttackDataAsset>(Spec->SourceObject.Get()) : nullptr;
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

bool QueryTargets(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UPRBasicAttackDataAsset& Data,
	const FVector AimDirection,
	FPRAbilityTargetQueryResult& OutResult,
	FGameplayTag& OutFailureTag)
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	if (!Avatar || !Subsystem)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}
	FPRAbilityTargetQuery Query;
	Query.SourceActor = Avatar;
	Query.AbilityTag = Data.AbilityTag;
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::ForwardSweep;
	Query.Origin = Avatar->GetActorLocation();
	Query.AimDirection = AimDirection;
	Query.AreaCenter = Query.Origin;
	Query.Range = Data.Range;
	Query.Radius = Data.Radius;
	return Subsystem->QueryTargets(Query, OutResult, OutFailureTag);
}
} // namespace PRBasicAttack

UPRGA_BasicAttack::UPRGA_BasicAttack()
{
	AbilityTag = UPRTagLibrary::GetSkillBasicAttackTag();
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
	ActivationBlockedTags.AddTag(UPRTagLibrary::GetStateStunnedTag());
}

bool UPRGA_BasicAttack::CanActivateAbility(
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
	const UPRBasicAttackDataAsset* Data = PRBasicAttack::ResolveData(Handle, ActorInfo);
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const FVector Aim = Avatar ? PRBasicAttack::ResolvePlanarAim(*Avatar) : FVector::ZeroVector;
	FPRAbilityTargetQueryResult Result;
	FGameplayTag FailureTag;
	if (!Data || !PRBasicAttack::QueryTargets(ActorInfo, *Data, Aim, Result, FailureTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(FailureTag.IsValid()
				? FailureTag
				: UPRTagLibrary::GetAbilityActivateFailNoTargetTag());
		}
		return false;
	}
	return true;
}

void UPRGA_BasicAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	const UPRBasicAttackDataAsset* Data = GetBasicAttackData();
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Data || !Avatar || !Avatar->GetWorld())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	LockedAimDirection = PRBasicAttack::ResolvePlanarAim(*Avatar);
	if (LockedAimDirection.IsNearlyZero())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Avatar->GetWorldTimerManager().SetTimer(
		StartupTimerHandle, this, &UPRGA_BasicAttack::HandleStartupElapsed, Data->StartupDuration, false);
}

void UPRGA_BasicAttack::EndAbility(
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
	ClearRuntimeTimers();
	LockedAimDirection = FVector::ZeroVector;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bEnding = false;
}

const UPRBasicAttackDataAsset* UPRGA_BasicAttack::GetBasicAttackData() const
{
	return Cast<UPRBasicAttackDataAsset>(GetCurrentSourceObject());
}

void UPRGA_BasicAttack::HandleStartupElapsed()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const UPRBasicAttackDataAsset* Data = GetBasicAttackData();
	UPRAbilitySystemComponent* ASC = ActorInfo
		? Cast<UPRAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get())
		: nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UPRCombatSubsystem* Combat = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRCombatSubsystem>()
		: nullptr;
	const IPRCombatantInterface* Combatant = ASC ? Cast<IPRCombatantInterface>(ASC->GetOwnerActor()) : nullptr;
	FPRAbilityTargetQueryResult Result;
	FGameplayTag FailureTag;
	if (!Data || !ASC || !Avatar || !Combat || !Combatant
		|| ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag())
		|| !PRBasicAttack::QueryTargets(ActorInfo, *Data, LockedAimDirection, Result, FailureTag)
		|| !CommitAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo()))
	{
		EndCurrentAbility(true);
		return;
	}

	const float AttackPower = ASC->GetNumericAttribute(UPRAttributeSet::GetAttackPowerAttribute());
	const float RawDamage = Data->BaseDamage + Data->AttackPowerScale * AttackPower;
	if (!FMath::IsFinite(RawDamage) || RawDamage < 0.0f)
	{
		EndCurrentAbility(true);
		return;
	}
	TSet<FName> HitTargetIds;
	for (const TWeakObjectPtr<AActor>& TargetPtr : Result.Targets)
	{
		AActor* Target = TargetPtr.Get();
		const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(Target);
		const FName TargetId = TargetInterface ? TargetInterface->GetAbilityTargetId() : NAME_None;
		if (!IsValid(Target) || !TargetInterface || TargetId.IsNone() || HitTargetIds.Contains(TargetId))
		{
			continue;
		}
		HitTargetIds.Add(TargetId);
		FPRDamageRequest Request;
		Request.SourceId = Combatant->GetCombatantId();
		Request.DamageSource = this;
		Request.Instigator = Avatar;
		Request.Target = Target;
		Request.AbilityTag = Data->AbilityTag;
		Request.RawDamage = RawDamage;
		Request.bCritical = false;
		Request.DamageTags.AddTag(Data->AbilityTag);
		Request.ImpactOrigin = Avatar->GetActorLocation();
		Request.IncomingDirection = LockedAimDirection;
		Combat->ApplyDamage(Request);
	}
	Avatar->GetWorldTimerManager().SetTimer(
		EndTimerHandle, this, &UPRGA_BasicAttack::HandleCompletionElapsed,
		Data->ActiveDuration + Data->RecoveryDuration, false, false);
}

void UPRGA_BasicAttack::HandleCompletionElapsed()
{
	EndCurrentAbility(false);
}

void UPRGA_BasicAttack::EndCurrentAbility(const bool bWasCancelled)
{
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, bWasCancelled);
	}
}

void UPRGA_BasicAttack::ClearRuntimeTimers()
{
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		if (AActor* Avatar = ActorInfo->AvatarActor.Get(); Avatar && Avatar->GetWorld())
		{
			Avatar->GetWorldTimerManager().ClearTimer(StartupTimerHandle);
			Avatar->GetWorldTimerManager().ClearTimer(EndTimerHandle);
		}
	}
}
