// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/PRSkillDecoyActor.h"

#include "AbilitySystemInterface.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Components/SceneComponent.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRTagLibrary.h"
#include "TimerManager.h"

APRSkillDecoyActor::APRSkillDecoyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	SetActorEnableCollision(false);
}

void APRSkillDecoyActor::InitializeProxy(AActor* InOwner, const FName InProxyId, const float InPerfectTimingWindow, const float InLifetime)
{
	if (!HasAuthority() || bConsumed || !IsValid(InOwner) || InProxyId.IsNone() || InPerfectTimingWindow < 0.0f || InLifetime <= 0.0f)
	{
		return;
	}
	ProxyOwner = InOwner;
	ProxyId = InProxyId;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	PerfectTimingEndTime = World->GetTimeSeconds() + InPerfectTimingWindow;
	InOwner->OnEndPlay.AddDynamic(this, &APRSkillDecoyActor::HandleProxyOwnerEndPlay);
	World->GetTimerManager().SetTimer(LifetimeTimer, this, &APRSkillDecoyActor::Expire, InLifetime, false);
}

FName APRSkillDecoyActor::GetAttackProxyId() const
{
	return ProxyId;
}

AActor* APRSkillDecoyActor::GetAttackProxyOwner() const
{
	return ProxyOwner.Get();
}

bool APRSkillDecoyActor::ConsumeAttackProxy(AActor* Attacker)
{
	if (!HasAuthority() || bConsumed || !IsValid(Attacker) || ProxyId.IsNone())
	{
		return false;
	}
	bConsumed = true;
	GetWorldTimerManager().ClearTimer(LifetimeTimer);
	PublishConsumptionOutcome();
	Destroy();
	return true;
}

void APRSkillDecoyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LifetimeTimer);
	if (AActor* ProxyOwnerActor = ProxyOwner.Get())
	{
		ProxyOwnerActor->OnEndPlay.RemoveDynamic(this, &APRSkillDecoyActor::HandleProxyOwnerEndPlay);
	}
	ProxyOwner.Reset();
	Super::EndPlay(EndPlayReason);
}

void APRSkillDecoyActor::Expire()
{
	if (!bConsumed)
	{
		Destroy();
	}
}

void APRSkillDecoyActor::HandleProxyOwnerEndPlay(AActor* Actor, const EEndPlayReason::Type EndPlayReason)
{
	if (!IsActorBeingDestroyed())
	{
		Destroy();
	}
}

void APRSkillDecoyActor::PublishConsumptionOutcome() const
{
	AActor* OwnerAvatar = ProxyOwner.Get();
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(OwnerAvatar);
	const UAbilitySystemComponent* ASC = AbilityInterface
		? AbilityInterface->GetAbilitySystemComponent()
		: nullptr;
	AActor* CombatOwner = ASC ? ASC->GetOwnerActor() : nullptr;
	const IPRCombatantInterface* Combatant = Cast<IPRCombatantInterface>(CombatOwner);
	UPRPlayerSkillSubsystem* Subsystem = OwnerAvatar && OwnerAvatar->GetWorld()
		? OwnerAvatar->GetWorld()->GetSubsystem<UPRPlayerSkillSubsystem>()
		: nullptr;
	if (!OwnerAvatar || !CombatOwner || !Combatant || !Subsystem)
	{
		return;
	}
	FPRCombatOutcomeRequest Outcome;
	Outcome.SourceId = Combatant->GetCombatantId();
	Outcome.AbilitySource = const_cast<APRSkillDecoyActor*>(this);
	Outcome.Instigator = OwnerAvatar;
	Outcome.Target = const_cast<APRSkillDecoyActor*>(this);
	Outcome.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"));
	Outcome.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseDecoyConsumedTag());
	if (const UWorld* World = GetWorld(); World && World->GetTimeSeconds() <= PerfectTimingEndTime)
	{
		Outcome.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
	}
	Subsystem->PublishAbilityOutcome(Outcome);
}

FName APRSkillDecoyActor::GetCombatEventSubjectId() const
{
	return GetAttackProxyId();
}
