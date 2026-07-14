// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRPlayerSkillComponent.h"

#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "Engine/World.h"
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
	return EPRCombatMitigationResult::NotHandled;
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
