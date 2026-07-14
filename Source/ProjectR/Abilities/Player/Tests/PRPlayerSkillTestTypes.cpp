// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"

#include "Combat/PRCombatTypes.h"
#include "Components/SphereComponent.h"

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
