// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "Chapters/Auditor/PRAuditorChapterTypes.h"
#include "Components/ActorComponent.h"

#include "PRAuditorChapterBossComponent.generated.h"

/** Event-driven chapter overlay for the frozen Demo Auditor; never publishes Boss completion. */
UCLASS(ClassGroup=(ProjectR), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRAuditorChapterBossComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRAuditorChapterBossComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ConfigureChapterState(int32 AuditPressure);
	const FPRAuditorChapterBossRuntimeState& GetRuntimeState() const;

private:
	void HandleCombatEvent(const struct FPRCombatEvent& Event);
	void EvaluateBasePhase();
	void StartRepeatedBuildAudit();
	void ResolveRepeatedBuildAudit();
	void StartVerdictEscalation();
	void ResolveVerdictEscalation();
	void CaptureHabitProjection();
	void ApplyPlayerDamage(float Damage, FName SourceId);
	void ClearTransientState();
	void ShowMechanic(const TCHAR* Text, const FColor& Color);
	void MarkDegradedNoOp();
	bool IsLegalP0Skill(FGameplayTag AbilityTag) const;
	class APRAuditorChapterBoss* GetBoss() const;
	class APawn* GetPlayerPawn() const;

	FPRAuditorChapterBossRuntimeState RuntimeState;
	TSet<FGameplayTag> RepeatedBuildSkills;
	TSet<FGameplayTag> VerdictSkills;
	TWeakObjectPtr<class UPRCombatSubsystem> BoundCombat;
	FDelegateHandle CombatHandle;
	FTimerHandle RepeatedBuildTimer;
	FTimerHandle VerdictTimer;
	bool bRepeatedBuildStarted = false;
	bool bVerdictStarted = false;
};
