// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRGA_AfterimageDodge.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "Abilities/Player/PRSkillDecoyActor.h"
#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "GameFramework/Character.h"

namespace PRAfterimageDodge
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

FVector ResolvePlanarDodgeDirection(const ACharacter& Avatar)
{
	FVector Direction = Avatar.GetLastMovementInputVector();
	Direction.Y = 0.0f;
	if (!Direction.Normalize())
	{
		Direction = Avatar.GetActorForwardVector();
		if (const USkeletalMeshComponent* Mesh = Avatar.GetMesh())
		{
			Direction = Mesh->GetRightVector();
		}
		Direction.Y = 0.0f;
		Direction.Normalize();
	}
	return Direction;
}

} // namespace PRAfterimageDodge

UPRGA_AfterimageDodge::UPRGA_AfterimageDodge()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"));
	ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;
}

bool UPRGA_AfterimageDodge::CanActivateAbility(
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
	const UPRPlayerSkillDataAsset* Data = PRAfterimageDodge::ResolveData(Handle, ActorInfo);
	FVector Direction;
	FVector End;
	float SafeDistance = 0.0f;
	FGameplayTag FailureTag;
	if (!Data || Data->InputTag != FGameplayTag::RequestGameplayTag(TEXT("Input.Dodge"))
		|| Data->TargetPolicy != EPRPlayerSkillTargetPolicy::Self
		|| !ResolveInvulnerableEffectClass()
		|| !ResolveDodgePath(ActorInfo, *Data, Direction, End, SafeDistance, FailureTag))
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

void UPRGA_AfterimageDodge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	bResolvedPathObstructed = false;
	DisplacementRequestId.Invalidate();
	ActiveDecoy.Reset();
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	ACharacter* Avatar = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	const UPRAfterimageDodgeSkillFragment* Fragment = Data
		? Cast<UPRAfterimageDodgeSkillFragment>(Data->SkillFragment)
		: nullptr;
	FVector Direction;
	FVector End;
	float SafeDistance = 0.0f;
	FGameplayTag FailureTag;
	FPRAbilityTargetQueryResult SelfResult;
	SelfResult.PrimaryTarget = Avatar;
	SelfResult.Targets.Add(Avatar);
	SelfResult.ResolvedPoint = Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector;
	if (!Data || !Component || !Avatar || !Subsystem || !Fragment || !ResolveInvulnerableEffectClass()
		|| !ResolveDodgePath(ActorInfo, *Data, Direction, End, SafeDistance, FailureTag)
		|| !PrepareExecutionSnapshot(SelfResult, Avatar->GetActorLocation(), Direction, false, FailureTag)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Avatar;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APRSkillDecoyActor* Decoy = Avatar->GetWorld()->SpawnActor<APRSkillDecoyActor>(
		APRSkillDecoyActor::StaticClass(), Avatar->GetActorLocation(), Avatar->GetActorRotation(), SpawnParameters);
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Avatar);
	UAbilitySystemComponent* ASC = AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr;
	if (!Decoy || !ASC)
	{
		EndCurrentAbility(true);
		return;
	}
	Decoy->InitializeProxy(Avatar, FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits)),
		Fragment->PerfectTimingWindow, Fragment->DecoyLifetime);
	ActiveDecoy = Decoy;
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(ResolveInvulnerableEffectClass(), 1.0f, EffectContext);
	if (!EffectSpec.IsValid() || !ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get()).IsValid())
	{
		EndCurrentAbility(true);
		return;
	}
	PublishOutcome(Decoy, UPRTagLibrary::GetCombatResponseDecoyCreatedTag());
	bResolvedPathObstructed = SafeDistance + UE_KINDA_SMALL_NUMBER < Data->DisplacementDistance;
	DisplacementFinishedHandle = Subsystem->OnDisplacementFinished().AddUObject(
		this, &UPRGA_AfterimageDodge::HandleDisplacementFinished);
	FPRAbilityDisplacementRequest Displacement;
	Displacement.SourceActor = Avatar;
	Displacement.TargetActor = Avatar;
	Displacement.StartLocation = Avatar->GetActorLocation();
	Displacement.DesiredEndLocation = End;
	Displacement.Duration = Data->ActiveDuration;
	Displacement.bSweep = true;
	Displacement.bStopOnBlockingHit = true;
	Displacement.AbilityTag = Data->AbilityTag;
	if (!Component->RequestOwnedDisplacement(Displacement, FailureTag))
	{
		EndCurrentAbility(true);
		return;
	}
	DisplacementRequestId = Component->GetActiveDisplacementRequestId();
	PhaseExpiredHandle = Component->OnPhaseExpired().AddUObject(this, &UPRGA_AfterimageDodge::HandlePhaseExpired);
	if (!DisplacementRequestId.IsValid()
		|| !Component->BeginPhase(Data->AbilityTag, EPRPlayerSkillPhase::Active, Data->ActiveDuration))
	{
		EndCurrentAbility(true);
	}
}

void UPRGA_AfterimageDodge::EndAbility(
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
	if (bWasCancelled)
	{
		if (APRSkillDecoyActor* Decoy = ActiveDecoy.Get())
		{
			Decoy->Destroy();
		}
	}
	ActiveDecoy.Reset();
	DisplacementRequestId.Invalidate();
	bResolvedPathObstructed = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bEnding = false;
}

void UPRGA_AfterimageDodge::HandlePhaseExpired(const FGameplayTag Tag, const EPRPlayerSkillPhase Phase)
{
	if (!bEnding && Tag == AbilityTag && Phase == EPRPlayerSkillPhase::Recovery)
	{
		EndCurrentAbility(false);
	}
}

void UPRGA_AfterimageDodge::HandleDisplacementFinished(const FPRAbilityDisplacementResult& Result)
{
	if (!bEnding && Result.RequestId == DisplacementRequestId && Result.AbilityTag == AbilityTag)
	{
		FinishActive(Result.EndReason == EPRAbilityDisplacementEndReason::Completed && bResolvedPathObstructed
			? EPRAbilityDisplacementEndReason::Blocked
			: Result.EndReason);
	}
}

void UPRGA_AfterimageDodge::FinishActive(const EPRAbilityDisplacementEndReason EndReason)
{
	if (bEnding)
	{
		return;
	}
	DisplacementRequestId.Invalidate();
	if (EndReason != EPRAbilityDisplacementEndReason::Completed)
	{
		EndCurrentAbility(true);
		return;
	}
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	PublishOutcome(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr,
		UPRTagLibrary::GetCombatResponseDisplacementAppliedTag());
	const UPRPlayerSkillDataAsset* Data = GetSkillDataAsset();
	UPRPlayerSkillComponent* Component = GetPlayerSkillComponent();
	if (!Data || !Component || !Component->BeginPhase(Data->AbilityTag, EPRPlayerSkillPhase::Recovery, Data->RecoveryDuration))
	{
		EndCurrentAbility(true);
	}
}

void UPRGA_AfterimageDodge::EndCurrentAbility(const bool bWasCancelled)
{
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), ActorInfo, GetCurrentActivationInfo(), true, bWasCancelled);
	}
}

void UPRGA_AfterimageDodge::ClearRuntimeBindings()
{
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		if (AActor* Avatar = ActorInfo->AvatarActor.Get())
		{
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

bool UPRGA_AfterimageDodge::ResolveDodgePath(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UPRPlayerSkillDataAsset& Data,
	FVector& OutDirection,
	FVector& OutEnd,
	float& OutSafeDistance,
	FGameplayTag& OutFailureTag) const
{
	ACharacter* Avatar = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	if (!Avatar || !Subsystem || !Avatar->GetCapsuleComponent())
	{
		OutFailureTag = UPRTagLibrary::GetAbilityActivateFailInvalidMovementTag();
		return false;
	}
	OutDirection = PRAfterimageDodge::ResolvePlanarDodgeDirection(*Avatar);
	return !OutDirection.IsNearlyZero()
		&& Subsystem->ResolveShadowPath(
			*Avatar,
			OutDirection,
			Data.DisplacementDistance,
			Avatar->GetCapsuleComponent()->GetScaledCapsuleRadius(),
			OutEnd,
			OutSafeDistance,
			OutFailureTag);
}

void UPRGA_AfterimageDodge::PublishOutcome(AActor* Target, const FGameplayTag ResponseTag) const
{
	const FPRPlayerSkillExecutionSnapshot* Snapshot = GetExecutionSnapshot();
	AActor* Avatar = Snapshot ? Snapshot->SourceActor.Get() : nullptr;
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Avatar);
	const UAbilitySystemComponent* ASC = AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr;
	AActor* Owner = ASC ? ASC->GetOwnerActor() : nullptr;
	const IPRCombatantInterface* Combatant = Cast<IPRCombatantInterface>(Owner);
	UPRPlayerSkillSubsystem* Subsystem = Avatar && Avatar->GetWorld()
		? Avatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	if (!Snapshot || !Owner || !Combatant || !Subsystem || !IsValid(Target))
	{
		return;
	}
	FPRCombatOutcomeRequest Outcome;
	Outcome.SourceId = Combatant->GetCombatantId();
	Outcome.AbilitySource = const_cast<UPRGA_AfterimageDodge*>(this);
	Outcome.Instigator = Avatar;
	Outcome.Target = Target;
	Outcome.AbilityTag = Snapshot->AbilityTag;
	Outcome.ResponseTags.AddTag(ResponseTag);
	Subsystem->PublishAbilityOutcome(Outcome);
}

TSubclassOf<UGameplayEffect> UPRGA_AfterimageDodge::ResolveInvulnerableEffectClass() const
{
	if (InvulnerableEffectClass)
	{
		return InvulnerableEffectClass;
	}
	const UPRGA_AfterimageDodge* ClassDefault = GetClass()->GetDefaultObject<UPRGA_AfterimageDodge>();
	return ClassDefault && ClassDefault != this ? ClassDefault->InvulnerableEffectClass : nullptr;
}
