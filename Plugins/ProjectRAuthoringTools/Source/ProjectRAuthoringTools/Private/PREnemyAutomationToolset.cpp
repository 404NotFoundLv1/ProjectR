// Copyright Epic Games, Inc. All Rights Reserved.

#include "PREnemyAutomationToolset.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyAIController.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemySubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PREnemyAutomationToolset
{
class FCheckpointARunner : public TSharedFromThis<FCheckpointARunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FCheckpointARunner> Runner = MakeShared<FCheckpointARunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Runner->FinishError(TEXT("RunPIEEnemyCheckpointASmoke requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		Runner->World = PlayWorld;
		Runner->StartedAt = PlayWorld->GetTimeSeconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](float)
			{
				return RunnerPtr->Tick();
			}));
		return Runner->Result.Get();
	}

private:
	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		UPREnemySubsystem* Subsystem = PlayWorld ? PlayWorld->GetSubsystem<UPREnemySubsystem>() : nullptr;
		APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
		APawn* PlayerPawn = Controller ? Controller->GetPawn() : nullptr;
		if (!PlayWorld || !Subsystem || !PlayerPawn)
		{
			FinishError(TEXT("Fixed Enemy PIE smoke lost its world, subsystem, or formal player pawn."));
			return false;
		}
		if (!SpawnedEnemy.IsValid())
		{
			APREnemyCharacter* Enemy = nullptr;
			FTransform SpawnTransform(PlayerPawn->GetActorRotation(), PlayerPawn->GetActorLocation() + FVector(130.0f, 0.0f, 0.0f));
			const EPREnemySpawnStatus SpawnStatus = Subsystem->SpawnEnemyPrototype(
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")), SpawnTransform, SpawnId, Enemy);
			if (SpawnStatus == EPREnemySpawnStatus::NotReady)
			{
				if (PlayWorld->GetTimeSeconds() - StartedAt < 5.0)
				{
					return true;
				}
				FinishError(TEXT("Fixed Enemy PIE registry did not become ready within five seconds."));
				return false;
			}
			if (SpawnStatus != EPREnemySpawnStatus::Spawned || !Enemy || !Enemy->IsEnemyInitialized())
			{
				FinishError(TEXT("Fixed Enemy PIE smoke could not spawn initialized MeleeMinion."));
				return false;
			}
			UPRAbilitySystemComponent* ASC = Enemy->GetProjectRAbilitySystemComponent();
			const UPRAttributeSet* Attributes = Enemy->GetAttributeSet();
			const APREnemyAIController* EnemyController = Cast<APREnemyAIController>(Enemy->GetController());
			const bool bValid = ASC && Attributes && ASC->GetOwnerActor() == Enemy && ASC->GetAvatarActor() == Enemy
				&& FMath::IsNearlyEqual(Attributes->GetHealth(), 100.0f)
				&& FMath::IsNearlyEqual(Attributes->GetAttackPower(), 10.0f)
				&& FMath::IsNearlyEqual(Attributes->GetMoveSpeed(), 450.0f)
				&& Enemy->GetAbilityTargetMobility() == EPRAbilityTargetMobility::Light
				&& EnemyController && EnemyController->IsEnemyStateTreeRunning();
			if (!bValid)
			{
				FinishError(TEXT("MeleeMinion GAS owner/avatar, P0 attributes, or mobility contract was invalid."));
				return false;
			}
			UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
			if (!Combat)
			{
				FinishError(TEXT("Fixed Enemy PIE smoke could not resolve the CombatSubsystem."));
				return false;
			}
			SpawnedEnemy = Enemy;
			SpawnedAt = PlayWorld->GetTimeSeconds();
			CombatEventHandle = Combat->OnCombatEvent().AddLambda([this](const FPRCombatEvent& Event)
			{
				if (Event.Instigator.Get() == SpawnedEnemy.Get() && Event.SourceId == TEXT("MeleeStrike"))
				{
					++MeleeEventCount;
					LastMeleeDamage = Event.RawDamage;
				}
			});
			return true;
		}

		APREnemyCharacter* Enemy = SpawnedEnemy.Get();
		UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		if (!Enemy || !Combat)
		{
			FinishError(TEXT("Fixed Enemy PIE smoke lost its spawned enemy or CombatSubsystem."));
			return false;
		}
		if (!bDeathRequested)
		{
			if (MeleeEventCount == 0)
			{
				if (PlayWorld->GetTimeSeconds() - SpawnedAt < 3.0)
				{
					return true;
				}
				const FPREnemyRuntimeState& Runtime = Enemy->GetEnemyBrain()->GetRuntimeState();
				const AActor* Target = Runtime.Target.Get();
				const float XDistance = Target
					? FMath::Abs(Target->GetActorLocation().X - Enemy->GetActorLocation().X)
					: -1.0f;
				FinishError(*FString::Printf(
					TEXT("MeleeMinion never produced its fixed 15-point CombatEvent (Brain=%d Phase=%d Target=%s XDistance=%.1f)."),
					static_cast<int32>(Runtime.BrainState), static_cast<int32>(Runtime.AttackPhase),
					Target ? *Target->GetName() : TEXT("None"), XDistance));
				return false;
			}
			if (MeleeEventCount != 1 || !FMath::IsNearlyEqual(LastMeleeDamage, 15.0f))
			{
				FinishError(TEXT("MeleeMinion attack did not produce exactly one 15-point first-hit CombatEvent."));
				return false;
			}
			FPRDamageRequest DeathRequest;
			DeathRequest.SourceId = TEXT("EnemyCheckpointASmoke");
			DeathRequest.DamageSource = PlayerPawn;
			DeathRequest.Instigator = PlayerPawn;
			DeathRequest.Target = Enemy;
			DeathRequest.RawDamage = 1000.0f;
			DeathRequest.ImpactOrigin = PlayerPawn->GetActorLocation();
			DeathRequest.IncomingDirection = (Enemy->GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal();
			if (Combat->ApplyDamage(DeathRequest) != EPRCombatRequestStatus::Applied)
			{
				FinishError(TEXT("Fixed Enemy PIE smoke could not kill the MeleeMinion through CombatSubsystem."));
				return false;
			}
			bDeathRequested = true;
			return true;
		}
		const APREnemyAIController* EnemyController = Cast<APREnemyAIController>(Enemy->GetController());
		if (!Enemy->IsEnemyDead() || !Enemy->GetEnemyBrain()
			|| Enemy->GetEnemyBrain()->GetRuntimeState().BrainState != EPREnemyBrainState::Dead
			|| (EnemyController && EnemyController->IsEnemyStateTreeRunning()))
		{
			if (PlayWorld->GetTimeSeconds() - SpawnedAt < 4.0)
			{
				return true;
			}
			FinishError(TEXT("MeleeMinion death did not stop the Brain and StateTree."));
			return false;
		}
		FinishSuccess();
		return false;
	}

	void CleanupRuntime()
	{
		if (UWorld* PlayWorld = World.Get())
		{
			if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>())
			{
				if (CombatEventHandle.IsValid()) Combat->OnCombatEvent().Remove(CombatEventHandle);
			}
			if (SpawnId.IsValid())
			{
				if (UPREnemySubsystem* Subsystem = PlayWorld->GetSubsystem<UPREnemySubsystem>()) Subsystem->DespawnEnemy(SpawnId);
			}
		}
		CombatEventHandle.Reset();
		SpawnId.Invalidate();
		SpawnedEnemy.Reset();
	}

	void FinishSuccess()
	{
		CleanupRuntime();
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"prototype\":\"MeleeMinion\",\"health\":100,\"attackPower\":10,\"moveSpeed\":450,\"stateTreeRunning\":true,\"meleeCombatEvent\":15,\"deathStopsBrain\":true,\"runtimeClean\":true}"));
	}

	void FinishError(const TCHAR* Message)
	{
		CleanupRuntime();
		Result->SetError(Message);
	}

	TWeakObjectPtr<UWorld> World;
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	double StartedAt = 0.0;
	double SpawnedAt = 0.0;
	FGuid SpawnId;
	TWeakObjectPtr<APREnemyCharacter> SpawnedEnemy;
	FDelegateHandle CombatEventHandle;
	int32 MeleeEventCount = 0;
	float LastMeleeDamage = 0.0f;
	bool bDeathRequested = false;
};
}

UToolCallAsyncResultString* UPREnemyAutomationToolset::RunPIEEnemyCheckpointASmoke()
{
	return PREnemyAutomationToolset::FCheckpointARunner::Start();
}
