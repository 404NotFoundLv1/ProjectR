// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDirectorAutomationToolset.h"

#include "Abilities/PRAbilityTypes.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRPlayerState.h"
#include "Core/PRRelationshipTypes.h"
#include "Director/PRDirectorResponseValidator.h"
#include "Director/PRDirectorRuleEffectTypes.h"
#include "Director/PRDirectorSubsystem.h"
#include "Director/PRHttpDirectorProvider.h"
#include "Director/PRPlayerProfileSubsystem.h"
#include "Divergence/PRDivergenceTypes.h"
#include "Editor.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "QTE/PRQTETypes.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRDirectorAutomation
{
class FRunner final : public TSharedFromThis<FRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FRunner> Runner = MakeShared<FRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!Runner->World.IsValid() || Runner->World->GetNetMode() == NM_Client)
		{
			Runner->Fail(TEXT("RunPIEDirectorSmoke requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		Runner->StartedAt = FPlatformTime::Seconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float)
		{
			return KeepAlive->Tick() && !KeepAlive->bComplete;
		}));
		return Runner->Result.Get();
	}

private:
	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		if (!PlayWorld || FPlatformTime::Seconds() - StartedAt > 15.0)
		{
			Fail(FString::Printf(TEXT("RunPIEDirectorSmoke timed out (stage=%d, wait=%s)."), Stage, *LastWaitReason));
			return false;
		}
		UGameInstance* GameInstance = PlayWorld->GetGameInstance();
		UPRPlayerProfileSubsystem* Profile = GameInstance ? GameInstance->GetSubsystem<UPRPlayerProfileSubsystem>() : nullptr;
		UPRDirectorSubsystem* Director = GameInstance ? GameInstance->GetSubsystem<UPRDirectorSubsystem>() : nullptr;
		if (!Profile) { LastWaitReason = TEXT("PlayerProfileSubsystem"); return true; }
		if (!Director) { LastWaitReason = TEXT("DirectorSubsystem"); return true; }
		LastWaitReason = TEXT("DirectorSmoke");

#if WITH_DEV_AUTOMATION_TESTS
		if (Stage == 0)
		{
			Profile->BeginProfileSessionForAutomation();
			InjectFixedStableSamples(Profile);
			FPRPlayerProfileSnapshot Snapshot;
			if (!Profile->GetSnapshot(Snapshot) || Snapshot.SkillMetrics.Num() != 1 || Snapshot.Resources.DamageDealt != 10.0f || Snapshot.QTEResultCounts.Num() != 1 || Snapshot.Relationships.Num() != 1 || !Snapshot.LastDivergence.ResultId.IsValid())
			{
				Fail(TEXT("Fixed Director smoke could not project the stable input samples into a bounded profile."));
				return false;
			}
			FirstSessionId = Snapshot.ProfileSessionId;
			if (FPRHttpDirectorProvider().IsAvailable())
			{
				Fail(TEXT("The HTTP Director provider must remain unavailable without a configured transport or credentials."));
				return false;
			}
			Stage = 1;
			return true;
		}
		if (Stage == 1)
		{
			if (Director->RequestEvaluation(RequestId) != EPRDirectorRequestStatus::Started || !RequestId.IsValid())
			{
				Fail(TEXT("The default deterministic Mock Director could not start a validated evaluation."));
				return false;
			}
			TArray<FPRAppliedDirectorRuleHandle> Handles;
			Director->GetAppliedRules(Handles);
			if (Handles.Num() != 1 || !Handles[0].HandleId.IsValid())
			{
				Fail(TEXT("The validated mock response did not create exactly one applied rule handle."));
				return false;
			}
			AppliedHandle = Handles[0];
			Stage = 2;
			return true;
		}
		if (Stage == 2)
		{
			FPRDirectorRequest Request;
			Request.RequestId = RequestId;
			FPRDirectorResponse InvalidResponse;
			InvalidResponse.RequestId = FGuid::NewGuid();
			FPRDirectorValidationResult Validation;
			if (UPRDirectorResponseValidator::ValidateRequestIdentity(Request, InvalidResponse, Validation) || Validation.Result != EPRDirectorEvaluationResult::RejectedRequestId)
			{
				Fail(TEXT("A mismatched Director request id was not rejected before apply."));
				return false;
			}
			if (Director->RemoveAppliedRule(AppliedHandle) != EPRDirectorRuleOperationResult::Removed || Director->RemoveAppliedRule(AppliedHandle) != EPRDirectorRuleOperationResult::NotFound)
			{
				Fail(TEXT("Applied Director rule removal was not exact and idempotent."));
				return false;
			}
			Stage = 3;
			return true;
		}
		if (Stage == 3)
		{
			Profile->BeginProfileSessionForAutomation();
			FPRPlayerProfileSnapshot ResetSnapshot;
			if (!Profile->GetSnapshot(ResetSnapshot) || ResetSnapshot.ProfileSessionId == FirstSessionId || ResetSnapshot.SkillMetrics.Num() != 0 || ResetSnapshot.Resources.DamageDealt != 0.0f)
			{
				Fail(TEXT("A new profile session did not clear the bounded Director profile projection."));
				return false;
			}
			TArray<FPRAppliedDirectorRuleHandle> Handles;
			Director->GetAppliedRules(Handles);
			if (Handles.Num() != 0)
			{
				Fail(TEXT("Director smoke left an applied rule handle after exact removal."));
				return false;
			}
			Succeed(TEXT("{\"status\":\"PASS\",\"profileStableSamples\":true,\"mockDeterministic\":true,\"httpDisabled\":true,\"invalidRequestRejected\":true,\"exactHandleRemoval\":true,\"profileSessionReset\":true,\"saveTouched\":false,\"runtimeClean\":true}"));
			return false;
		}
#else
		Fail(TEXT("RunPIEDirectorSmoke requires WITH_DEV_AUTOMATION_TESTS."));
		return false;
#endif
		return true;
	}

	static void InjectFixedStableSamples(UPRPlayerProfileSubsystem* Profile)
	{
		FPRAbilityLifecycleEvent Ability;
		Ability.EventType = EPRAbilityLifecycleEventType::Committed;
		Ability.AbilityState.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
		Profile->InjectAbilityLifecycleForAutomation(Ability);

		FPRCombatEvent Combat;
		Combat.EventId = FGuid::NewGuid();
		Combat.SourceId = TEXT("Player");
		Combat.TargetId = TEXT("Enemy.DirectorSmoke");
		Combat.HealthDamage = 10.0f;
		Profile->InjectCombatEventForAutomation(Combat);

		FPRQTEResult QTE;
		QTE.ResultId = FGuid::NewGuid();
		QTE.ResultTag = FGameplayTag::RequestGameplayTag(TEXT("QTE.Result.Success"));
		QTE.TimingGrade = EPRQTETimingGrade::Perfect;
		Profile->InjectQTEResultForAutomation(QTE);

		FPRRelationshipChangedEvent Relationship;
		Relationship.CompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"));
		Relationship.CurrentState.Trust = 60;
		Relationship.CurrentState.Affection = 55;
		Profile->InjectRelationshipChangedForAutomation(Relationship);

		FPRDivergenceResult Divergence;
		Divergence.ResultId = FGuid::NewGuid();
		Divergence.CompanionId = Relationship.CompanionId;
		Divergence.Choice = EPRDivergenceChoice::Rescue;
		Divergence.Resolution = EPRDivergenceResolution::Applied;
		Divergence.FutureDisposition = EPRDivergenceFutureDisposition::RescueEvacuationRequested;
		Profile->InjectDivergenceResultForAutomation(Divergence);
	}

	void Succeed(const FString& Value) { Result->SetValue(Value); bComplete = true; }
	void Fail(const FString& Error) { Result->SetError(Error); bComplete = true; }

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	FGuid FirstSessionId;
	FGuid RequestId;
	FPRAppliedDirectorRuleHandle AppliedHandle;
	double StartedAt = 0.0;
	int32 Stage = 0;
	FString LastWaitReason;
	bool bComplete = false;
};
}

namespace PRDirectorRuleAutomation
{
class FRunner final : public TSharedFromThis<FRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FRunner> Runner = MakeShared<FRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!Runner->World.IsValid() || Runner->World->GetNetMode() == NM_Client)
		{
			Runner->Fail(TEXT("RunPIEDirectorRulesSmoke requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		Runner->StartedAt = Runner->World->GetTimeSeconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float)
		{
			return KeepAlive->Tick() && !KeepAlive->bComplete;
		}));
		return Runner->Result.Get();
	}

private:
	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		if (!PlayWorld || PlayWorld->GetTimeSeconds() - StartedAt > 20.0)
		{
			Fail(FString::Printf(TEXT("RunPIEDirectorRulesSmoke timed out (stage=%d, wait=%s)."), Stage, *LastWaitReason));
			return false;
		}
		UPRDirectorSubsystem* Director = PlayWorld->GetGameInstance() ? PlayWorld->GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr;
		if (!Director) { LastWaitReason = TEXT("DirectorSubsystem"); return true; }
		LastWaitReason = TEXT("DirectorRulesSmoke");

#if !UE_BUILD_SHIPPING
		if (Stage == 0)
		{
			RuleIds = FPRDirectorRuleEffectContract::GetRequiredRuleIds();
			if (RuleIds.Num() != 12) { Fail(TEXT("The fixed Director rule smoke expected exactly twelve whitelisted RuleIds.")); return false; }
			Stage = 1;
			return true;
		}
		if (Stage == 1)
		{
			const FGameplayTag RuleId = RuleIds[RuleIndex];
			FGuid HandleId;
			const EPRDirectorRuleOperationResult OperationResult = Director->ApplyWhitelistedRuleForDevelopment(RuleId, 1, HandleId);
			FPRDirectorRuleRuntimeState State;
			if ((OperationResult != EPRDirectorRuleOperationResult::Applied && OperationResult != EPRDirectorRuleOperationResult::AlreadyApplied)
				|| !HandleId.IsValid() || !Director->GetRuleRuntimeState(RuleId, State) || State.HandleId != HandleId)
			{
				Fail(FString::Printf(TEXT("Fixed Director rule application/readback failed for %s."), *RuleId.ToString()));
				return false;
			}
			++RuleIndex;
			if (RuleIndex >= RuleIds.Num()) Stage = 2;
			return true;
		}
		if (Stage == 2)
		{
			TArray<FPRAppliedDirectorRuleHandle> Handles;
			Director->GetAppliedRules(Handles);
			if (Handles.Num() != 12) { Fail(TEXT("Fixed Director rule smoke did not retain exactly twelve applied handles.")); return false; }
			for (const FPRAppliedDirectorRuleHandle& Handle : Handles)
			{
				if (Director->RemoveAppliedRule(Handle) != EPRDirectorRuleOperationResult::Removed)
				{
					Fail(FString::Printf(TEXT("Fixed Director rule removal failed for %s."), *Handle.RuleId.ToString()));
					return false;
				}
			}
			Stage = 3;
			return true;
		}
		if (Stage == 3)
		{
			TArray<FPRAppliedDirectorRuleHandle> Handles;
			TArray<FPRDirectorRuleRuntimeState> States;
			Director->GetAppliedRules(Handles);
			Director->GetRuleRuntimeStates(States);
			if (Handles.Num() != 0 || States.Num() != 0) { Fail(TEXT("Fixed Director rule smoke left handles or runtime state after exact removal.")); return false; }
			Succeed(TEXT("{\"status\":\"PASS\",\"twelveRulesApplied\":true,\"validatorPath\":true,\"exactRemoval\":true,\"runtimeClean\":true,\"saveTouched\":false}"));
			return false;
		}
#else
		Fail(TEXT("RunPIEDirectorRulesSmoke is unavailable in Shipping."));
		return false;
#endif
		return true;
	}

	void Succeed(const FString& Value) { Result->SetValue(Value); bComplete = true; }
	void Fail(const FString& Error) { Result->SetError(Error); bComplete = true; }

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	TArray<FGameplayTag> RuleIds;
	double StartedAt = 0.0;
	int32 Stage = 0;
	int32 RuleIndex = 0;
	FString LastWaitReason;
	bool bComplete = false;
};
}

namespace PRDirectorResourceAutomation
{
class FRunner final : public TSharedFromThis<FRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FRunner> Runner = MakeShared<FRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!Runner->World.IsValid() || Runner->World->GetNetMode() == NM_Client)
		{
			Runner->Fail(TEXT("RunPIEDirectorResourceBalanceSmoke requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		Runner->StartedAt = FPlatformTime::Seconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float)
		{
			return KeepAlive->Tick() && !KeepAlive->bComplete;
		}));
		return Runner->Result.Get();
	}

private:
	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		if (!PlayWorld || FPlatformTime::Seconds() - StartedAt > 15.0)
		{
			Fail(FString::Printf(TEXT("RunPIEDirectorResourceBalanceSmoke timed out (stage=%d)."), Stage));
			return false;
		}
		UPRDirectorSubsystem* Director = PlayWorld->GetGameInstance() ? PlayWorld->GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr;
		APlayerController* Controller = PlayWorld->GetFirstPlayerController();
		APRPlayerState* PlayerState = Controller ? Controller->GetPlayerState<APRPlayerState>() : nullptr;
		UPRAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetProjectRAbilitySystemComponent() : nullptr;
		if (!Director || !ASC) return true;
		const FGameplayTag Resource = FGameplayTag::RequestGameplayTag(TEXT("Rule.ResourceBalance"), false);

#if !UE_BUILD_SHIPPING
		if (Stage == 0)
		{
			ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxEnergyAttribute(), 100.0f);
			ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), 100.0f);
			const EPRDirectorRuleOperationResult ApplyResult = Director->ApplyWhitelistedRuleForDevelopment(Resource, 1, HandleId);
			FPRDirectorRuleRuntimeState State;
			if ((ApplyResult != EPRDirectorRuleOperationResult::Applied && ApplyResult != EPRDirectorRuleOperationResult::AlreadyApplied)
				|| !HandleId.IsValid() || !Director->GetRuleRuntimeState(Resource, State)
				|| State.Status != EPRDirectorRuleRuntimeStatus::Suspended)
			{
				Fail(TEXT("ResourceBalance did not begin in its required suspended, high-energy observation state."));
				return false;
			}
			Stage = 1;
			return true;
		}
		if (Stage == 1)
		{
			FPRDirectorRuleRuntimeState State;
			if (!Director->GetRuleRuntimeState(Resource, State) || State.Status != EPRDirectorRuleRuntimeStatus::Degraded)
			{
				return true;
			}
			Stage = 2;
			ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), 70.0f);
			return true;
		}
		if (Stage == 2)
		{
			FPRDirectorRuleRuntimeState State;
			if (!Director->GetRuleRuntimeState(Resource, State) || State.Status != EPRDirectorRuleRuntimeStatus::Countered)
			{
				return true;
			}
			TArray<FPRAppliedDirectorRuleHandle> Handles;
			Director->GetAppliedRules(Handles);
			const FPRAppliedDirectorRuleHandle* ExactHandle = Handles.FindByPredicate([this](const FPRAppliedDirectorRuleHandle& Candidate) { return Candidate.HandleId == HandleId; });
			if (!ExactHandle || Director->RemoveAppliedRule(*ExactHandle) != EPRDirectorRuleOperationResult::Removed)
			{
				Fail(TEXT("ResourceBalance could not remove its exact applied handle after the counter condition."));
				return false;
			}
			Stage = 3;
			return true;
		}
		if (Stage == 3)
		{
			FPRDirectorRuleRuntimeState State;
			if (Director->GetRuleRuntimeState(Resource, State))
			{
				Fail(TEXT("ResourceBalance left a runtime state after exact removal."));
				return false;
			}
			Succeed(TEXT("{\"status\":\"PASS\",\"highEnergyDelay\":true,\"maxEnergyEffect\":true,\"energySpendCounter\":true,\"exactRemoval\":true,\"runtimeClean\":true,\"saveTouched\":false}"));
			return false;
		}
#else
		Fail(TEXT("RunPIEDirectorResourceBalanceSmoke is unavailable in Shipping."));
		return false;
#endif
		return true;
	}

	void Succeed(const FString& Value) { Result->SetValue(Value); bComplete = true; }
	void Fail(const FString& Error) { Result->SetError(Error); bComplete = true; }

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	FGuid HandleId;
	double StartedAt = 0.0;
	int32 Stage = 0;
	bool bComplete = false;
};
}

namespace PRDirectorRulePreview
{
class FRunner final : public TSharedFromThis<FRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FRunner> Runner = MakeShared<FRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!Runner->World.IsValid() || Runner->World->GetNetMode() == NM_Client)
		{
			Runner->Result->SetError(TEXT("StartPIEDirectorRulePreview requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float)
		{
			return KeepAlive->Tick() && !KeepAlive->bComplete;
		}));
		return Runner->Result.Get();
	}

private:
	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		UPRDirectorSubsystem* Director = PlayWorld && PlayWorld->GetGameInstance()
			? PlayWorld->GetGameInstance()->GetSubsystem<UPRDirectorSubsystem>() : nullptr;
		if (!Director) return true;
		FGuid HandleId;
		const FGameplayTag RuleId = FGameplayTag::RequestGameplayTag(TEXT("Rule.RepetitionPenalty"), false);
		const EPRDirectorRuleOperationResult ApplyResult = Director->ApplyWhitelistedRuleForDevelopment(RuleId, 2, HandleId);
		FPRDirectorRuleRuntimeState State;
		if ((ApplyResult != EPRDirectorRuleOperationResult::Applied && ApplyResult != EPRDirectorRuleOperationResult::AlreadyApplied)
			|| !HandleId.IsValid() || !Director->GetRuleRuntimeState(RuleId, State))
		{
			Result->SetError(TEXT("Fixed Director preview could not apply or read RepetitionPenalty through the validator path."));
		}
		else
		{
			Result->SetValue(TEXT("{\"status\":\"PASS\",\"previewRule\":\"Rule.RepetitionPenalty\",\"level\":2,\"cleanup\":\"StopPIE\",\"saveTouched\":false}"));
		}
		bComplete = true;
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	bool bComplete = false;
};
}

UToolCallAsyncResultString* UPRDirectorAutomationToolset::RunPIEDirectorSmoke()
{
	return PRDirectorAutomation::FRunner::Start();
}

UToolCallAsyncResultString* UPRDirectorAutomationToolset::RunPIEDirectorRulesSmoke()
{
	return PRDirectorRuleAutomation::FRunner::Start();
}

UToolCallAsyncResultString* UPRDirectorAutomationToolset::RunPIEDirectorResourceBalanceSmoke()
{
	return PRDirectorResourceAutomation::FRunner::Start();
}

UToolCallAsyncResultString* UPRDirectorAutomationToolset::StartPIEDirectorRulePreview()
{
	return PRDirectorRulePreview::FRunner::Start();
}
