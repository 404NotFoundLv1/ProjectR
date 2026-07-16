// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_VectorHook.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

namespace PRVectorHook
{
constexpr float ValidationInterval = 1.0f / 120.0f;

const UPRPlayerSkillDataAsset* ResolveData(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo)
{
	const FGameplayAbilitySpec* Spec = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)
		: nullptr;
	return Spec ? Cast<UPRPlayerSkillDataAsset>(Spec->SourceObject.Get()) : nullptr;
}

FVector ResolvePlanarAim(const ACharacter& Avatar)
{
	FVector Aim = Avatar.GetActorForwardVector();
	if (const USkeletalMeshComponent* Mesh = Avatar.GetMesh())
	{
		Aim = Mesh->GetRightVector();
	}
	Aim.Y = 0.0f;
	return Aim.GetSafeNormal();
}
} // namespace PRVectorHook

UPRGA_VectorHook::UPRGA_VectorHook()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_VectorHook::CanActivateAbility(
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
	const UPRPlayerSkillDataAsset* Data = PRVectorHook::ResolveData(Handle, ActorInfo);
	FPRAbilityTargetQueryResult Targets;
	FPRAbilityDisplacementRequest Displacement;
	FGameplayTag FailureTag;
	if (!Data || Data->AbilityTag != AbilityTag
		|| Data->InputTag != FGameplayTag::RequestGameplayTag(TEXT("Input.Skill.VectorHook"))
		|| Data->ActivationPolicy != EPRAbilityActivationPolicy::OnInputTriggered
		|| Data->TargetPolicy != EPRPlayerSkillTargetPolicy::SingleTarget
		|| !ResolveHookPlan(ActorInfo, *Data, Targets, Displacement, FailureTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(FailureTag.IsValid()
				? FailureTag : UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag());
		}
		return false;
	}
	return true;
}

void UPRGA_VectorHook::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	bCompletedOutcomePublished = false;
	DisplacementRequestId.Invalidate();
	ActivationAvatar.Reset();
	LockedTarget.Reset();
	LockedTargetId = NAME_None;
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	ACharacter* Avatar = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>() : nullptr;
	FPRAbilityTargetQueryResult Targets;
	FPRAbilityDisplacementRequest Displacement;
	FGameplayTag FailureTag;
	const FVector Aim = Avatar ? PRVectorHook::ResolvePlanarAim(*Avatar) : FVector::ZeroVector;
	if (!Data || !Component || !Avatar || !Subsystem
		|| !ResolveHookPlan(ActorInfo, *Data, Targets, Displacement, FailureTag)
		|| !PrepareExecutionSnapshot(Targets, Avatar->GetActorLocation(), Aim, false, FailureTag)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	LockedTarget = Targets.PrimaryTarget;
	ActivationAvatar = Avatar;
	if (const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(LockedTarget.Get()))
	{
		LockedTargetId = TargetInterface->GetAbilityTargetId();
	}
	DisplacementFinishedHandle = Subsystem->OnDisplacementFinished().AddUObject(
		this, &UPRGA_VectorHook::HandleDisplacementFinished);
	PhaseExpiredHandle = Component->OnPhaseExpired().AddUObject(this, &UPRGA_VectorHook::HandlePhaseExpired);
	if (!Component->RequestOwnedDisplacement(Displacement, FailureTag))
	{
		EndCurrentAbility(true);
		return;
	}
	DisplacementRequestId = Component->GetActiveDisplacementRequestId();
	if (!DisplacementRequestId.IsValid()
		|| !Component->BeginPhase(Data->AbilityTag, EPRPlayerSkillPhase::Active, Data->ActiveDuration))
	{
		EndCurrentAbility(true);
		return;
	}
	Avatar->GetWorldTimerManager().SetTimer(
		ValidationTimerHandle, this, &UPRGA_VectorHook::HandleValidationTick,
		PRVectorHook::ValidationInterval, true);
}

void UPRGA_VectorHook::EndAbility(
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
	if (DisplacementRequestId.IsValid())
	{
		if (AActor* RequestAvatar = ActivationAvatar.Get())
		{
			if (UPRPlayerSkillSubsystem* Subsystem = RequestAvatar->GetWorld()
				? RequestAvatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>() : nullptr)
			{
				Subsystem->CancelDisplacement(DisplacementRequestId);
			}
		}
	}
	// Avatar replacement may already have changed CurrentActorInfo before GAS calls
	// EndAbility. Cancel through the avatar that owns this request, not through
	// the replacement avatar selected by the base-class cleanup.
	if (ACharacter* RequestAvatar = Cast<ACharacter>(ActivationAvatar.Get()))
	{
		if (UPRPlayerSkillComponent* RequestComponent =
			RequestAvatar->FindComponentByClass<UPRPlayerSkillComponent>())
		{
			RequestComponent->ClearExecution();
		}
	}
	ClearRuntimeBindings();
	DisplacementRequestId.Invalidate();
	ActivationAvatar.Reset();
	LockedTarget.Reset();
	LockedTargetId = NAME_None;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bEnding = false;
}

bool UPRGA_VectorHook::ResolveHookPlan(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UPRPlayerSkillDataAsset& Data,
	FPRAbilityTargetQueryResult& OutTargets,
	FPRAbilityDisplacementRequest& OutDisplacement,
	FGameplayTag& OutFailureTag) const
{
	OutTargets = FPRAbilityTargetQueryResult();
	OutDisplacement = FPRAbilityDisplacementRequest();
	OutFailureTag = FGameplayTag();
	ACharacter* Avatar = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>() : nullptr;
	const UPRVectorHookSkillFragment* Fragment = Cast<UPRVectorHookSkillFragment>(Data.SkillFragment);
	const FVector Aim = Avatar ? PRVectorHook::ResolvePlanarAim(*Avatar) : FVector::ZeroVector;
	if (!Avatar || !Subsystem || !Fragment || Aim.IsNearlyZero()
		|| !FMath::IsFinite(Data.Range) || !FMath::IsFinite(Data.HalfAngleDegrees)
		|| !FMath::IsFinite(Data.ActiveDuration) || !FMath::IsFinite(Fragment->StopDistance)
		|| Data.Range <= 0.0f || Data.HalfAngleDegrees < 0.0f || Data.HalfAngleDegrees > 180.0f
		|| Data.ActiveDuration <= 0.0f || Fragment->StopDistance < 0.0f)
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}

	FPRAbilityTargetQuery Query;
	Query.SourceActor = Avatar;
	Query.AbilityTag = Data.AbilityTag;
	// Query all deterministic candidates first. The DataAsset remains SingleTarget;
	// this internal pass is solely to skip pre-commit unreachable candidates.
	Query.TargetPolicy = EPRPlayerSkillTargetPolicy::ForwardArea;
	Query.Origin = Avatar->GetActorLocation();
	Query.AimDirection = Aim;
	Query.AreaCenter = Query.Origin;
	Query.Range = Data.Range;
	Query.HalfAngleDegrees = Data.HalfAngleDegrees;
	FPRAbilityTargetQueryResult Candidates;
	if (!Subsystem->QueryTargets(Query, Candidates, OutFailureTag))
	{
		return false;
	}

	for (const TWeakObjectPtr<AActor>& CandidatePtr : Candidates.Targets)
	{
		AActor* Candidate = CandidatePtr.Get();
		const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(Candidate);
		if (!IsValid(Candidate) || !TargetInterface)
		{
			continue;
		}
		ACharacter* MovedCharacter = nullptr;
		if (TargetInterface->GetAbilityTargetMobility() == EPRAbilityTargetMobility::Light)
		{
			// The approved D contract narrows Light to targets backed by the
			// existing CharacterMovement displacement infrastructure.
			MovedCharacter = Cast<ACharacter>(Candidate);
		}
		else
		{
			MovedCharacter = Avatar;
		}
		if (!MovedCharacter)
		{
			continue;
		}
		FPRAbilityDisplacementRequest CandidateRequest;
		CandidateRequest.SourceActor = Avatar;
		CandidateRequest.TargetActor = MovedCharacter;
		CandidateRequest.StartLocation = MovedCharacter->GetActorLocation();
		CandidateRequest.DesiredEndLocation = TargetInterface->GetAbilityTargetMobility() == EPRAbilityTargetMobility::Light
			? Avatar->GetActorLocation()
			: TargetInterface->GetAbilityTargetPoint();
		CandidateRequest.Duration = Data.ActiveDuration;
		CandidateRequest.StopDistance = Fragment->StopDistance;
		CandidateRequest.bSweep = true;
		CandidateRequest.bStopOnBlockingHit = true;
		CandidateRequest.AbilityTag = Data.AbilityTag;
		FVector EffectiveEnd;
		FGameplayTag CandidateFailure;
		if (!Subsystem->ValidateDisplacementRequest(CandidateRequest, EffectiveEnd, CandidateFailure))
		{
			continue;
		}
		OutTargets.PrimaryTarget = Candidate;
		OutTargets.Targets.Add(Candidate);
		OutTargets.ResolvedPoint = TargetInterface->GetAbilityTargetPoint();
		OutDisplacement = CandidateRequest;
		return true;
	}

	OutFailureTag = UPRTagLibrary::GetAbilityActivateFailObstructedTag();
	return false;
}

void UPRGA_VectorHook::HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result)
{
	if (bEnding || Result.RequestId != DisplacementRequestId || Result.AbilityTag != AbilityTag)
	{
		return;
	}
	DisplacementRequestId.Invalidate();
	if (Result.EndReason == EPRAbilityDisplacementEndReason::Completed)
	{
		PublishCompletedOutcome();
		EndCurrentAbility(false);
		return;
	}
	EndCurrentAbility(true);
}

void UPRGA_VectorHook::HandlePhaseExpired(
	const FGameplayTag ExpiredAbilityTag,
	const EPRPlayerSkillPhase ExpiredPhase)
{
	if (!bEnding && ExpiredAbilityTag == AbilityTag && ExpiredPhase == EPRPlayerSkillPhase::Active)
	{
		EndCurrentAbility(true);
	}
}

void UPRGA_VectorHook::HandleValidationTick()
{
	AActor* Target = LockedTarget.Get();
	const IPRAbilityTargetInterface* TargetInterface = Cast<IPRAbilityTargetInterface>(Target);
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	bool bValid = IsValid(Target) && TargetInterface && TargetInterface->GetAbilityTargetId() == LockedTargetId
		&& IsValid(Avatar) && Avatar == ActivationAvatar.Get();
	if (bValid)
	{
		if (const IAbilitySystemInterface* TargetAbility = Cast<IAbilitySystemInterface>(Target))
		{
			const UAbilitySystemComponent* TargetASC = TargetAbility->GetAbilitySystemComponent();
			bValid = !TargetASC || (!TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag())
				&& !TargetASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()));
		}
	}
	if (!bValid)
	{
		EndCurrentAbility(true);
	}
}

void UPRGA_VectorHook::EndCurrentAbility(const bool bWasCancelled)
{
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, bWasCancelled);
	}
}

void UPRGA_VectorHook::ClearRuntimeBindings()
{
	if (AActor* Avatar = ActivationAvatar.Get())
	{
		Avatar->GetWorldTimerManager().ClearTimer(ValidationTimerHandle);
		if (UPRPlayerSkillSubsystem* Subsystem = Avatar->GetWorld()
			? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>() : nullptr)
		{
			Subsystem->OnDisplacementFinished().Remove(DisplacementFinishedHandle);
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

void UPRGA_VectorHook::PublishCompletedOutcome()
{
	if (bCompletedOutcomePublished)
	{
		return;
	}
	const FPRPlayerSkillExecutionSnapshot* Snapshot = GetExecutionSnapshot();
	AActor* Avatar = Snapshot ? Snapshot->SourceActor.Get() : nullptr;
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Avatar);
	const UAbilitySystemComponent* ASC = AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr;
	AActor* Owner = ASC ? ASC->GetOwnerActor() : nullptr;
	const IPRCombatantInterface* Combatant = Cast<IPRCombatantInterface>(Owner);
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>() : nullptr;
	AActor* Target = LockedTarget.Get();
	if (!Snapshot || !IsValid(Avatar) || !IsValid(Target) || !Owner || !Combatant || !Subsystem)
	{
		return;
	}
	FPRCombatOutcomeRequest Outcome;
	Outcome.SourceId = Combatant->GetCombatantId();
	Outcome.AbilitySource = this;
	Outcome.Instigator = Avatar;
	Outcome.Target = Target;
	Outcome.AbilityTag = Snapshot->AbilityTag;
	Outcome.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseDisplacementAppliedTag());
	bCompletedOutcomePublished = Subsystem->PublishAbilityOutcome(Outcome);
}
