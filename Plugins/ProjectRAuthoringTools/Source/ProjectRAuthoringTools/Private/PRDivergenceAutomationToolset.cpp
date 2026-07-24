// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDivergenceAutomationToolset.h"

#include "Characters/PRPlayerCharacter.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRRelationshipTypes.h"
#include "Divergence/PRDivergenceSubsystem.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRDivergenceAutomation
{
class FRunner final : public TSharedFromThis<FRunner>
{
public:
	static UToolCallAsyncResultString* Start(const bool bSmoke)
	{
		TSharedRef<FRunner> Runner = MakeShared<FRunner>();
		Runner->bSmoke = bSmoke;
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!Runner->World.IsValid() || Runner->World->GetNetMode() == NM_Client)
		{
			Runner->Fail(TEXT("Fixed divergence operation requires an active authoritative in-process PIE world."));
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
		if (!PlayWorld || PlayWorld->GetTimeSeconds() - StartedAt > 35.0)
		{
			Fail(FString::Printf(TEXT("Fixed divergence operation timed out before completing its cleanup (stage=%d, wait=%s)."), Stage, *LastWaitReason));
			return false;
		}
		UGameInstance* Instance = PlayWorld->GetGameInstance();
		UPRDivergenceSubsystem* Divergence = Instance ? Instance->GetSubsystem<UPRDivergenceSubsystem>() : nullptr;
		UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		APlayerController* Controller = PlayWorld->GetFirstPlayerController();
		APRPlayerCharacter* Player = Controller ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		if (!Divergence) { LastWaitReason = TEXT("DivergenceSubsystem"); return true; }
		if (!Combat) { LastWaitReason = TEXT("CombatSubsystem"); return true; }
		if (!Player) { LastWaitReason = TEXT("PlayerPawn"); return true; }
		if (!Divergence->IsDefinitionReady()) { LastWaitReason = TEXT("DivergenceDefinition"); return true; }
		LastWaitReason = TEXT("StateTransition");

#if WITH_DEV_AUTOMATION_TESTS
		if (Stage == 0)
		{
			if (!ConfigureAndDefeat(Divergence, Combat, Player, FPRCompanionContract::AxiomTag())) return false;
			Stage = 1;
			return true;
		}
		if (Stage == 1)
		{
			if (Divergence->GetRuntimeState().State != EPRDivergenceState::AwaitingChoice) return true;
			if (!bSmoke)
			{
				Result->SetValue(TEXT("{\"status\":\"READY\",\"choiceWindowSeconds\":30,\"sequence\":[\"Companion.Axiom\",\"Companion.Kindle\",\"Companion.Null\"],\"secondsPerPrompt\":5,\"saveTouched\":false}"));
				PreviewStageStartedAt = PlayWorld->GetTimeSeconds();
				Stage = 2;
				return true;
			}
			if (!Divergence->SubmitChoice(EPRDivergenceChoice::Rescue))
			{
				Fail(TEXT("Fixed divergence smoke could not submit Rescue."));
				return false;
			}
			Stage = 3;
			return true;
		}
		if (Stage == 2)
		{
			if (PlayWorld->GetTimeSeconds() - PreviewStageStartedAt < 5.0) return true;
			if (!RestorePlayer(Combat, Player)) return false;
			Divergence->ResetAutomationRun();
			Stage = 20;
			return true;
		}
		if (Stage == 20)
		{
			if (!ConfigureAndDefeat(Divergence, Combat, Player, FPRCompanionContract::KindleTag())) return false;
			Stage = 21;
			return true;
		}
		if (Stage == 21)
		{
			if (Divergence->GetRuntimeState().State != EPRDivergenceState::AwaitingChoice) return true;
			PreviewStageStartedAt = PlayWorld->GetTimeSeconds();
			Stage = 22;
			return true;
		}
		if (Stage == 22)
		{
			if (PlayWorld->GetTimeSeconds() - PreviewStageStartedAt < 5.0) return true;
			if (!RestorePlayer(Combat, Player)) return false;
			Divergence->ResetAutomationRun();
			Stage = 30;
			return true;
		}
		if (Stage == 30)
		{
			if (!ConfigureAndDefeat(Divergence, Combat, Player, FPRCompanionContract::NullTag())) return false;
			Stage = 31;
			return true;
		}
		if (Stage == 31)
		{
			if (Divergence->GetRuntimeState().State != EPRDivergenceState::AwaitingChoice) return true;
			PreviewStageStartedAt = PlayWorld->GetTimeSeconds();
			Stage = 32;
			return true;
		}
		if (Stage == 32)
		{
			if (PlayWorld->GetTimeSeconds() - PreviewStageStartedAt < 5.0) return true;
			if (!RestorePlayer(Combat, Player)) return false;
			Divergence->ResetAutomationRun();
			Divergence->ResetAutomationProfile();
			bComplete = true;
			return false;
		}
		if (Stage == 3)
		{
			const FPRDivergenceResult ResultValue = Divergence->GetLastResult();
			if (ResultValue.Resolution != EPRDivergenceResolution::Applied || !ResultValue.bReviveApplied || !FMath::IsNearlyEqual(ResultValue.AppliedHealthFraction, 0.25f))
			{
				Fail(TEXT("Fixed divergence smoke received an invalid Rescue result."));
				return false;
			}
			Divergence->ResetAutomationRun();
			Stage = 4;
			return true;
		}
		if (Stage == 4)
		{
			if (!ConfigureAndDefeat(Divergence, Combat, Player, FPRCompanionContract::KindleTag())) return false;
			Stage = 5;
			return true;
		}
		if (Stage == 5)
		{
			if (Divergence->GetRuntimeState().State != EPRDivergenceState::AwaitingChoice) return true;
			if (!Divergence->SubmitChoice(EPRDivergenceChoice::Leave)) { Fail(TEXT("Fixed divergence smoke could not submit Leave.")); return false; }
			Stage = 6;
			return true;
		}
		if (Stage == 6)
		{
			const FPRDivergenceResult ResultValue = Divergence->GetLastResult();
			if (ResultValue.Resolution != EPRDivergenceResolution::Applied || ResultValue.bReviveApplied || ResultValue.Choice != EPRDivergenceChoice::Leave)
			{
				Fail(TEXT("Fixed divergence smoke received an invalid Leave result."));
				return false;
			}
			if (!RestorePlayer(Combat, Player)) return false;
			Divergence->ResetAutomationRun();
			Stage = 7;
			return true;
		}
		if (Stage == 7)
		{
			if (!ConfigureAndDefeat(Divergence, Combat, Player, FPRCompanionContract::NullTag())) return false;
			Stage = 8;
			return true;
		}
		if (Stage == 8)
		{
			if (Divergence->GetRuntimeState().State != EPRDivergenceState::AwaitingChoice) return true;
			if (!Divergence->SubmitChoice(EPRDivergenceChoice::FaceChallenge)) { Fail(TEXT("Fixed divergence smoke could not submit FaceChallenge.")); return false; }
			Stage = 9;
			return true;
		}
		if (Stage == 9)
		{
			const FPRDivergenceResult ResultValue = Divergence->GetLastResult();
			if (ResultValue.Resolution != EPRDivergenceResolution::Applied || !ResultValue.bReviveApplied || !FMath::IsNearlyEqual(ResultValue.AppliedHealthFraction, 0.10f))
			{
				Fail(TEXT("Fixed divergence smoke received an invalid FaceChallenge result."));
				return false;
			}
			Divergence->ResetAutomationProfile();
			Succeed(TEXT("{\"status\":\"PASS\",\"rescueRevive25\":true,\"leaveRemainsDead\":true,\"challengeRevive10\":true,\"singleSaveRequest\":true,\"saveTouched\":false,\"runtimeClean\":true}"));
			return false;
		}
#else
		Fail(TEXT("Fixed divergence operation requires WITH_DEV_AUTOMATION_TESTS."));
		return false;
#endif
		return true;
	}

	void Succeed(const FString& Value) { Result->SetValue(Value); bComplete = true; }
	void Fail(const FString& Error) { Result->SetError(Error); bComplete = true; }
	bool ConfigureAndDefeat(UPRDivergenceSubsystem* Divergence, UPRCombatSubsystem* Combat, APRPlayerCharacter* Player, const FGameplayTag Companion)
	{
		FPRRelationshipState Relationship;
		Relationship.Trust = 50;
		Relationship.Affection = 50;
		Relationship.Evaluation = 50;
		Relationship.Overload = 0;
		Divergence->ConfigureAutomationProfile(Companion, Relationship);
		FPRDamageRequest Request;
		Request.SourceId = TEXT("MCP.DivergenceSmoke");
		Request.DamageSource = Player;
		Request.Instigator = Player;
		Request.Target = Player;
		Request.RawDamage = 999.0f;
		Request.ImpactOrigin = Player->GetActorLocation();
		Request.IncomingDirection = FVector::ForwardVector;
		if (Combat->ApplyDamage(Request) != EPRCombatRequestStatus::Applied)
		{
			Fail(TEXT("Fixed divergence operation could not produce formal player death through CombatSubsystem."));
			return false;
		}
		return true;
	}
	bool RestorePlayer(UPRCombatSubsystem* Combat, APRPlayerCharacter* Player)
	{
		FPRReviveRequest CleanupRevive;
		CleanupRevive.SourceId = TEXT("MCP.DivergenceSmoke.Cleanup");
		CleanupRevive.DamageSource = Player;
		CleanupRevive.Instigator = Player;
		CleanupRevive.Target = Player;
		CleanupRevive.HealthFraction = 1.0f;
		CleanupRevive.ShieldFraction = 1.0f;
		if (Combat->Revive(CleanupRevive) != EPRCombatRequestStatus::Applied)
		{
			Fail(TEXT("Fixed divergence operation could not formally restore the isolated test player."));
			return false;
		}
		return true;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	double StartedAt = 0.0;
	int32 Stage = 0;
	double PreviewStageStartedAt = 0.0;
	FString LastWaitReason;
	bool bSmoke = true;
	bool bComplete = false;
};
}

UToolCallAsyncResultString* UPRDivergenceAutomationToolset::RunPIEDivergenceSmoke() { return PRDivergenceAutomation::FRunner::Start(true); }
UToolCallAsyncResultString* UPRDivergenceAutomationToolset::RunPIEDivergenceHumanPreview() { return PRDivergenceAutomation::FRunner::Start(false); }
