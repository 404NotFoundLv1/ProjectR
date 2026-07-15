// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"

#include "Combat/PRCombatTypes.h"
#include "Components/SphereComponent.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

APRPlayerSkillTestTarget::APRPlayerSkillTestTarget()
{
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("TargetCollision"));
	CollisionComponent->InitSphereRadius(30.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(CollisionComponent);
}

FName APRPlayerSkillTestTarget::GetAbilityTargetId() const
{
	return TargetId;
}

FVector APRPlayerSkillTestTarget::GetAbilityTargetPoint() const
{
	return GetActorLocation() + TargetPointOffset;
}

EPRAbilityTargetMobility APRPlayerSkillTestTarget::GetAbilityTargetMobility() const
{
	return Mobility;
}

bool APRPlayerSkillTestTarget::CanBeTargetedByAbility(const FGameplayTag AbilityTag) const
{
	return bTargetable && AbilityTag.IsValid();
}

FName APRPlayerSkillTestTarget::GetCombatEventSubjectId() const
{
	return TargetId;
}

void APRPlayerSkillTestTarget::ConfigureTarget(
	const FName InTargetId,
	const bool bInTargetable,
	const EPRAbilityTargetMobility InMobility)
{
	TargetId = InTargetId;
	bTargetable = bInTargetable;
	Mobility = InMobility;
}

void APRPlayerSkillTestTarget::SetTargetPointOffset(const FVector InOffset)
{
	TargetPointOffset = InOffset;
}

void APRPlayerSkillTestTarget::SetTestCollisionObjectType(const ECollisionChannel InObjectType)
{
	CollisionComponent->SetCollisionObjectType(InObjectType);
}

void APRPlayerSkillMitigationTestCharacter::ConfigureMitigation(
	const EPRCombatMitigationResult InResult,
	const FGameplayTagContainer& InResponseTags)
{
	MitigationResult = InResult;
	MitigationResponseTags = InResponseTags;
}

int32 APRPlayerSkillMitigationTestCharacter::GetMitigationEvaluationCount() const
{
	return MitigationEvaluationCount;
}

FVector APRPlayerSkillMitigationTestCharacter::GetLastIncomingDirection() const
{
	return LastIncomingDirection;
}

EPRCombatMitigationResult APRPlayerSkillMitigationTestCharacter::EvaluateIncomingDamage(
	const FPRDamageRequest& Request,
	FGameplayTagContainer& OutResponseTags) const
{
	++MitigationEvaluationCount;
	LastIncomingDirection = Request.IncomingDirection;
	OutResponseTags.AppendTags(MitigationResponseTags);
	return MitigationResult;
}

FName APRPlayerSkillCombatTestCharacter::GetAbilityTargetId() const
{
	return TargetId;
}

FVector APRPlayerSkillCombatTestCharacter::GetAbilityTargetPoint() const
{
	return GetActorLocation();
}

EPRAbilityTargetMobility APRPlayerSkillCombatTestCharacter::GetAbilityTargetMobility() const
{
	return EPRAbilityTargetMobility::Light;
}

bool APRPlayerSkillCombatTestCharacter::CanBeTargetedByAbility(const FGameplayTag AbilityTag) const
{
	return bTargetable && AbilityTag.IsValid();
}

void APRPlayerSkillCombatTestCharacter::ConfigureTarget(
	const FName InTargetId,
	const bool bInTargetable)
{
	TargetId = InTargetId;
	bTargetable = bInTargetable;
}


UPRPlayerSkillBurningTestEffect::UPRPlayerSkillBurningTestEffect(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));
	Period = FScalableFloat(0.0f);
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
	UTargetTagsGameplayEffectComponent* TargetTags =
		ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(
			this,
			TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	FInheritedTagContainer GrantedTags;
	GrantedTags.AddTag(UPRTagLibrary::GetStateBurningTag());
	TargetTags->SetAndApplyTargetTagChanges(GrantedTags);
}

UPRPlayerSkillLifecycleTestAbility::UPRPlayerSkillLifecycleTestAbility()
{
	AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
}

void UPRPlayerSkillLifecycleTestAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
}
