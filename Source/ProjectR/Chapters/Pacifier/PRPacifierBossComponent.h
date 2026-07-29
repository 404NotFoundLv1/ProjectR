// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/Pacifier/PRPacifierTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "PRPacifierBossComponent.generated.h"

UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRPacifierBossComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRPacifierBossComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ConfigureChapterState(int32 ComfortPressure);
	const FPRPacifierBossRuntimeState& GetRuntimeState() const;

private:
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void EvaluatePhases();
	void EnterIllusionSplit();
	void SpawnIllusionProjections();
	void ResolveIllusionSplit();
	void EnterLowRiskRewardLure();
	void MonitorLowRiskRewardLure();
	void ResolveLowRiskRewardLure();
	void EnterAdventureYieldSuppression();
	void ResolveAdventureYieldSuppression();
	void DestroyIllusionProjections();
	void ClearTransientState();
	void ApplyPlayerDamage(float Damage, FName SourceId);
	void PublishCompletion();
	void ShowMechanic(const TCHAR* Text, const FColor& Color);
	void MarkDegradedNoOp();
	class APREnemyCharacter* GetBoss() const;
	class APawn* GetPlayerPawn() const;
	bool IsLegalP0Skill(FGameplayTag AbilityTag) const;

	FPRPacifierBossRuntimeState RuntimeState;
	TSet<FGameplayTag> IllusionCounterSkills;
	TSet<FGameplayTag> SuppressionCounterSkills;
	TArray<TWeakObjectPtr<class APRPacifierIllusionProjection>> IllusionProjections;
	TWeakObjectPtr<class UPRCombatSubsystem> BoundCombat;
	FDelegateHandle CombatHandle;
	FTimerHandle IllusionWarningTimer;
	FTimerHandle IllusionWindowTimer;
	FTimerHandle LureWindowTimer;
	FTimerHandle LureMonitorTimer;
	FTimerHandle SuppressionTimer;
	bool bStayedOutsideLureRange = true;
	bool bCompletionPublished = false;
};
