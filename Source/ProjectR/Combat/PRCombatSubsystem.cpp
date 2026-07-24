// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/PRCombatSubsystem.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRPlayerSkillTypes.h"
#include "AbilitySystemInterface.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRCombatFeedbackInterface.h"
#include "Core/PRCombatEventSubjectInterface.h"
#include "Core/PRCombatMitigationInterface.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffect.h"
#include "ProjectR.h"

namespace
{
constexpr float MaxNormalizedDamage = 200000.0f;

struct FResolvedCombatTarget
{
	AActor* Target = nullptr;
	UPRAbilitySystemComponent* ASC = nullptr;
	const UPRAttributeSet* Attributes = nullptr;
	IPRCombatantInterface* Combatant = nullptr;
	FName TargetId;
	TSubclassOf<UGameplayEffect> DamageEffect;
};

bool ResolveCombatTarget(
	const FName SourceId,
	AActor* Target,
	const bool bRequireDamageEffect,
	FResolvedCombatTarget& OutTarget)
{
	if (SourceId.IsNone() || !IsValid(Target) || !Target->HasAuthority())
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid damage request: SourceId or authoritative Target is invalid."));
		return false;
	}

	IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Target);
	UPRAbilitySystemComponent* ASC = AbilityInterface
		? Cast<UPRAbilitySystemComponent>(AbilityInterface->GetAbilitySystemComponent())
		: nullptr;
	if (!IsValid(ASC) || ASC->GetAvatarActor() != Target)
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid damage request: Target %s is not the current ProjectR ASC Avatar."),
			*GetNameSafe(Target));
		return false;
	}

	AActor* Owner = ASC->GetOwnerActor();
	IPRCombatantInterface* Combatant = Cast<IPRCombatantInterface>(Owner);
	const UPRAttributeSet* Attributes = Cast<UPRAttributeSet>(ASC->GetAttributeSet(UPRAttributeSet::StaticClass()));
	if (!IsValid(Owner) || !Combatant || !IsValid(Attributes))
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid damage request: ASC Owner, Combatant, or AttributeSet is missing for %s."),
			*GetNameSafe(Target));
		return false;
	}

	const FName TargetId = Combatant->GetCombatantId();
	const TSubclassOf<UGameplayEffect> DamageEffect = Combatant->GetDamageEffectClass();
	if (TargetId.IsNone() || (bRequireDamageEffect && !DamageEffect))
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid damage request: Combatant %s has no stable ID or DamageEffect."),
			*GetNameSafe(Owner));
		return false;
	}

	OutTarget.Target = Target;
	OutTarget.ASC = ASC;
	OutTarget.Attributes = Attributes;
	OutTarget.Combatant = Combatant;
	OutTarget.TargetId = TargetId;
	OutTarget.DamageEffect = DamageEffect;
	return true;
}

bool SetLifeState(UPRAbilitySystemComponent* ASC, const bool bDead)
{
	if (!IsValid(ASC))
	{
		return false;
	}

	const FGameplayTag& AliveTag = UPRTagLibrary::GetStateAliveTag();
	const FGameplayTag& DeadTag = UPRTagLibrary::GetStateDeadTag();
	const int32 OldAliveCount = ASC->GetGameplayTagCount(AliveTag);
	const int32 OldDeadCount = ASC->GetGameplayTagCount(DeadTag);
	ASC->SetLooseGameplayTagCount(AliveTag, bDead ? 0 : 1, EGameplayTagReplicationState::TagOnly);
	ASC->SetLooseGameplayTagCount(DeadTag, bDead ? 1 : 0, EGameplayTagReplicationState::TagOnly);
	const bool bSucceeded = ASC->GetGameplayTagCount(AliveTag) == (bDead ? 0 : 1)
		&& ASC->GetGameplayTagCount(DeadTag) == (bDead ? 1 : 0);
	if (!bSucceeded)
	{
		ASC->SetLooseGameplayTagCount(AliveTag, OldAliveCount, EGameplayTagReplicationState::TagOnly);
		ASC->SetLooseGameplayTagCount(DeadTag, OldDeadCount, EGameplayTagReplicationState::TagOnly);
	}
	return bSucceeded;
}

bool AreResponseTagsValid(const FGameplayTagContainer& Tags)
{
	if (Tags.IsEmpty())
	{
		return false;
	}
	for (const FGameplayTag& Tag : Tags)
	{
		if (!Tag.IsValid() || !Tag.ToString().StartsWith(TEXT("Combat.Response.")))
		{
			return false;
		}
	}
	return true;
}

FPRCombatEvent MakeDamageEvent(
	const UWorld* World,
	const FPRDamageRequest& Request,
	const FResolvedCombatTarget& Target,
	const FGameplayTag& EventTag,
	const float NormalizedDamage)
{
	FPRCombatEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.EventTag = EventTag;
	Event.SourceId = Request.SourceId;
	Event.TargetId = Target.TargetId;
	Event.DamageSource = Request.DamageSource;
	Event.Instigator = Request.Instigator;
	Event.Target = Request.Target;
	Event.AbilityTag = Request.AbilityTag;
	Event.RawDamage = NormalizedDamage;
	Event.RemainingHealth = Target.Attributes->GetHealth();
	Event.RemainingShield = Target.Attributes->GetShield();
	Event.MaxHealth = Target.Attributes->GetMaxHealth();
	Event.bCritical = Request.bCritical;
	Event.DamageTags = Request.DamageTags;
	Event.WorldTimeSeconds = IsValid(World) ? World->GetTimeSeconds() : 0.0;
	return Event;
}

FPRCombatEvent MakeReviveEvent(
	const UWorld* World,
	const FPRReviveRequest& Request,
	const FResolvedCombatTarget& Target)
{
	FPRCombatEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.EventTag = UPRTagLibrary::GetCombatEventReviveTag();
	Event.SourceId = Request.SourceId;
	Event.TargetId = Target.TargetId;
	Event.DamageSource = Request.DamageSource;
	Event.Instigator = Request.Instigator;
	Event.Target = Request.Target;
	Event.RemainingHealth = Target.Attributes->GetHealth();
	Event.RemainingShield = Target.Attributes->GetShield();
	Event.MaxHealth = Target.Attributes->GetMaxHealth();
	Event.ResponseTags.AddTag(UPRTagLibrary::GetStateAliveTag());
	Event.WorldTimeSeconds = IsValid(World) ? World->GetTimeSeconds() : 0.0;
	return Event;
}
} // namespace

EPRCombatRequestStatus UPRCombatSubsystem::ApplyDamage(const FPRDamageRequest& Request)
{
	if (!FMath::IsFinite(Request.RawDamage) || Request.RawDamage <= 0.0f
		|| Request.ImpactOrigin.ContainsNaN() || Request.IncomingDirection.ContainsNaN())
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid damage request from %s: RawDamage %.9g is not finite and positive."),
			*Request.SourceId.ToString(), Request.RawDamage);
		return EPRCombatRequestStatus::Invalid;
	}
	FPRDamageRequest ValidatedRequest = Request;
	if (!ValidatedRequest.IncomingDirection.IsNearlyZero())
	{
		ValidatedRequest.IncomingDirection.Normalize();
	}
	else if (ValidatedRequest.AbilityTag.IsValid()
		&& ValidatedRequest.AbilityTag.ToString().StartsWith(TEXT("Skill.")))
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat rejected player-skill damage with zero IncomingDirection from %s."),
			*ValidatedRequest.SourceId.ToString());
		return EPRCombatRequestStatus::Invalid;
	}

	FResolvedCombatTarget Target;
	if (!ResolveCombatTarget(ValidatedRequest.SourceId, ValidatedRequest.Target.Get(), true, Target))
	{
		return EPRCombatRequestStatus::Invalid;
	}

	const float NormalizedDamage = FMath::Min(ValidatedRequest.RawDamage, MaxNormalizedDamage);
	if (ValidatedRequest.RawDamage > MaxNormalizedDamage)
	{
		UE_LOG(LogProjectR, Warning,
			TEXT("Combat clamped damage from %.9g to %.9g for SourceId %s."),
			ValidatedRequest.RawDamage, NormalizedDamage, *ValidatedRequest.SourceId.ToString());
	}

	const bool bDead = Target.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag());
	if (bDead)
	{
		FPRCombatEvent Rejected = MakeDamageEvent(
			GetWorld(), ValidatedRequest, Target, UPRTagLibrary::GetCombatEventDamageRejectedTag(), NormalizedDamage);
		Rejected.ResponseTags.AddTag(UPRTagLibrary::GetStateDeadTag());
		BroadcastCombatEvent(Rejected);
		return EPRCombatRequestStatus::RejectedDead;
	}

	if (Target.Attributes->GetHealth() <= 0.0f)
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid damage request for %s: Health is zero without State.Dead."),
			*Target.TargetId.ToString());
		return EPRCombatRequestStatus::Invalid;
	}

	if (const IPRCombatMitigationInterface* Mitigation = Cast<IPRCombatMitigationInterface>(Target.Target))
	{
		FGameplayTagContainer MitigationTags;
		const EPRCombatMitigationResult MitigationResult = Mitigation->EvaluateIncomingDamage(
			ValidatedRequest,
			MitigationTags);
		if (MitigationResult == EPRCombatMitigationResult::Blocked)
		{
			if (!AreResponseTagsValid(MitigationTags))
			{
				UE_LOG(LogProjectR, Error,
					TEXT("Combat mitigation for %s returned Blocked without valid Combat.Response tags."),
					*Target.TargetId.ToString());
				return EPRCombatRequestStatus::Invalid;
			}
			FPRCombatEvent Rejected = MakeDamageEvent(
				GetWorld(),
				ValidatedRequest,
				Target,
				UPRTagLibrary::GetCombatEventDamageRejectedTag(),
				NormalizedDamage);
			Rejected.ResponseTags.AppendTags(MitigationTags);
			BroadcastCombatEvent(Rejected);
			return EPRCombatRequestStatus::RejectedBlocked;
		}
	}

	if (Target.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateInvulnerableTag()))
	{
		FPRCombatEvent Rejected = MakeDamageEvent(
			GetWorld(), ValidatedRequest, Target, UPRTagLibrary::GetCombatEventDamageRejectedTag(), NormalizedDamage);
		Rejected.ResponseTags.AddTag(UPRTagLibrary::GetStateInvulnerableTag());
		BroadcastCombatEvent(Rejected);
		return EPRCombatRequestStatus::RejectedInvulnerable;
	}

	UPRAbilitySystemComponent* SourceASC = nullptr;
	if (AActor* Instigator = ValidatedRequest.Instigator.Get())
	{
		if (IAbilitySystemInterface* SourceInterface = Cast<IAbilitySystemInterface>(Instigator))
		{
			SourceASC = Cast<UPRAbilitySystemComponent>(SourceInterface->GetAbilitySystemComponent());
		}
	}
	if (!IsValid(SourceASC))
	{
		SourceASC = Target.ASC;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	AActor* EffectCauser = Cast<AActor>(ValidatedRequest.DamageSource.Get());
	Context.AddInstigator(ValidatedRequest.Instigator.Get(),
		EffectCauser ? EffectCauser : ValidatedRequest.Instigator.Get());
	if (UObject* DamageSource = ValidatedRequest.DamageSource.Get())
	{
		Context.AddSourceObject(DamageSource);
	}
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Target.DamageEffect, 1.0f, Context);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid damage request for %s: GE_Damage Spec creation failed."),
			*Target.TargetId.ToString());
		return EPRCombatRequestStatus::Invalid;
	}
	SpecHandle.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetCombatDataDamageTag(), NormalizedDamage);

	const float BeforeHealth = Target.Attributes->GetHealth();
	const float BeforeShield = Target.Attributes->GetShield();
	const FActiveGameplayEffectHandle AppliedHandle = SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(), Target.ASC);
	if (!AppliedHandle.WasSuccessfullyApplied())
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid damage request for %s: GE_Damage was not applied."),
			*Target.TargetId.ToString());
		return EPRCombatRequestStatus::Invalid;
	}

	const float AfterHealth = Target.Attributes->GetHealth();
	const float AfterShield = Target.Attributes->GetShield();
	FPRCombatEvent DamageEvent = MakeDamageEvent(
		GetWorld(), ValidatedRequest, Target, UPRTagLibrary::GetCombatEventDamageTag(), NormalizedDamage);
	DamageEvent.ShieldAbsorbed = FMath::Max(0.0f, BeforeShield - AfterShield);
	DamageEvent.HealthDamage = FMath::Max(0.0f, BeforeHealth - AfterHealth);
	DamageEvent.RemainingHealth = AfterHealth;
	DamageEvent.RemainingShield = AfterShield;
	DamageEvent.bFatal = BeforeHealth > 0.0f && AfterHealth <= 0.0f;
	if (DamageEvent.ShieldAbsorbed > 0.0f)
	{
		DamageEvent.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseShieldAbsorbedTag());
	}
	if (BeforeShield > 0.0f && AfterShield <= 0.0f)
	{
		DamageEvent.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseShieldBrokenTag());
	}
	if (DamageEvent.HealthDamage > 0.0f)
	{
		DamageEvent.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseHealthDamagedTag());
	}
	BroadcastCombatEvent(DamageEvent);

	IPRCombatFeedbackInterface* Feedback = Cast<IPRCombatFeedbackInterface>(Target.Target);
	if (DamageEvent.bFatal)
	{
		if (!SetLifeState(Target.ASC, true))
		{
			UE_LOG(LogProjectR, Error,
				TEXT("Combat failed to enter State.Dead for %s after fatal damage."),
				*Target.TargetId.ToString());
			return EPRCombatRequestStatus::Invalid;
		}
		if (Feedback)
		{
			Feedback->HandleCombatLifeStateChanged(true);
		}
		FPRCombatEvent DeathEvent = DamageEvent;
		DeathEvent.EventId = FGuid::NewGuid();
		DeathEvent.EventTag = UPRTagLibrary::GetCombatEventDeathTag();
		DeathEvent.ResponseTags.AddTag(UPRTagLibrary::GetStateDeadTag());
		DeathEvent.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		BroadcastCombatEvent(DeathEvent);
	}
	else if ((DamageEvent.ShieldAbsorbed > 0.0f || DamageEvent.HealthDamage > 0.0f) && Feedback)
	{
		Feedback->HandleCombatHitReaction();
	}

	return EPRCombatRequestStatus::Applied;
}

EPRCombatRequestStatus UPRCombatSubsystem::Revive(const FPRReviveRequest& Request)
{
	if (!FMath::IsFinite(Request.HealthFraction)
		|| !FMath::IsFinite(Request.ShieldFraction)
		|| Request.HealthFraction <= 0.0f
		|| Request.HealthFraction > 1.0f
		|| Request.ShieldFraction < 0.0f
		|| Request.ShieldFraction > 1.0f)
	{
		UE_LOG(LogProjectR, Error,
			TEXT("Combat received an invalid revive request from %s: fractions are out of range."),
			*Request.SourceId.ToString());
		return EPRCombatRequestStatus::Invalid;
	}

	FResolvedCombatTarget Target;
	if (!ResolveCombatTarget(Request.SourceId, Request.Target.Get(), false, Target))
	{
		return EPRCombatRequestStatus::Invalid;
	}
	if (!Target.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
	{
		return EPRCombatRequestStatus::RejectedAlive;
	}

	const float OldHealth = Target.Attributes->GetHealth();
	const float OldShield = Target.Attributes->GetShield();
	const int32 OldAliveCount = Target.ASC->GetGameplayTagCount(UPRTagLibrary::GetStateAliveTag());
	const int32 OldDeadCount = Target.ASC->GetGameplayTagCount(UPRTagLibrary::GetStateDeadTag());
	const float NewHealth = Target.Attributes->GetMaxHealth() * Request.HealthFraction;
	const float NewShield = Target.Attributes->GetMaxShield() * Request.ShieldFraction;
	Target.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), NewHealth);
	Target.ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), NewShield);
	const bool bValuesSucceeded = Target.Attributes->GetHealth() > 0.0f
		&& FMath::IsNearlyEqual(Target.Attributes->GetHealth(), NewHealth)
		&& FMath::IsNearlyEqual(Target.Attributes->GetShield(), NewShield);
	if (!bValuesSucceeded || !SetLifeState(Target.ASC, false))
	{
		Target.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), OldHealth);
		Target.ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), OldShield);
		Target.ASC->SetLooseGameplayTagCount(
			UPRTagLibrary::GetStateAliveTag(), OldAliveCount, EGameplayTagReplicationState::TagOnly);
		Target.ASC->SetLooseGameplayTagCount(
			UPRTagLibrary::GetStateDeadTag(), OldDeadCount, EGameplayTagReplicationState::TagOnly);
		UE_LOG(LogProjectR, Error,
			TEXT("Combat failed to atomically revive %s and restored the prior state."),
			*Target.TargetId.ToString());
		return EPRCombatRequestStatus::Invalid;
	}

	if (IPRCombatFeedbackInterface* Feedback = Cast<IPRCombatFeedbackInterface>(Target.Target))
	{
		Feedback->HandleCombatLifeStateChanged(false);
	}
	BroadcastCombatEvent(MakeReviveEvent(GetWorld(), Request, Target));
	return EPRCombatRequestStatus::Applied;
}

bool UPRCombatSubsystem::PublishAbilityOutcome(const FPRCombatOutcomeRequest& Request)
{
	AActor* Instigator = Request.Instigator.Get();
	if (Request.SourceId.IsNone() || !IsValid(Instigator) || !Instigator->HasAuthority()
		|| !Request.AbilityTag.IsValid() || !AreResponseTagsValid(Request.ResponseTags))
	{
		UE_LOG(LogProjectR, Error, TEXT("Combat rejected an invalid AbilityOutcome request."));
		return false;
	}

	FPRCombatEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.EventTag = UPRTagLibrary::GetCombatEventAbilityOutcomeTag();
	Event.SourceId = Request.SourceId;
	Event.DamageSource = Request.AbilitySource;
	Event.Instigator = Instigator;
	Event.AbilityTag = Request.AbilityTag;
	Event.ResponseTags = Request.ResponseTags;
	Event.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	AActor* RequestedTarget = Request.Target.Get();
	FName TargetId;
	if (const IPRCombatantInterface* Combatant = Cast<IPRCombatantInterface>(RequestedTarget))
	{
		TargetId = Combatant->GetCombatantId();
	}
	if (TargetId.IsNone())
	{
		if (const IPRCombatEventSubjectInterface* Subject = Cast<IPRCombatEventSubjectInterface>(RequestedTarget))
		{
			TargetId = Subject->GetCombatEventSubjectId();
		}
	}
	if (!TargetId.IsNone())
	{
		Event.Target = RequestedTarget;
		Event.TargetId = TargetId;
	}
	BroadcastCombatEvent(Event);
	return true;
}

FPRCombatEventNative& UPRCombatSubsystem::OnCombatEvent()
{
	return CombatEvent;
}

void UPRCombatSubsystem::BroadcastCombatEvent(const FPRCombatEvent& Event)
{
	CombatEvent.Broadcast(Event);
}
