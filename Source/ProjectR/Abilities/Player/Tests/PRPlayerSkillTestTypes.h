// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Player/PRAbilityTargetInterface.h"
#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"
#include "Characters/PRPlayerCharacter.h"
#include "Core/PRCombatEventSubjectInterface.h"
#include "GameplayEffect.h"

#include "PRPlayerSkillTestTypes.generated.h"

class USphereComponent;

/** Transient native target used only by ProjectR automation worlds. */
UCLASS(Transient, NotPlaceable, NotBlueprintable)
class PROJECTR_API APRPlayerSkillTestTarget final
	: public AActor
	, public IPRAbilityTargetInterface
	, public IPRCombatEventSubjectInterface
{
	GENERATED_BODY()

public:
	APRPlayerSkillTestTarget();

	virtual FName GetAbilityTargetId() const override;
	virtual FVector GetAbilityTargetPoint() const override;
	virtual EPRAbilityTargetMobility GetAbilityTargetMobility() const override;
	virtual bool CanBeTargetedByAbility(FGameplayTag AbilityTag) const override;
	virtual FName GetCombatEventSubjectId() const override;

	void ConfigureTarget(
		FName InTargetId,
		bool bInTargetable,
		EPRAbilityTargetMobility InMobility = EPRAbilityTargetMobility::Light);
	void SetTargetPointOffset(FVector InOffset);
	void SetTestCollisionObjectType(ECollisionChannel InObjectType);

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> CollisionComponent;

	FName TargetId = TEXT("Automation.Target");
	FVector TargetPointOffset = FVector::ZeroVector;
	EPRAbilityTargetMobility Mobility = EPRAbilityTargetMobility::Light;
	bool bTargetable = true;
};

/** Player character whose native mitigation result is configurable by automation. */
UCLASS(Transient, NotPlaceable, NotBlueprintable)
class PROJECTR_API APRPlayerSkillMitigationTestCharacter final : public APRPlayerCharacter
{
	GENERATED_BODY()

public:
	void ConfigureMitigation(
		EPRCombatMitigationResult InResult,
		const FGameplayTagContainer& InResponseTags);
	int32 GetMitigationEvaluationCount() const;
	FVector GetLastIncomingDirection() const;

	virtual EPRCombatMitigationResult EvaluateIncomingDamage(
		const FPRDamageRequest& Request,
		FGameplayTagContainer& OutResponseTags) const override;

private:
	EPRCombatMitigationResult MitigationResult = EPRCombatMitigationResult::NotHandled;
	FGameplayTagContainer MitigationResponseTags;
	mutable int32 MitigationEvaluationCount = 0;
	mutable FVector LastIncomingDirection = FVector::ZeroVector;
};

/** Transient combat-capable target used to exercise formal player skill abilities. */
UCLASS(Transient, NotPlaceable, NotBlueprintable)
class PROJECTR_API APRPlayerSkillCombatTestCharacter final
	: public APRPlayerCharacter
	, public IPRAbilityTargetInterface
{
	GENERATED_BODY()

public:
	virtual FName GetAbilityTargetId() const override;
	virtual FVector GetAbilityTargetPoint() const override;
	virtual EPRAbilityTargetMobility GetAbilityTargetMobility() const override;
	virtual bool CanBeTargetedByAbility(FGameplayTag AbilityTag) const override;

	void ConfigureTarget(FName InTargetId, bool bInTargetable = true);

private:
	FName TargetId = TEXT("Automation.CombatTarget");
	bool bTargetable = true;
};

/** In-memory Burning GE used before the formal checkpoint-B asset is authored. */
UCLASS(Transient, NotBlueprintable)
class PROJECTR_API UPRPlayerSkillBurningTestEffect final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPRPlayerSkillBurningTestEffect(const FObjectInitializer& ObjectInitializer);
};

/** Transient ability that stays active until GAS lifecycle cancellation is exercised. */
UCLASS(Transient, NotBlueprintable)
class PROJECTR_API UPRPlayerSkillLifecycleTestAbility final : public UPRPlayerSkillGameplayAbility
{
	GENERATED_BODY()

public:
	UPRPlayerSkillLifecycleTestAbility();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
