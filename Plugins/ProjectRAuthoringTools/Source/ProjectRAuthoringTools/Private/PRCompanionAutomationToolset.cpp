// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRCompanionAutomationToolset.h"

#include "Companions/PRCompanionComponent.h"
#include "Companions/PRCompanionPawn.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRRelationshipTypes.h"
#include "Editor.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRCompanionAutomationToolset
{
class FPIECompanionCombatRunner final : public TSharedFromThis<FPIECompanionCombatRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FPIECompanionCombatRunner> Runner = MakeShared<FPIECompanionCombatRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!World || World->GetNetMode() == NM_Client)
		{
			Runner->Fail(TEXT("RunPIECompanionCombatSmoke requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		Runner->World = World;
		Runner->StartedAt = World->GetTimeSeconds();
		if (UPRCompanionRuntimeSubsystem* Runtime = World->GetSubsystem<UPRCompanionRuntimeSubsystem>())
		{
			Runner->SupportHandle = Runtime->OnSupportEvent().AddLambda([WeakRunner = Runner.ToWeakPtr()](const FPRCompanionSupportEvent& Event)
			{
				if (const TSharedPtr<FPIECompanionCombatRunner> PinnedRunner = WeakRunner.Pin())
				{
					PinnedRunner->OnSupport(Event);
				}
			});
		}
		else
		{
			Runner->Fail(TEXT("Companion RuntimeSubsystem is unavailable in PIE."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([RunnerPtr = Runner.ToSharedPtr()](float)
		{
			return RunnerPtr->Tick();
		}));
		return Runner->Result.Get();
	}

private:
	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		UGameInstance* GameInstance = PlayWorld ? PlayWorld->GetGameInstance() : nullptr;
		UPRCompanionSubsystem* Companions = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
		UPRCompanionRuntimeSubsystem* Runtime = PlayWorld ? PlayWorld->GetSubsystem<UPRCompanionRuntimeSubsystem>() : nullptr;
		APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		if (!PlayWorld || !Companions || !Runtime || !Player)
		{
			Fail(TEXT("Fixed Companion PIE smoke lost its required world state.")); return false;
		}
		if (PlayWorld->GetTimeSeconds() - StartedAt > 32.0)
		{
			const FPRCompanionSyncState SyncState = Companions->GetSyncState();
			const APRCompanionPawn* ActivePawn = Runtime->GetActiveCompanionPawn();
			Fail(FString::Printf(TEXT("Fixed Companion PIE smoke timed out at stage %d (primary=%s, activePawn=%s, activeId=%s)."),
				Stage,
				*SyncState.PrimaryCompanionId.ToString(),
				ActivePawn ? *ActivePawn->GetName() : TEXT("None"),
				ActivePawn ? *ActivePawn->GetCompanionId().ToString() : TEXT("None"))
				+ FString::Printf(TEXT(" LastSupport=[Axiom:%d, Kindle:%d, Null:%d]."), LastAxiomResult, LastKindleResult, LastNullResult));
			return false;
		}
		if (Stage == 0)
		{
			if (Companions->SelectPrimaryCompanion(FPRCompanionContract::AxiomTag()) == EPRPrimaryCompanionSelectionResult::RejectedRegistryUnavailable) return true;
			if (!VerifySinglePawn(*Companions, *Runtime, FPRCompanionContract::AxiomTag())) return true;
			Stage = 1; StageStarted = PlayWorld->GetTimeSeconds(); return true;
		}
		if (Stage == 1 && SawAxiom)
		{
			Companions->SelectPrimaryCompanion(FPRCompanionContract::KindleTag());
			Stage = 2; StageStarted = PlayWorld->GetTimeSeconds(); return true;
		}
		if (Stage == 2)
		{
			if (!VerifySinglePawn(*Companions, *Runtime, FPRCompanionContract::KindleTag())) return true;
			if (!SpawnEnemy(*PlayWorld, *Player, TEXT("Enemy.Type.MeleeMinion"))) return true;
			Stage = 3; StageStarted = PlayWorld->GetTimeSeconds(); return true;
		}
		if (Stage == 3 && SawKindle)
		{
			Companions->SelectPrimaryCompanion(FPRCompanionContract::NullTag());
			Stage = 4; StageStarted = PlayWorld->GetTimeSeconds(); return true;
		}
		if (Stage == 4)
		{
			if (!VerifySinglePawn(*Companions, *Runtime, FPRCompanionContract::NullTag())) return true;
			Stage = 5; return true;
		}
		if (Stage == 5 && SawNull)
		{
			Finish(); return false;
		}
		return true;
	}

	bool VerifySinglePawn(const UPRCompanionSubsystem& Companions, const UPRCompanionRuntimeSubsystem& Runtime, const FGameplayTag Expected) const
	{
		APRCompanionPawn* Pawn = Runtime.GetActiveCompanionPawn();
		const FPRCompanionSyncState State = Companions.GetSyncState();
		return Pawn && Pawn->GetCompanionId().MatchesTagExact(Expected)
			&& State.PrimaryCompanionId.MatchesTagExact(Expected) && State.BackgroundCompanionIds.Num() == 2;
	}
	bool SpawnEnemy(UWorld& PlayWorld, APawn& Player, const TCHAR* TagName)
	{
		if (Enemy.IsValid()) return true;
		UPREnemySubsystem* Enemies = PlayWorld.GetSubsystem<UPREnemySubsystem>();
		if (!Enemies || !Enemies->IsRegistryReady()) return false;
		APREnemyCharacter* Spawned = nullptr;
		const EPREnemySpawnStatus Status = Enemies->SpawnEnemyPrototype(FGameplayTag::RequestGameplayTag(TagName),
			FTransform(Player.GetActorRotation(), Player.GetActorLocation() + FVector(420.0f, 0.0f, 0.0f)), SpawnId, Spawned);
		if (Status == EPREnemySpawnStatus::NotReady) return false;
		if (Status != EPREnemySpawnStatus::Spawned || !Spawned || !SpawnId.IsValid())
		{
			Fail(TEXT("Fixed Companion PIE smoke could not spawn its formal Melee target."));
			return false;
		}
		Enemy = Spawned; return true;
	}
	void OnSupport(const FPRCompanionSupportEvent& Event)
	{
		if (Event.CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag())) LastAxiomResult = static_cast<int32>(Event.Result);
		if (Event.CompanionId.MatchesTagExact(FPRCompanionContract::KindleTag())) LastKindleResult = static_cast<int32>(Event.Result);
		if (Event.CompanionId.MatchesTagExact(FPRCompanionContract::NullTag())) LastNullResult = static_cast<int32>(Event.Result);
		SawAxiom |= Event.CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag()) && Event.Result == EPRCompanionSupportResult::Applied;
		SawKindle |= Event.CompanionId.MatchesTagExact(FPRCompanionContract::KindleTag()) && Event.Result == EPRCompanionSupportResult::Applied && Event.AppliedMagnitude > 0.0f;
		SawNull |= Event.CompanionId.MatchesTagExact(FPRCompanionContract::NullTag()) && Event.Result == EPRCompanionSupportResult::Applied;
	}
	void Cleanup()
	{
		if (UWorld* PlayWorld = World.Get())
		{
			if (UPRCompanionRuntimeSubsystem* Runtime = PlayWorld->GetSubsystem<UPRCompanionRuntimeSubsystem>(); Runtime && SupportHandle.IsValid()) Runtime->OnSupportEvent().Remove(SupportHandle);
			if (UPREnemySubsystem* Enemies = PlayWorld->GetSubsystem<UPREnemySubsystem>(); Enemies && SpawnId.IsValid()) Enemies->DespawnEnemy(SpawnId);
		}
		SupportHandle.Reset(); SpawnId.Invalidate(); Enemy.Reset();
	}
	void Finish()
	{
		// The selected primary Companion is deliberately retained for the remainder of this PIE
		// session.  World teardown, rather than this runner, owns that Pawn's lifecycle.
		Cleanup();
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"singlePrimaryPawn\":true,\"backgroundLogicalOnly\":true,\"axiomShield\":true,\"kindleDamage\":true,\"nullControl\":true,\"runnerTransientClean\":true,\"requiresStopPIEForWorldCleanup\":true}"));
	}
	void Fail(const FString& Message) { Cleanup(); Result->SetError(Message); }

	TWeakObjectPtr<UWorld> World; TStrongObjectPtr<UToolCallAsyncResultString> Result; FDelegateHandle SupportHandle; TWeakObjectPtr<APREnemyCharacter> Enemy; FGuid SpawnId; double StartedAt = 0.0, StageStarted = 0.0; int32 Stage = 0; int32 LastAxiomResult = -1, LastKindleResult = -1, LastNullResult = -1; bool SawAxiom = false, SawKindle = false, SawNull = false;
};

class FPIECompanionSyncRunner final : public TSharedFromThis<FPIECompanionSyncRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FPIECompanionSyncRunner> Runner = MakeShared<FPIECompanionSyncRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}

		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](const float DeltaSeconds) mutable
			{
				const bool bContinue = RunnerPtr->Tick(DeltaSeconds);
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

private:
	bool Initialize()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			return Fail(TEXT("RunPIECompanionSyncSmoke requires an active authoritative in-process PIE world."));
		}

		ElapsedSeconds = 0.0f;
		return true;
	}

	bool Tick(const float DeltaSeconds)
	{
		ElapsedSeconds += DeltaSeconds;
		if (ElapsedSeconds > 20.0f)
		{
			Fail(bTravelStarted
				? TEXT("Fixed Companion PIE travel did not reach BossGym within 20 seconds.")
				: TEXT("The fixed Companion registry did not become ready within 20 seconds of PIE startup."));
			return false;
		}

		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld))
		{
			return true;
		}

		UGameInstance* GameInstance = PlayWorld->GetGameInstance();
		UPRCompanionSubsystem* CompanionSubsystem = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
		if (!bTravelStarted)
		{
			if (!CompanionSubsystem || !CompanionSubsystem->IsRegistryReady())
			{
				return true;
			}
			if (CompanionSubsystem->SelectPrimaryCompanion(FPRCompanionContract::AxiomTag()) != EPRPrimaryCompanionSelectionResult::Selected
				|| !HasExpectedAxiomSync(*CompanionSubsystem))
			{
				Fail(TEXT("The fixed Axiom primary selection did not derive the canonical two backgrounds."));
				return false;
			}
			bTravelStarted = true;
			UGameplayStatics::OpenLevel(PlayWorld, FName(TEXT("/Game/ProjectR/Maps/L_BossGym")));
			return true;
		}

		if (!PlayWorld->GetMapName().Contains(TEXT("L_BossGym")))
		{
			return true;
		}

		if (!CompanionSubsystem || !CompanionSubsystem->IsRegistryReady() || !HasExpectedAxiomSync(*CompanionSubsystem))
		{
			Fail(TEXT("Companion run-local selection did not persist through fixed BossGym travel."));
			return false;
		}

		TArray<FPRCompanionRelationshipRecord> Records;
		CompanionSubsystem->GetAllRelationshipSnapshots(Records);
		if (!FPRCompanionContract::AreCanonicalRelationshipRecords(Records))
		{
			Fail(TEXT("Companion relationship snapshots are not canonical after fixed PIE travel."));
			return false;
		}

		Result->SetValue(TEXT("{\"status\":\"PASS\",\"primary\":\"Companion.Axiom\",\"backgroundCount\":2,\"travel\":\"L_BossGym\",\"saveIO\":false,\"packagesSaved\":false}"));
		return false;
	}

	bool HasExpectedAxiomSync(const UPRCompanionSubsystem& CompanionSubsystem) const
	{
		const FPRCompanionSyncState State = CompanionSubsystem.GetSyncState();
		return State.PrimaryCompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag())
			&& State.BackgroundCompanionIds.Num() == 2
			&& State.BackgroundCompanionIds[0].MatchesTagExact(FPRCompanionContract::KindleTag())
			&& State.BackgroundCompanionIds[1].MatchesTagExact(FPRCompanionContract::NullTag());
	}

	bool Fail(const FString& Message)
	{
		Result->SetError(Message);
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	float ElapsedSeconds = 0.0f;
	bool bTravelStarted = false;
};
} // namespace PRCompanionAutomationToolset

UToolCallAsyncResultString* UPRCompanionAutomationToolset::RunPIECompanionSyncSmoke()
{
	return PRCompanionAutomationToolset::FPIECompanionSyncRunner::Start();
}

UToolCallAsyncResultString* UPRCompanionAutomationToolset::RunPIECompanionCombatSmoke()
{
	return PRCompanionAutomationToolset::FPIECompanionCombatRunner::Start();
}
