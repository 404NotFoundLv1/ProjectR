// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/PRAbilitySystemComponent.h"
#include "Components/ActorComponent.h"
#include "Enemies/Bosses/PRBossTypes.h"
#include "Enemies/PREnemyAttackSelectionInterface.h"

#include "PRAuditorBossComponent.generated.h"

class APREnemyCharacter;
struct FPRDamageRequest;
struct FPRCombatEvent;
struct FOnAttributeChangeData;

/** Event-driven Auditor mechanics; it never owns attributes, damage, or CombatEvent publication. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRAuditorBossComponent : public UActorComponent, public IPREnemyAttackSelectionInterface
{
	GENERATED_BODY()

public:
	UPRAuditorBossComponent();
	/** Pure deterministic policy helpers; they make the P2/P3 data contract directly regression-testable. */
	static FGameplayTag SelectRuleAuditRule(const TArray<FPRAuditorBossSample>& InSamples);
	static FGameplayTag SelectPredictedAbilityTag(const TArray<FPRAuditorBossSample>& InSamples);
	virtual bool SelectEnemyAttack(AActor* Target, FGameplayTag& OutAttackTag) const override;
	virtual bool GetPreferredRangeOverride(AActor* Target, float& OutMinimumRange, float& OutMaximumRange) const override;
	virtual void NotifyEnemyAttackActivated(FGameplayTag AttackTag) override;
	EPRAuditorBossPhase GetPhase() const;
	void SetPhase(EPRAuditorBossPhase InPhase);
	bool IsPredictionBlocking(const FPRDamageRequest& Request, FGameplayTagContainer& OutResponseTags) const;
	FGameplayTag GetPredictedAbilityTag() const;
	FGameplayTag GetActiveRuleId() const;
	void HandleBossDefeated();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleCombatEvent(const FPRCombatEvent& Event);
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void RefreshPhaseAndState();
	void EvaluateRuleAudit();
	void EnterPredictionShield();
	void RemovePredictionShield();
	void PublishState() const;
	void InitializeWhenReady();
	void PruneSamples(double Now);
	const class UPRAuditorBossDataAsset* GetAuditorData() const;
	APREnemyCharacter* GetEnemyOwner() const;

	EPRAuditorBossPhase Phase = EPRAuditorBossPhase::Dormant;
	FGameplayTag PredictedAbilityTag;
	FGameplayTag ActiveRuleId;
	FGameplayTag RepeatedAbilityTag;
	bool bCounterArmed = false;
	bool bDefeatedPublished = false;
	TArray<FPRAuditorBossSample> Samples;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle HealthChangedHandle;
	FActiveGameplayEffectHandle PredictionShieldHandle;
	FTimerHandle InitializationTimer;
};
