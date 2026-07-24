// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDialogueAutomationToolset.h"

#include "Characters/PRPlayerCharacter.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRRelationshipTypes.h"
#include "Dialogue/PRCompanionDialogueSubsystem.h"
#include "Editor.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

#if WITH_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#endif

namespace PRDialogueAutomation
{
class FDialogueSmokeRunner final : public TSharedFromThis<FDialogueSmokeRunner>
{
public:
	static UToolCallAsyncResultString* Start(const bool bAutoFinish)
	{
		TSharedRef<FDialogueSmokeRunner> Runner = MakeShared<FDialogueSmokeRunner>();
		Runner->bAutoFinish = bAutoFinish;
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!Runner->World.IsValid() || Runner->World->GetNetMode() == NM_Client)
		{
			Runner->Fail(TEXT("Fixed dialogue smoke requires an active authoritative in-process PIE world."));
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
			UPRCompanionDialogueSubsystem* TimedOutDialogue = PlayWorld ? PlayWorld->GetSubsystem<UPRCompanionDialogueSubsystem>() : nullptr;
			const FPRDialogueRuntimeState State = TimedOutDialogue ? TimedOutDialogue->GetRuntimeState() : FPRDialogueRuntimeState();
			Fail(FString::Printf(TEXT("Fixed dialogue smoke timed out: auto=%d stage=%d state=%d companion=%s trigger=%d."),
				bAutoFinish, Stage, static_cast<int32>(State.PresentationState), *State.CompanionId.ToString(), static_cast<int32>(State.TriggerKind)));
			return false;
		}
		UGameInstance* Instance = PlayWorld->GetGameInstance();
		UPRCompanionSubsystem* Companions = Instance ? Instance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
		UPRCompanionDialogueSubsystem* Dialogue = PlayWorld->GetSubsystem<UPRCompanionDialogueSubsystem>();
		UPREnemySubsystem* Enemies = PlayWorld->GetSubsystem<UPREnemySubsystem>();
		UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		APlayerController* Controller = PlayWorld->GetFirstPlayerController();
		APRPlayerCharacter* Player = Controller ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		if (!Companions || !Dialogue || !Enemies || !Combat || !Player || !Dialogue->IsRegistryReady() || !Enemies->IsRegistryReady()) return true;
		if (!bAutoFinish) return TickHumanPreview(Companions, Dialogue, Enemies, Combat, Player);
		if (Stage == 0)
		{
			const EPRPrimaryCompanionSelectionResult Selection = Companions->SelectPrimaryCompanion(FPRCompanionContract::AxiomTag());
			if (Selection != EPRPrimaryCompanionSelectionResult::Selected && Selection != EPRPrimaryCompanionSelectionResult::AlreadySelected) { Fail(TEXT("Dialogue smoke could not select Axiom.")); return false; }
			FGuid SpawnId;
			APREnemyCharacter* Spawned = nullptr;
			const EPREnemySpawnStatus SpawnStatus = Enemies->SpawnEnemyPrototype(
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")),
				FTransform(FVector(500.0f, 0.0f, 100.0f)), SpawnId, Spawned);
			if (SpawnStatus != EPREnemySpawnStatus::Spawned || !Spawned) { Fail(TEXT("Dialogue smoke could not spawn its fixed whitelist target.")); return false; }
			SpawnedEnemy = Spawned;
			SpawnIdToClean = SpawnId;
			Stage = 1;
			return true;
		}
		if (Stage == 1)
		{
			FPRDamageRequest Request;
			Request.SourceId = TEXT("MCP.DialogueSmoke");
			Request.DamageSource = Player;
			Request.Instigator = Player;
			Request.Target = Player;
			Request.RawDamage = 135.0f;
			Request.ImpactOrigin = Player->GetActorLocation();
			Request.IncomingDirection = FVector::ForwardVector;
			Request.bCritical = true;
			Combat->ApplyDamage(Request);
			Stage = 2;
			return true;
		}
		const FPRDialogueRuntimeState State = Dialogue->GetRuntimeState();
		if (Stage == 2 && State.PresentationState != EPRDialoguePresentationState::Idle)
		{
			if (State.CompanionId != FPRCompanionContract::AxiomTag() || State.TriggerKind != EPRDialogueTriggerKind::CriticalLowHealth) { Fail(TEXT("Dialogue smoke received an unexpected runtime state.")); return false; }
			if (!bAutoFinish) { Succeed(TEXT("{\"status\":\"PASS\",\"preview\":\"AxiomCriticalBark\",\"saveTouched\":false}")); return false; }
			Dialogue->CancelDialogue();
			Stage = 3;
			return true;
		}
		if (Stage == 3 && State.PresentationState == EPRDialoguePresentationState::Idle)
		{
			Succeed(TEXT("{\"status\":\"PASS\",\"registryReady\":true,\"criticalBark\":true,\"cancelClean\":true,\"saveTouched\":false}"));
			return false;
		}
		return true;
	}

	bool TickHumanPreview(UPRCompanionSubsystem* Companions, UPRCompanionDialogueSubsystem* Dialogue, UPREnemySubsystem* Enemies, UPRCombatSubsystem* Combat, APRPlayerCharacter* Player)
	{
		const FPRDialogueRuntimeState State = Dialogue->GetRuntimeState();
		if (Stage == 0 || Stage == 3 || Stage == 6)
		{
			if (State.PresentationState != EPRDialoguePresentationState::Idle) return true;
			const FGameplayTag Companion = Stage == 0 ? FPRCompanionContract::AxiomTag() : (Stage == 3 ? FPRCompanionContract::KindleTag() : FPRCompanionContract::NullTag());
			const EPRPrimaryCompanionSelectionResult Selection = Companions->SelectPrimaryCompanion(Companion);
			if (Selection != EPRPrimaryCompanionSelectionResult::Selected && Selection != EPRPrimaryCompanionSelectionResult::AlreadySelected)
			{
				Fail(TEXT("Fixed human preview could not select its companion."));
				return false;
			}
			if (Stage == 0)
			{
				FGuid SpawnId;
				APREnemyCharacter* Spawned = nullptr;
				const EPREnemySpawnStatus SpawnStatus = Enemies->SpawnEnemyPrototype(
					FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")), FTransform(FVector(500.0f, 0.0f, 100.0f)), SpawnId, Spawned);
				if (SpawnStatus != EPREnemySpawnStatus::Spawned || !Spawned)
				{
					Fail(TEXT("Fixed human preview could not spawn its fixed whitelist target."));
					return false;
				}
				SpawnedEnemy = Spawned;
				SpawnIdToClean = SpawnId;
			}
			++Stage;
			return true;
		}
		if (Stage == 1 || Stage == 4 || Stage == 7)
		{
			FPRDamageRequest Request;
			Request.SourceId = TEXT("MCP.DialogueSmoke");
			Request.DamageSource = Player;
			Request.Instigator = Player;
			Request.Target = Player;
			Request.RawDamage = Stage == 1 ? 135.0f : 1.0f;
			Request.ImpactOrigin = Player->GetActorLocation();
			Request.IncomingDirection = FVector::ForwardVector;
			Request.bCritical = true;
			Combat->ApplyDamage(Request);
			++Stage;
			return true;
		}
		if (Stage == 2 || Stage == 5 || Stage == 8)
		{
			if (State.PresentationState == EPRDialoguePresentationState::Idle) return true;
			const FGameplayTag Expected = Stage == 2 ? FPRCompanionContract::AxiomTag() : (Stage == 5 ? FPRCompanionContract::KindleTag() : FPRCompanionContract::NullTag());
			if (State.CompanionId != Expected || State.TriggerKind != EPRDialogueTriggerKind::CriticalLowHealth)
			{
				Fail(TEXT("Fixed human preview received an unexpected dialogue runtime state."));
				return false;
			}
			if (Stage == 2 && !bHumanPreviewReadyReported)
			{
				// Human preview must hand control back while Axiom's bark is still on screen.
				// The ticker deliberately remains alive to present Kindle and Null afterwards.
				Result->SetValue(TEXT("{\"status\":\"READY\",\"previewActive\":true,\"preview\":\"AxiomKindleNullCriticalBarks\",\"allThreeVoicesPresented\":false,\"saveTouched\":false}"));
				bHumanPreviewReadyReported = true;
				++Stage;
				return true;
			}
			if (Stage == 8)
			{
				if (!bHumanPreviewReadyReported)
				{
					Succeed(TEXT("{\"status\":\"READY\",\"previewActive\":true,\"preview\":\"AxiomKindleNullCriticalBarks\",\"allThreeVoicesPresented\":true,\"saveTouched\":false}"));
					return false;
				}
				Cleanup();
				bComplete = true;
				return false;
			}
			++Stage;
			return true;
		}
		Fail(TEXT("Fixed human preview entered an invalid stage."));
		return false;
	}

	void Succeed(const FString& Value)
	{
		Cleanup();
		Result->SetValue(Value);
		bComplete = true;
	}
	void Fail(const FString& Error)
	{
		Cleanup();
		Result->SetError(Error);
		bComplete = true;
	}
	void Cleanup()
	{
		if (UWorld* PlayWorld = World.Get()) if (UPREnemySubsystem* Enemies = PlayWorld->GetSubsystem<UPREnemySubsystem>(); SpawnIdToClean.IsValid()) Enemies->DespawnEnemy(SpawnIdToClean);
		SpawnedEnemy.Reset();
		SpawnIdToClean.Invalidate();
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APREnemyCharacter> SpawnedEnemy;
	FGuid SpawnIdToClean;
	double StartedAt = 0.0;
	int32 Stage = 0;
	bool bAutoFinish = true;
	bool bComplete = false;
	bool bHumanPreviewReadyReported = false;
};
}

UToolCallAsyncResultString* UPRDialogueAutomationToolset::RunPIECompanionDialogueSmoke() { return PRDialogueAutomation::FDialogueSmokeRunner::Start(true); }
UToolCallAsyncResultString* UPRDialogueAutomationToolset::PreparePIECompanionDialogueHumanPreview() { return PRDialogueAutomation::FDialogueSmokeRunner::Start(false); }

#if WITH_AUTOMATION_TESTS

namespace PRDialogueAutomationToolsetTests
{
struct FPIERunState
{
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	double StartedAtSeconds = 0.0;
	bool bStarted = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDialogueFixedPIESmokeAutomationTest,
	"ProjectRAuthoringTools.Dialogue.PIE.FixedSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRDialogueFixedPIESmokeAutomationTest::RunTest(const FString& Parameters)
{
	using namespace PRDialogueAutomationToolsetTests;

	const TSharedRef<FPIERunState> State = MakeShared<FPIERunState>();
	State->StartedAtSeconds = FPlatformTime::Seconds();
	AddCommand(new FEditorLoadMap(TEXT("/Game/ProjectR/Maps/L_CombatGym")));
	AddCommand(new FStartPIECommand(false));
	AddCommand(new FFunctionLatentCommand([this, State]()
	{
		if (!GEditor || !GEditor->PlayWorld)
		{
			if (FPlatformTime::Seconds() - State->StartedAtSeconds > 15.0)
			{
				AddError(TEXT("Fixed Dialogue PIE smoke did not start an authoritative PIE world."));
				return true;
			}
			return false;
		}

		if (!State->bStarted)
		{
			State->Result = TStrongObjectPtr<UToolCallAsyncResultString>(UPRDialogueAutomationToolset::RunPIECompanionDialogueSmoke());
			State->bStarted = true;
			TestNotNull(TEXT("Fixed Dialogue PIE runner returns an async result."), State->Result.Get());
			return false;
		}

		if (!State->Result.IsValid())
		{
			AddError(TEXT("Fixed Dialogue PIE runner result became invalid before completion."));
			return true;
		}
		if (!State->Result->bIsComplete)
		{
			if (FPlatformTime::Seconds() - State->StartedAtSeconds > 20.0)
			{
				AddError(TEXT("Fixed Dialogue PIE runner did not complete within twenty seconds."));
				return true;
			}
			return false;
		}

		TestTrue(TEXT("Fixed Dialogue PIE runner completes without an error."), State->Result->Error.IsEmpty());
		TestTrue(TEXT("Fixed Dialogue PIE runner reports a critical-hit bark."), State->Result->Value.Contains(TEXT("\"criticalBark\":true")));
		TestTrue(TEXT("Fixed Dialogue PIE runner cleans its whitelist spawn."), State->Result->Value.Contains(TEXT("\"cancelClean\":true")));
		return true;
	}));
	AddCommand(new FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDialogueHumanPreviewReadinessAutomationTest,
	"ProjectRAuthoringTools.Dialogue.PIE.HumanPreviewReadiness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRDialogueHumanPreviewReadinessAutomationTest::RunTest(const FString& Parameters)
{
	using namespace PRDialogueAutomationToolsetTests;

	const TSharedRef<FPIERunState> State = MakeShared<FPIERunState>();
	State->StartedAtSeconds = FPlatformTime::Seconds();
	AddCommand(new FEditorLoadMap(TEXT("/Game/ProjectR/Maps/L_CombatGym")));
	AddCommand(new FStartPIECommand(false));
	AddCommand(new FFunctionLatentCommand([this, State]()
	{
		if (!GEditor || !GEditor->PlayWorld)
		{
			if (FPlatformTime::Seconds() - State->StartedAtSeconds > 15.0)
			{
				AddError(TEXT("Human dialogue preview did not start an authoritative PIE world."));
				return true;
			}
			return false;
		}

		if (!State->bStarted)
		{
			State->Result = TStrongObjectPtr<UToolCallAsyncResultString>(UPRDialogueAutomationToolset::PreparePIECompanionDialogueHumanPreview());
			State->StartedAtSeconds = FPlatformTime::Seconds();
			State->bStarted = true;
			TestNotNull(TEXT("Human dialogue preview returns an async result."), State->Result.Get());
			return false;
		}

		if (!State->Result.IsValid())
		{
			AddError(TEXT("Human dialogue preview result became invalid before readiness."));
			return true;
		}
		if (!State->Result->bIsComplete)
		{
			if (FPlatformTime::Seconds() - State->StartedAtSeconds > 15.0)
			{
				AddError(TEXT("Human dialogue preview did not report readiness within fifteen seconds."));
				return true;
			}
			return false;
		}

		TestTrue(TEXT("Human preview reports that its first bark is visible."), State->Result->Value.Contains(TEXT("\"previewActive\":true")));
		TestTrue(TEXT("Human preview returns while the first bark is still visible."), FPlatformTime::Seconds() - State->StartedAtSeconds < 3.5);
		return true;
	}));
	AddCommand(new FEndPlayMapCommand());
	return true;
}

#endif
