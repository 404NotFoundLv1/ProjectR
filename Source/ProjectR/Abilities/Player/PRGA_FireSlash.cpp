// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_FireSlash.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "AbilitySystemInterface.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRTagLibrary.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectR.h"

namespace PRFireSlash
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
			// The formal side-view mesh encodes +X/-X facing as relative yaw -90/+90.
			// Its component right axis is therefore the visual attack direction.
			Aim = Mesh->GetRightVector();
		}
	}
	Aim.Y = 0.0f;
	return Aim.GetSafeNormal();
}

bool QueryFireTargets(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UPRPlayerSkillDataAsset& Data,
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
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::ForwardArea;
	Query.Origin = Avatar->GetActorLocation();
	Query.AimDirection = ResolvePlanarAim(*Avatar);
	Query.AreaCenter = Query.Origin;
	Query.Range = Data.Range;
	Query.HalfAngleDegrees = Data.HalfAngleDegrees;
	return Subsystem->QueryTargets(Query, OutResult, OutFailureTag);
}
} // namespace PRFireSlash

UPRGA_FireSlash::UPRGA_FireSlash()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_FireSlash::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		UE_LOG(LogProjectR, Verbose, TEXT("FireSlash activation was rejected by the GAS base contract."));
		return false;
	}
	const UPRPlayerSkillDataAsset* Data = PRFireSlash::ResolveData(Handle, ActorInfo);
	FPRAbilityTargetQueryResult Result;
	FGameplayTag FailureTag;
	if (!Data || Data->TargetPolicy != EPRPlayerSkillTargetPolicy::ForwardArea
		|| !PRFireSlash::QueryFireTargets(ActorInfo, *Data, Result, FailureTag))
	{
		UE_LOG(LogProjectR, Verbose,
			TEXT("FireSlash target preflight failed. Data=%s Policy=%d Failure=%s"),
			*GetNameSafe(Data),
			Data ? static_cast<int32>(Data->TargetPolicy) : INDEX_NONE,
			*FailureTag.ToString());
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

void UPRGA_FireSlash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	if (!Data || !Component || !ResolveBurningEffectClass())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	PhaseExpiredHandle = Component->OnPhaseExpired().AddUObject(
		this,
		&UPRGA_FireSlash::HandlePhaseExpired);
	if (!Component->BeginPhase(Data->AbilityTag, EPRPlayerSkillPhase::Startup, Data->StartupDuration))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UPRGA_FireSlash::EndAbility(
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

void UPRGA_FireSlash::HandlePhaseExpired(
	const FGameplayTag ExpiredAbilityTag,
	const EPRPlayerSkillPhase ExpiredPhase)
{
	if (ExpiredAbilityTag != AbilityTag || bEnding)
	{
		return;
	}
	if (ExpiredPhase == EPRPlayerSkillPhase::Startup)
	{
		HandleStartupExpired();
	}
	else if (ExpiredPhase == EPRPlayerSkillPhase::Recovery)
	{
		EndCurrentAbility(false);
	}
}

void UPRGA_FireSlash::HandleStartupExpired()
{
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	const UPRFireSlashSkillFragment* Fragment = Data
		? Cast<UPRFireSlashSkillFragment>(Data->SkillFragment)
		: nullptr;
	FPRAbilityTargetQueryResult Result;
	FGameplayTag FailureTag;
	const TSubclassOf<UGameplayEffect> ResolvedBurningEffectClass = ResolveBurningEffectClass();
	if (!Data || !Component || !Avatar || !Subsystem || !Fragment || !ResolvedBurningEffectClass
		|| !PRFireSlash::QueryFireTargets(ActorInfo, *Data, Result, FailureTag)
		|| !PrepareExecutionSnapshot(
			Result,
			Avatar->GetActorLocation(),
			PRFireSlash::ResolvePlanarAim(*Avatar),
			false,
			FailureTag)
		|| !CommitAbility(
			GetCurrentAbilitySpecHandle(),
			ActorInfo,
			GetCurrentActivationInfo()))
	{
		EndCurrentAbility(true);
		return;
	}

	Component->BeginPhase(Data->AbilityTag, EPRPlayerSkillPhase::Active, 0.0f);
	const FPRPlayerSkillExecutionSnapshot* Snapshot = GetExecutionSnapshot();
	if (!Snapshot)
	{
		EndCurrentAbility(true);
		return;
	}
	for (const TWeakObjectPtr<AActor>& TargetPtr : Snapshot->Targets)
	{
		AActor* Target = TargetPtr.Get();
		if (!IsValid(Target))
		{
			continue;
		}
		const EPRCombatRequestStatus Status = Subsystem->ApplySkillDamage(
			*Snapshot,
			*Target,
			*this,
			Snapshot->Origin,
			Snapshot->BaseDamage,
			Snapshot->AttackPowerScale,
			Snapshot->bCritical,
			Snapshot->AimDirection);
		const IAbilitySystemInterface* TargetAbilityInterface = Cast<IAbilitySystemInterface>(Target);
		const UAbilitySystemComponent* TargetASC = TargetAbilityInterface
			? TargetAbilityInterface->GetAbilitySystemComponent()
			: nullptr;
		if (Status == EPRCombatRequestStatus::Applied && TargetASC
			&& !TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
		{
			Subsystem->ApplyOrRefreshBurning(
				*Snapshot,
				*Target,
				*this,
				ResolvedBurningEffectClass,
				*Fragment);
		}
	}
	EnterRecovery();
}

void UPRGA_FireSlash::EnterRecovery()
{
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	if (!Data || !Component
		|| !Component->BeginPhase(
			Data->AbilityTag,
			EPRPlayerSkillPhase::Recovery,
			Data->RecoveryDuration))
	{
		EndCurrentAbility(true);
	}
}

void UPRGA_FireSlash::EndCurrentAbility(const bool bWasCancelled)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo)
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			ActorInfo,
			GetCurrentActivationInfo(),
			true,
			bWasCancelled);
	}
}

void UPRGA_FireSlash::UnbindPhaseDelegate()
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

TSubclassOf<UGameplayEffect> UPRGA_FireSlash::ResolveBurningEffectClass() const
{
	if (BurningEffectClass)
	{
		return BurningEffectClass;
	}
	const UPRGA_FireSlash* ClassDefault = GetClass()->GetDefaultObject<UPRGA_FireSlash>();
	return ClassDefault && ClassDefault != this
		? ClassDefault->BurningEffectClass
		: nullptr;
}
