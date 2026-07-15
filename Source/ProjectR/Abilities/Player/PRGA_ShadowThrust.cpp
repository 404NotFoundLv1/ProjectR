// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_ShadowThrust.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "AbilitySystemInterface.h"
#include "Combat/PRCombatTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

namespace PRShadowThrust
{
constexpr float ScanInterval = 1.0f / 120.0f;

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

bool QueryShadowTargets(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UPRPlayerSkillDataAsset& Data,
	const float Range,
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
	Query.AimDirection = ResolvePlanarAim(*Avatar);
	Query.AreaCenter = Query.Origin;
	Query.Range = Range;
	Query.Radius = Data.Radius;
	return Subsystem->QueryTargets(Query, OutResult, OutFailureTag);
}
} // namespace PRShadowThrust

UPRGA_ShadowThrust::UPRGA_ShadowThrust()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_ShadowThrust::CanActivateAbility(
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
	const UPRPlayerSkillDataAsset* Data = PRShadowThrust::ResolveData(Handle, ActorInfo);
	FPRAbilityTargetQueryResult Result;
	FGameplayTag FailureTag;
	if (!Data || Data->TargetPolicy != EPRPlayerSkillTargetPolicy::ForwardSweep
		|| !PRShadowThrust::QueryShadowTargets(ActorInfo, *Data, Data->Range, Result, FailureTag))
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

void UPRGA_ShadowThrust::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	bFinishingActive = false;
	bResolvedPathObstructed = false;
	LockedTargets.Reset();
	HitTargetIds.Reset();
	DisplacementRequestId.Invalidate();
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	if (!Data || !Component)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	PhaseExpiredHandle = Component->OnPhaseExpired().AddUObject(
		this,
		&UPRGA_ShadowThrust::HandlePhaseExpired);
	if (!Component->BeginPhase(Data->AbilityTag, EPRPlayerSkillPhase::Startup, Data->StartupDuration))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UPRGA_ShadowThrust::EndAbility(
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
	ClearRuntimeBindings();
	LockedTargets.Reset();
	HitTargetIds.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	DisplacementRequestId.Invalidate();
	bResolvedPathObstructed = false;
	bFinishingActive = false;
	bEnding = false;
}

void UPRGA_ShadowThrust::HandlePhaseExpired(
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
	else if (ExpiredPhase == EPRPlayerSkillPhase::Active)
	{
		ScanCurrentPath();
	}
	else if (ExpiredPhase == EPRPlayerSkillPhase::Recovery)
	{
		EndCurrentAbility(false);
	}
}

void UPRGA_ShadowThrust::HandleStartupExpired()
{
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	ACharacter* Avatar = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	FVector EndLocation;
	float SafeDistance = 0.0f;
	FGameplayTag FailureTag;
	const FVector Aim = Avatar ? PRShadowThrust::ResolvePlanarAim(*Avatar) : FVector::ZeroVector;
	FPRAbilityTargetQueryResult Result;
	if (!Data || !Component || !Avatar || !Subsystem
		|| !Subsystem->ResolveShadowPath(
			*Avatar,
			Aim,
			Data->DisplacementDistance,
			Data->Radius,
			EndLocation,
			SafeDistance,
			FailureTag)
		|| !PRShadowThrust::QueryShadowTargets(ActorInfo, *Data, SafeDistance, Result, FailureTag)
		|| !PrepareExecutionSnapshot(
			Result,
			Avatar->GetActorLocation(),
			Aim,
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
	bResolvedPathObstructed = SafeDistance + UE_KINDA_SMALL_NUMBER < Data->DisplacementDistance;

	for (const TWeakObjectPtr<AActor>& TargetPtr : Result.Targets)
	{
		AActor* Target = TargetPtr.Get();
		const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(Target);
		if (IsValid(Target) && TargetInterface)
		{
			LockedTargets.Add(Target, TargetInterface->GetAbilityTargetId());
		}
	}
	DisplacementFinishedHandle = Subsystem->OnDisplacementFinished().AddUObject(
		this,
		&UPRGA_ShadowThrust::HandleDisplacementFinished);
	FPRAbilityDisplacementRequest Displacement;
	Displacement.SourceActor = Avatar;
	Displacement.TargetActor = Avatar;
	Displacement.StartLocation = Avatar->GetActorLocation();
	Displacement.DesiredEndLocation = EndLocation;
	Displacement.Duration = Data->ActiveDuration;
	Displacement.StopDistance = 0.0f;
	Displacement.bSweep = true;
	Displacement.bStopOnBlockingHit = true;
	Displacement.AbilityTag = Data->AbilityTag;
	if (!Component->RequestOwnedDisplacement(Displacement, FailureTag))
	{
		EndCurrentAbility(true);
		return;
	}
	DisplacementRequestId = Component->GetActiveDisplacementRequestId();
	if (!DisplacementRequestId.IsValid()
		|| !Component->BeginPhase(
			Data->AbilityTag,
			EPRPlayerSkillPhase::Active,
			Data->ActiveDuration))
	{
		EndCurrentAbility(true);
		return;
	}
	PreviousScanLocation = Avatar->GetActorLocation();
	ScanCurrentPath();
	Avatar->GetWorldTimerManager().SetTimer(
		ScanTimerHandle,
		this,
		&UPRGA_ShadowThrust::ScanCurrentPath,
		PRShadowThrust::ScanInterval,
		true);
}

void UPRGA_ShadowThrust::HandleDisplacementFinished(
	const FPRAbilityDisplacementResult& Result)
{
	if (!bEnding && Result.RequestId == DisplacementRequestId
		&& Result.AbilityTag == AbilityTag)
	{
		const EPRAbilityDisplacementEndReason SkillEndReason =
			Result.EndReason == EPRAbilityDisplacementEndReason::Completed && bResolvedPathObstructed
			? EPRAbilityDisplacementEndReason::Blocked
			: Result.EndReason;
		FinishActive(SkillEndReason);
	}
}

void UPRGA_ShadowThrust::ScanCurrentPath()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar || bEnding)
	{
		return;
	}
	const FVector Current = Avatar->GetActorLocation();
	ScanSegment(PreviousScanLocation, Current);
	PreviousScanLocation = Current;
}

void UPRGA_ShadowThrust::ScanSegment(FVector SegmentStart, const FVector SegmentEnd)
{
	const FPRPlayerSkillExecutionSnapshot* Snapshot = GetExecutionSnapshot();
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	AActor* Avatar = Snapshot ? Snapshot->SourceActor.Get() : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	if (!Snapshot || !Data || !Avatar || !Subsystem)
	{
		return;
	}
	FVector SegmentDirection = SegmentEnd - SegmentStart;
	SegmentDirection.Y = 0.0f;
	const float SegmentDistance = SegmentDirection.Size();
	if (!SegmentDirection.Normalize())
	{
		SegmentDirection = Snapshot->AimDirection;
	}
	FPRAbilityTargetQuery Query;
	Query.SourceActor = Avatar;
	Query.AbilityTag = Data->AbilityTag;
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::ForwardSweep;
	Query.Origin = SegmentStart;
	Query.AimDirection = SegmentDirection;
	Query.AreaCenter = SegmentStart;
	Query.Range = FMath::Max(SegmentDistance, 1.0f);
	Query.Radius = Data->Radius;
	FPRAbilityTargetQueryResult SegmentResult;
	FGameplayTag FailureTag;
	if (!Subsystem->QueryTargets(Query, SegmentResult, FailureTag))
	{
		return;
	}
	for (const TWeakObjectPtr<AActor>& TargetPtr : SegmentResult.Targets)
	{
		AActor* Target = TargetPtr.Get();
		const FName* LockedId = LockedTargets.Find(TargetPtr);
		const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(Target);
		if (!IsValid(Target) || !LockedId || !TargetInterface
			|| *LockedId != TargetInterface->GetAbilityTargetId()
			|| HitTargetIds.Contains(*LockedId))
		{
			continue;
		}
		HitTargetIds.Add(*LockedId);
		Subsystem->ApplySkillDamage(
			*Snapshot,
			*Target,
			*this,
			SegmentStart,
			Snapshot->BaseDamage,
			Snapshot->AttackPowerScale,
			Snapshot->bCritical,
			Snapshot->AimDirection);
	}
}

void UPRGA_ShadowThrust::FinishActive(const EPRAbilityDisplacementEndReason EndReason)
{
	if (bFinishingActive || bEnding)
	{
		return;
	}
	bFinishingActive = true;
	ScanCurrentPath();
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		if (AActor* Avatar = ActorInfo->AvatarActor.Get())
		{
			Avatar->GetWorldTimerManager().ClearTimer(ScanTimerHandle);
		}
	}
	DisplacementRequestId.Invalidate();
	if (EndReason == EPRAbilityDisplacementEndReason::Cancelled)
	{
		EndCurrentAbility(true);
		return;
	}
	if (EndReason == EPRAbilityDisplacementEndReason::Completed)
	{
		PublishCompletedOutcome();
	}
	EnterRecovery();
}

void UPRGA_ShadowThrust::EnterRecovery()
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

void UPRGA_ShadowThrust::PublishCompletedOutcome()
{
	const FPRPlayerSkillExecutionSnapshot* Snapshot = GetExecutionSnapshot();
	AActor* Avatar = Snapshot ? Snapshot->SourceActor.Get() : nullptr;
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Avatar);
	const UAbilitySystemComponent* ASC = AbilityInterface
		? AbilityInterface->GetAbilitySystemComponent()
		: nullptr;
	AActor* Owner = ASC ? ASC->GetOwnerActor() : nullptr;
	const IPRCombatantInterface* Combatant = Cast<IPRCombatantInterface>(Owner);
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	if (!Snapshot || !Avatar || !Owner || !Combatant || !Subsystem)
	{
		return;
	}
	FPRCombatOutcomeRequest Outcome;
	Outcome.SourceId = Combatant->GetCombatantId();
	Outcome.AbilitySource = this;
	Outcome.Instigator = Avatar;
	Outcome.Target = Owner;
	Outcome.AbilityTag = Snapshot->AbilityTag;
	Outcome.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseDisplacementAppliedTag());
	Subsystem->PublishAbilityOutcome(Outcome);
}

void UPRGA_ShadowThrust::EndCurrentAbility(const bool bWasCancelled)
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

void UPRGA_ShadowThrust::ClearRuntimeBindings()
{
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		if (AActor* Avatar = ActorInfo->AvatarActor.Get())
		{
			Avatar->GetWorldTimerManager().ClearTimer(ScanTimerHandle);
			if (UPRPlayerSkillSubsystem* Subsystem = Avatar->GetWorld()
				? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
				: nullptr)
			{
				Subsystem->OnDisplacementFinished().Remove(DisplacementFinishedHandle);
			}
		}
	}
	DisplacementFinishedHandle.Reset();
	if (PhaseExpiredHandle.IsValid())
	{
		if (UPRPlayerSkillComponent* Component = GetPlayerSkillComponent())
		{
			Component->OnPhaseExpired().Remove(PhaseExpiredHandle);
		}
		PhaseExpiredHandle.Reset();
	}
}
