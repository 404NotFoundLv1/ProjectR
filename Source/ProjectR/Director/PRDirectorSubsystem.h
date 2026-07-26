// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorProvider.h"
#include "Director/PRDirectorRuleEffectExecutor.h"
#include "Director/PRDirectorTypes.h"
#include "GameplayEffectTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PRDirectorSubsystem.generated.h"

class UPRDirectorRuleRegistryDataAsset;
class UPRDirectorRuleDataAsset;
class UPRCombatSubsystem;
class UPRQTESubsystem;
class UPRAbilitySystemComponent;
class UPRCompanionSubsystem;
struct FPRCombatEvent;
struct FPRQTEResult;
struct FPRAbilityLifecycleEvent;
struct FPRRelationshipChangedEvent;

/** Owns provider selection, validation, and non-gameplay applied-rule handles. */
UCLASS()
class PROJECTR_API UPRDirectorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	EPRDirectorRequestStatus RequestEvaluation(FGuid& OutRequestId);
	bool GetAppliedRules(TArray<FPRAppliedDirectorRuleHandle>& OutRules) const;
	EPRDirectorRuleOperationResult RemoveAppliedRule(const FPRAppliedDirectorRuleHandle& Handle);
	bool GetRuleRuntimeState(FGameplayTag RuleId, FPRDirectorRuleRuntimeState& OutState) const;
	void GetRuleRuntimeStates(TArray<FPRDirectorRuleRuntimeState>& OutStates) const;
	FPRDirectorEvaluationCompletedNative& OnEvaluationCompleted();
	FPRDirectorAppliedRuleChangedNative& OnAppliedRuleChanged();
	FPRDirectorRuleRuntimeChangedNative& OnRuleRuntimeChanged();
#if !UE_BUILD_SHIPPING
	EPRDirectorRuleOperationResult ApplyWhitelistedRuleForDevelopment(FGameplayTag RuleId, int32 Level, FGuid& OutHandleId);
#endif
private:
	void CompleteProviderResponse(const FPRDirectorResponse& Response, bool bFallback);
	EPRDirectorRuleOperationResult ApplyValidatedResponse(const FPRDirectorResponse& Response);
	void RebindRuntimeWorld();
	void UnbindRuntimeWorld();
	void HandleCombatEvent(const FPRCombatEvent& Event);
	void HandleQTEResult(const FPRQTEResult& Result);
	void HandleAbilityLifecycle(const FPRAbilityLifecycleEvent& Event);
	void HandleRelationshipChanged(const FPRRelationshipChangedEvent& Event);
	void HandleEnergyChanged(const FOnAttributeChangeData& Data);
	void HandleMaxEnergyChanged(const FOnAttributeChangeData& Data);
	void EvaluateResourceBalance(double WorldTimeSeconds);
	void ActivateResourceBalanceIfStillHigh();
	void EvaluateProfileDrivenRules(double WorldTimeSeconds);
	void BroadcastRuleRuntime(const FGameplayTag& RuleId);
	TObjectPtr<UPRDirectorRuleRegistryDataAsset> Registry = nullptr;
	TUniquePtr<IPRDirectorProvider> MockProvider;
	TUniquePtr<IPRDirectorProvider> HttpProvider;
	TArray<FPRAppliedDirectorRuleHandle> AppliedRules;
	FPRDirectorRequest ActiveRequest;
	FPRDirectorEvaluationCompletedNative EvaluationCompleted;
	FPRDirectorAppliedRuleChangedNative AppliedRuleChanged;
	FPRDirectorRuleRuntimeChangedNative RuleRuntimeChanged;
	TUniquePtr<FPRDirectorRuleEffectExecutor> RuleEffectExecutor;
	int64 RequestSequence = 0;
	int64 ApplySequence = 0;
	bool bHasActiveRequest = false;
	bool bShuttingDown = false;
	TWeakObjectPtr<UPRCombatSubsystem> BoundCombat;
	TWeakObjectPtr<UPRQTESubsystem> BoundQTE;
	TWeakObjectPtr<UPRAbilitySystemComponent> BoundPlayerASC;
	TWeakObjectPtr<UPRCompanionSubsystem> BoundCompanions;
	FDelegateHandle CombatEventHandle;
	FDelegateHandle QTEResultHandle;
	FDelegateHandle AbilityLifecycleHandle;
	FDelegateHandle RelationshipChangedHandle;
	FDelegateHandle EnergyAttributeHandle;
	FDelegateHandle MaxEnergyAttributeHandle;
	FTimerHandle ResourceBalanceHighEnergyTimer;
	FGameplayTag LastCommittedSkillTag;
	int32 RepetitionCommitStreak = 0;
	TSet<FGameplayTag> PredictionDistinctSkillTags;
	int32 LongRangeSafeActionCount = 0;
	float ResourceEnergySpentSinceApply = 0.0f;
};
