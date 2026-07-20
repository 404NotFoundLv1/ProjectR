// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Abilities/PRAbilityTypes.h"
#include "Abilities/Player/PRAbilityTargetInterface.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRCombatFeedbackInterface.h"
#include "GameFramework/Character.h"

#include "PREnemyCharacter.generated.h"

class UGameplayEffect;
class UPRAbilitySystemComponent;
class UPRAttributeSet;
class UPREnemyBrainComponent;
class UPREnemyPlaneMovementComponent;
class UPREnemyPrototypeDataAsset;
class UPREnemyAttackDataAsset;
class UPRAbilitySetDataAsset;
class UStateTree;
class UPRCombatSubsystem;
struct FPRCombatEvent;
struct FOnAttributeChangeData;

UCLASS(Abstract)
class PROJECTR_API APREnemyCharacter
	: public ACharacter
	, public IAbilitySystemInterface
	, public IPRCombatantInterface
	, public IPRCombatFeedbackInterface
	, public IPRAbilityTargetInterface
{
	GENERATED_BODY()

public:
	APREnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FName GetCombatantId() const override;
	virtual TSubclassOf<UGameplayEffect> GetDamageEffectClass() const override;
	virtual void HandleCombatHitReaction() override;
	virtual void HandleCombatLifeStateChanged(bool bIsDead) override;
	virtual FName GetAbilityTargetId() const override;
	virtual FVector GetAbilityTargetPoint() const override;
	virtual EPRAbilityTargetMobility GetAbilityTargetMobility() const override;
	virtual bool CanBeTargetedByAbility(FGameplayTag AbilityTag) const override;

	UPRAbilitySystemComponent* GetProjectRAbilitySystemComponent() const;
	const UPRAttributeSet* GetAttributeSet() const;
	UPREnemyPlaneMovementComponent* GetEnemyPlaneMovement() const;
	UPREnemyBrainComponent* GetEnemyBrain() const;
	const UPREnemyPrototypeDataAsset* GetEnemyPrototype() const;
	const TArray<TObjectPtr<UPREnemyAttackDataAsset>>& GetAttackDefinitions() const;
	UStateTree* GetEnemyStateTree() const;
	bool IsEnemyDead() const;
	bool IsEnemyInitialized() const;
	FGuid GetSpawnId() const;

	void ConfigureSpawn(UPREnemyPrototypeDataAsset* InPrototype, FGuid InSpawnId, float InPlaneY);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Combat")
	TSubclassOf<UGameplayEffect> DamageEffect;
	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Enemy")
	TObjectPtr<UStateTree> EnemyStateTree;

private:
	friend class FPREnemyShieldGuardingLifecycleTest;
	friend class FPREnemyEliteShieldBreakLifecycleTest;

	UPROPERTY(VisibleAnywhere, Category="ProjectR|Abilities")
	TObjectPtr<UPRAbilitySystemComponent> ProjectRAbilitySystemComponent;
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Abilities")
	TObjectPtr<UPRAttributeSet> ProjectRAttributeSet;
	UPROPERTY(VisibleAnywhere, Category="ProjectR|Enemy")
	TObjectPtr<UPREnemyBrainComponent> EnemyBrain;
	UPROPERTY(Transient)
	TObjectPtr<UPREnemyPrototypeDataAsset> Prototype;
	FGuid SpawnId;
	float PlaneY = 0.0f;
	bool bInitialized = false;
	bool bDefaultAttributesApplied = false;
	FPRAbilitySetGrantHandle InitialAbilityGrant;
	FDelegateHandle ShieldAttributeDelegateHandle;
	FDelegateHandle StunnedTagDelegateHandle;
	FDelegateHandle ShieldBreakCombatEventDelegateHandle;
	TWeakObjectPtr<UPRCombatSubsystem> BoundCombatSubsystem;
	bool bEnemyStunned = false;
	bool bShieldBreakResponseConsumed = false;

	/** Called only after the pawn has both begun play and acquired its fixed AI controller. */
	void TryInitializeEnemy();
	void BindShieldGuardingLifecycle();
	void ClearShieldGuardingLifecycle();
	void SynchronizeShieldGuardingState();
	void HandleShieldAttributeChanged(const FOnAttributeChangeData& ChangeData);
	bool BindEnemyStunnedLifecycle();
	void ClearEnemyStunnedLifecycle();
	bool BindShieldBreakResponseLifecycle();
	void ClearShieldBreakResponseLifecycle();
	void HandleEnemyStunnedTagChanged(FGameplayTag Tag, int32 NewCount);
	void HandleShieldBreakCombatEvent(const FPRCombatEvent& Event);
	void CancelActiveEnemyAttackAbilities();
};
