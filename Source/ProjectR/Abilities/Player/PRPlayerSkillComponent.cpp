// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRPlayerSkillComponent.h"

#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/PRCombatTypes.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Core/PRTagLibrary.h"
#include "TimerManager.h"

UPRPlayerSkillComponent::UPRPlayerSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPRPlayerSkillComponent::BeginPhase(
	const FGameplayTag AbilityTag,
	const EPRPlayerSkillPhase Phase,
	const float Duration)
{
	if (!AbilityTag.IsValid() || !AbilityTag.ToString().StartsWith(TEXT("Skill."))
		|| Phase == EPRPlayerSkillPhase::Idle
		|| !FMath::IsFinite(Duration)
		|| Duration < 0.0f
		|| (CurrentAbilityTag.IsValid() && CurrentAbilityTag != AbilityTag))
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}
	CurrentAbilityTag = AbilityTag;
	CurrentPhase = Phase;
	if (Duration > 0.0f)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			ClearExecution();
			return false;
		}
		World->GetTimerManager().SetTimer(
			PhaseTimerHandle,
			this,
			&UPRPlayerSkillComponent::HandlePhaseExpired,
			Duration,
			false);
	}
	return true;
}

void UPRPlayerSkillComponent::CancelExecution()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}
	CancelOwnedDisplacement();
	if (CurrentAbilityTag.IsValid())
	{
		CurrentPhase = EPRPlayerSkillPhase::Cancelled;
	}
}

void UPRPlayerSkillComponent::ClearExecution()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
	}
	CancelOwnedDisplacement();
	ClearCounterProofGuard(CurrentAbilityTag);
	CurrentAbilityTag = FGameplayTag();
	CurrentPhase = EPRPlayerSkillPhase::Idle;
}

FGameplayTag UPRPlayerSkillComponent::GetCurrentAbilityTag() const
{
	return CurrentAbilityTag;
}

EPRPlayerSkillPhase UPRPlayerSkillComponent::GetCurrentPhase() const
{
	return CurrentPhase;
}

FGuid UPRPlayerSkillComponent::GetActiveDisplacementRequestId() const
{
	return ActiveDisplacementRequestId;
}

FPRPlayerSkillPhaseExpiredNative& UPRPlayerSkillComponent::OnPhaseExpired()
{
	return PhaseExpiredEvent;
}

bool UPRPlayerSkillComponent::RequestOwnedDisplacement(
	const FPRAbilityDisplacementRequest& Request,
	FGameplayTag& OutFailureTag)
{
	OutFailureTag = FGameplayTag();
	if (ActiveDisplacementRequestId.IsValid() || !GetWorld())
	{
		return false;
	}
	UPRPlayerSkillSubsystem* Subsystem = GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>();
	if (!Subsystem)
	{
		return false;
	}
	EnsureSubsystemBinding();
	return Subsystem->RequestDisplacement(Request, ActiveDisplacementRequestId, OutFailureTag);
}

void UPRPlayerSkillComponent::CancelOwnedDisplacement()
{
	if (!ActiveDisplacementRequestId.IsValid())
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (UPRPlayerSkillSubsystem* Subsystem = World->GetSubsystem<UPRPlayerSkillSubsystem>())
		{
			Subsystem->CancelDisplacement(ActiveDisplacementRequestId);
		}
	}
	ActiveDisplacementRequestId.Invalidate();
}

EPRCombatMitigationResult UPRPlayerSkillComponent::EvaluateIncomingDamage(
	const FPRDamageRequest& Request,
	FGameplayTagContainer& OutResponseTags) const
{
	if (!CounterProofGuard.bActive
		|| CurrentAbilityTag != CounterProofGuard.AbilityTag
		|| CurrentPhase != EPRPlayerSkillPhase::Active
		|| !FMath::IsFinite(Request.IncomingDirection.X)
		|| !FMath::IsFinite(Request.IncomingDirection.Y)
		|| !FMath::IsFinite(Request.IncomingDirection.Z))
	{
		return EPRCombatMitigationResult::NotHandled;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(GetOwner());
	const UAbilitySystemComponent* ASC = AbilityInterface
		? AbilityInterface->GetAbilitySystemComponent()
		: nullptr;
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Character || !ASC || ASC->GetAvatarActor() != GetOwner()
		|| !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()) || !Mesh)
	{
		return EPRCombatMitigationResult::NotHandled;
	}

	FVector Facing = Mesh->GetRightVector();
	Facing.Y = 0.0f;
	FVector AttackDirection = -Request.IncomingDirection;
	AttackDirection.Y = 0.0f;
	if (!Facing.Normalize() || !AttackDirection.Normalize())
	{
		return EPRCombatMitigationResult::NotHandled;
	}

	const float MinimumFrontDot = FMath::Cos(FMath::DegreesToRadians(CounterProofGuard.HalfAngleDegrees));
	if (FVector::DotProduct(Facing, AttackDirection) + UE_KINDA_SMALL_NUMBER < MinimumFrontDot)
	{
		return EPRCombatMitigationResult::NotHandled;
	}

	OutResponseTags.AddTag(UPRTagLibrary::GetCombatResponseGuardBlockedTag());
	if (const UWorld* World = GetWorld(); World
		&& World->GetTimeSeconds() <= CounterProofGuard.PerfectTimingEndTime + UE_KINDA_SMALL_NUMBER)
	{
		OutResponseTags.AddTag(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
	}
	return EPRCombatMitigationResult::Blocked;
}

void UPRPlayerSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearExecution();
	if (DisplacementFinishedHandle.IsValid() && GetWorld())
	{
		if (UPRPlayerSkillSubsystem* Subsystem = GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>())
		{
			Subsystem->OnDisplacementFinished().Remove(DisplacementFinishedHandle);
		}
	}
	DisplacementFinishedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRPlayerSkillComponent::HandlePhaseExpired()
{
	const FGameplayTag ExpiredAbilityTag = CurrentAbilityTag;
	const EPRPlayerSkillPhase ExpiredPhase = CurrentPhase;
	PhaseTimerHandle.Invalidate();
	PhaseExpiredEvent.Broadcast(ExpiredAbilityTag, ExpiredPhase);
}

void UPRPlayerSkillComponent::HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result)
{
	if (Result.RequestId == ActiveDisplacementRequestId)
	{
		ActiveDisplacementRequestId.Invalidate();
	}
}

void UPRPlayerSkillComponent::EnsureSubsystemBinding()
{
	if (DisplacementFinishedHandle.IsValid() || !GetWorld())
	{
		return;
	}
	if (UPRPlayerSkillSubsystem* Subsystem = GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>())
	{
		DisplacementFinishedHandle = Subsystem->OnDisplacementFinished().AddUObject(
			this,
			&UPRPlayerSkillComponent::HandleDisplacementFinished);
	}
}

bool UPRPlayerSkillComponent::BeginCounterProofGuard(
	const FGameplayTag AbilityTag,
	const float HalfAngleDegrees,
	const double PerfectTimingEndTime)
{
	if (!AbilityTag.IsValid() || CurrentAbilityTag != AbilityTag
		|| CurrentPhase != EPRPlayerSkillPhase::Active
		|| !FMath::IsFinite(HalfAngleDegrees) || HalfAngleDegrees < 0.0f || HalfAngleDegrees > 180.0f
		|| !FMath::IsFinite(PerfectTimingEndTime))
	{
		return false;
	}
	CounterProofGuard.AbilityTag = AbilityTag;
	CounterProofGuard.HalfAngleDegrees = HalfAngleDegrees;
	CounterProofGuard.PerfectTimingEndTime = PerfectTimingEndTime;
	CounterProofGuard.bActive = true;
	return true;
}

void UPRPlayerSkillComponent::ClearCounterProofGuard(const FGameplayTag AbilityTag)
{
	if (!CounterProofGuard.bActive || CounterProofGuard.AbilityTag != AbilityTag)
	{
		return;
	}
	CounterProofGuard = FCounterProofGuardContext();
}
