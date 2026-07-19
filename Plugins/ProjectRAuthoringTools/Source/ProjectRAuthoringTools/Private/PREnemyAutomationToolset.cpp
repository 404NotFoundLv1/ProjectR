// Copyright Epic Games, Inc. All Rights Reserved.

#include "PREnemyAutomationToolset.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRSkillDecoyActor.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyAIController.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyProjectile.h"
#include "Enemies/PREnemySubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRTagLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

/**
 * Deliberately narrow B smoke: it neither accepts content paths nor writes assets/maps.
 * The fixed registry and fixed formal player pawn are the only inputs.  Detailed edge,
 * proxy, invalidation, and coarse-frame cases remain native automation coverage.
 */
class FCheckpointBRunner : public TSharedFromThis<FCheckpointBRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FCheckpointBRunner> Runner = MakeShared<FCheckpointBRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Runner->FinishError(TEXT("RunPIEEnemyCheckpointBSmoke requires an active authoritative in-process PIE world."));
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
			FinishError(TEXT("Fixed B smoke lost its PIE world, EnemySubsystem, or formal player pawn."));
			return false;
		}

		if (!SpawnedEnemy.IsValid())
		{
			APREnemyCharacter* Enemy = nullptr;
			const FTransform SpawnTransform(PlayerPawn->GetActorRotation(), PlayerPawn->GetActorLocation() + FVector(500.0f, 0.0f, 0.0f));
			const EPREnemySpawnStatus Status = Subsystem->SpawnEnemyPrototype(
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion")), SpawnTransform, SpawnId, Enemy);
			if (Status == EPREnemySpawnStatus::NotReady && PlayWorld->GetTimeSeconds() - StartedAt < 5.0)
			{
				return true;
			}
			if (Status != EPREnemySpawnStatus::Spawned || !Enemy || !Enemy->IsEnemyInitialized())
			{
				FinishError(TEXT("Fixed B smoke could not spawn initialized RangedMinion from the fixed whitelist."));
				return false;
			}
			const UPRAttributeSet* Attributes = Enemy->GetAttributeSet();
			if (!Attributes || !FMath::IsNearlyEqual(Attributes->GetHealth(), 80.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetAttackPower(), 10.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetMoveSpeed(), 380.0f)
				|| Enemy->GetAbilityTargetMobility() != EPRAbilityTargetMobility::Light)
			{
				FinishError(*FString::Printf(TEXT("RangedMinion P0 attributes or Light mobility were invalid (Health=%.2f AP=%.2f Move=%.2f Mobility=%d)."),
					Attributes ? Attributes->GetHealth() : -1.0f, Attributes ? Attributes->GetAttackPower() : -1.0f,
					Attributes ? Attributes->GetMoveSpeed() : -1.0f, static_cast<int32>(Enemy->GetAbilityTargetMobility())));
				return false;
			}
			UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
			if (!Combat)
			{
				FinishError(TEXT("Fixed B smoke could not resolve CombatSubsystem."));
				return false;
			}
			SpawnedEnemy = Enemy;
			SpawnedAt = PlayWorld->GetTimeSeconds();
			CombatEventHandle = Combat->OnCombatEvent().AddLambda([this](const FPRCombatEvent& Event)
			{
				if (Event.Instigator.Get() == SpawnedEnemy.Get() && Event.SourceId == TEXT("RangedShot"))
				{
					++RangedEventCount;
					LastRangedDamage = Event.RawDamage;
				}
				if (Event.AbilityTag == FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"))
					&& Event.ResponseTags.HasTag(UPRTagLibrary::GetCombatResponseDecoyConsumedTag()))
				{
					bProxyConsumed = true;
				}
			});
			return true;
		}

		APREnemyCharacter* Enemy = SpawnedEnemy.Get();
		if (!Enemy || !Enemy->GetEnemyBrain())
		{
			FinishError(TEXT("Fixed B smoke lost its spawned RangedMinion or Brain."));
			return false;
		}
		int32 ActiveProjectileCount = 0;
		APREnemyProjectile* ActiveProjectile = nullptr;
		for (TActorIterator<APREnemyProjectile> ProjectileIt(PlayWorld); ProjectileIt; ++ProjectileIt)
		{
			++ActiveProjectileCount;
			ActiveProjectile = *ProjectileIt;
		}
		MaxObservedProjectileCount = FMath::Max(MaxObservedProjectileCount, ActiveProjectileCount);
		if (bProxyShotRequested && !bProxyDecoySpawned && ActiveProjectile)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = PlayerPawn;
			FVector ProxyLocation = ActiveProjectile->GetActorLocation() + ActiveProjectile->GetActorForwardVector().GetSafeNormal() * 60.0f;
			ProxyLocation.Y = ActiveProjectile->GetActorLocation().Y;
			APRSkillDecoyActor* Decoy = PlayWorld->SpawnActor<APRSkillDecoyActor>(
				APRSkillDecoyActor::StaticClass(), ProxyLocation, FRotator::ZeroRotator, SpawnParameters);
			if (!Decoy)
			{
				FinishError(TEXT("Fixed B smoke could not create its fixed transient Afterimage attack proxy."));
				return false;
			}
			Decoy->InitializeProxy(PlayerPawn, TEXT("EnemyCheckpointBDecoy"), 0.14f, 1.5f);
			if (Decoy->GetAttackProxyOwner() != PlayerPawn || Decoy->GetAttackProxyId().IsNone())
			{
				Decoy->Destroy();
				FinishError(TEXT("Fixed B smoke transient Afterimage attack proxy did not initialize."));
				return false;
			}
			SpawnedDecoy = Decoy;
			bProxyDecoySpawned = true;
			ProxySpawnedAt = PlayWorld->GetTimeSeconds();
		}
		const FPREnemyRuntimeState& Runtime = Enemy->GetEnemyBrain()->GetRuntimeState();
		if (Runtime.BrainState == EPREnemyBrainState::HoldRange || Runtime.BrainState == EPREnemyBrainState::Attack)
		{
			bObservedHoldRange = true;
		}
		if (!bProxyShotRequested && RangedEventCount == 0)
		{
			if (PlayWorld->GetTimeSeconds() - SpawnedAt < 4.0)
			{
				return true;
			}
			FinishError(*FString::Printf(TEXT("RangedMinion did not produce a fixed 12-point projectile CombatEvent (Brain=%d Phase=%d Hold=%d PeakProjectiles=%d)."),
				static_cast<int32>(Runtime.BrainState), static_cast<int32>(Runtime.AttackPhase), bObservedHoldRange ? 1 : 0, MaxObservedProjectileCount));
			return false;
		}
		if (bProxyShotRequested && !bProxyDecoySpawned && PlayWorld->GetTimeSeconds() - SpawnedAt > 4.0)
		{
			FinishError(TEXT("Fixed B smoke did not observe the second RangedShot projectile needed for attack-proxy coverage."));
			return false;
		}
		if (!bProxyShotRequested && (!bObservedHoldRange || RangedEventCount != 1 || !FMath::IsNearlyEqual(LastRangedDamage, 12.0f)))
		{
			FinishError(TEXT("RangedMinion did not make exactly one fixed 12-point HoldRange projectile resolution."));
			return false;
		}
		if (!bProxyShotRequested)
		{
			if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>(); Combat && CombatEventHandle.IsValid())
			{
				Combat->OnCombatEvent().Remove(CombatEventHandle);
				CombatEventHandle.Reset();
			}
			Subsystem->DespawnEnemy(SpawnId);
			SpawnId.Invalidate();
			SpawnedEnemy.Reset();
			bProxyShotRequested = true;
			return true;
		}
		if (bProxyConsumed && !SpawnedDecoy.IsValid())
		{
			if (RangedEventCount != 1)
			{
				FinishError(TEXT("Afterimage attack-proxy consumption produced an unexpected second RangedShot CombatEvent."));
				return false;
			}
			FinishSuccess();
			return false;
		}
		if (bProxyDecoySpawned && PlayWorld->GetTimeSeconds() - ProxySpawnedAt > 1.6)
		{
			FinishError(*FString::Printf(
				TEXT("Ranged projectile did not consume the fixed Afterimage attack proxy within its formal lifetime (Events=%d Consumed=%d DecoyValid=%d Brain=%d Phase=%d)."),
				RangedEventCount, bProxyConsumed ? 1 : 0, SpawnedDecoy.IsValid() ? 1 : 0,
				static_cast<int32>(Runtime.BrainState), static_cast<int32>(Runtime.AttackPhase)));
			return false;
		}
		return true;
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
		if (APRSkillDecoyActor* Decoy = SpawnedDecoy.Get())
		{
			Decoy->Destroy();
		}
		SpawnedDecoy.Reset();
		SpawnId.Invalidate();
		SpawnedEnemy.Reset();
	}

	void FinishSuccess()
	{
		CleanupRuntime();
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"prototype\":\"RangedMinion\",\"holdRange\":true,\"projectileCombatEvent\":12,\"rawDamage\":12,\"afterimageProxyConsumed\":true,\"runtimeClean\":true}"));
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
	int32 RangedEventCount = 0;
	int32 MaxObservedProjectileCount = 0;
	float LastRangedDamage = 0.0f;
	bool bObservedHoldRange = false;
	bool bProxyShotRequested = false;
	bool bProxyDecoySpawned = false;
	bool bProxyConsumed = false;
	double ProxySpawnedAt = 0.0;
	TWeakObjectPtr<APRSkillDecoyActor> SpawnedDecoy;
};
}

UToolCallAsyncResultString* UPREnemyAutomationToolset::RunPIEEnemyCheckpointASmoke()
{
	return PREnemyAutomationToolset::FCheckpointARunner::Start();
}

UToolCallAsyncResultString* UPREnemyAutomationToolset::RunPIEEnemyCheckpointBSmoke()
{
	return PREnemyAutomationToolset::FCheckpointBRunner::Start();
}
