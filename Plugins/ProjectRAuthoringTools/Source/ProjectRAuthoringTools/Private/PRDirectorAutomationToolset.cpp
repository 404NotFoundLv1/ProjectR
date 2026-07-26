// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDirectorAutomationToolset.h"

#include "Abilities/PRAbilityTypes.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRRelationshipTypes.h"
#include "Director/PRDirectorResponseValidator.h"
#include "Director/PRDirectorSubsystem.h"
#include "Director/PRHttpDirectorProvider.h"
#include "Director/PRPlayerProfileSubsystem.h"
#include "Divergence/PRDivergenceTypes.h"
#include "Editor.h"
#include "Engine/World.h"
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
		if (!PlayWorld || PlayWorld->GetTimeSeconds() - StartedAt > 15.0)
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

UToolCallAsyncResultString* UPRDirectorAutomationToolset::RunPIEDirectorSmoke()
{
	return PRDirectorAutomation::FRunner::Start();
}
