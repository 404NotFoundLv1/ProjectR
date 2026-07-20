// Copyright Epic Games, Inc. All Rights Reserved.

#include "PREnemyAutomationToolset.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRSkillDecoyActor.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "PRPlayerSkillAutomationToolset.h"
#include "Characters/PRPlayerCharacter.h"
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
#if WITH_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#endif

#if WITH_AUTOMATION_TESTS

namespace PREnemyCheckpointDPlayerHookTest
{
struct FRunState
{
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	double StartedAtSeconds = 0.0;
	bool bStarted = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyCheckpointDPlayerHookAutomationTest,
	"ProjectRAuthoringTools.Enemy.PIE.CheckpointDPlayerHookRegression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPREnemyCheckpointDPlayerHookAutomationTest::RunTest(const FString& Parameters)
{
	using namespace PREnemyCheckpointDPlayerHookTest;

	const TSharedRef<FRunState> State = MakeShared<FRunState>();
	State->StartedAtSeconds = FPlatformTime::Seconds();
	AddCommand(new FEditorLoadMap(TEXT("/Game/ProjectR/Maps/L_CombatGym")));
	AddCommand(new FStartPIECommand(false));
	AddCommand(new FFunctionLatentCommand([this, State]()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld))
		{
			if (FPlatformTime::Seconds() - State->StartedAtSeconds > 30.0)
			{
				AddError(TEXT("Frozen PlayerSkill checkpoint-D regression did not start PIE within thirty seconds."));
				return true;
			}
			return false;
		}

		if (!State->bStarted)
		{
			State->Result = TStrongObjectPtr<UToolCallAsyncResultString>(UPRPlayerSkillAutomationToolset::RunPIEPlayerSkillCheckpointDSmoke());
			State->bStarted = true;
			TestNotNull(TEXT("Frozen PlayerSkill checkpoint-D runner returns an async result."), State->Result.Get());
			return false;
		}

		if (!State->Result.IsValid())
		{
			AddError(TEXT("Frozen PlayerSkill checkpoint-D result became invalid before completion."));
			return true;
		}

		if (!State->Result->bIsComplete)
		{
			if (FPlatformTime::Seconds() - State->StartedAtSeconds > 30.0)
			{
				AddError(TEXT("Frozen PlayerSkill checkpoint-D runner did not complete while Editor automation was ticking PIE."));
				return true;
			}
			return false;
		}

		TestTrue(TEXT("Frozen PlayerSkill checkpoint-D runner completes without an error."), State->Result->Error.IsEmpty());
		TestTrue(TEXT("Frozen PlayerSkill checkpoint-D runner reports PASS."), State->Result->Value.Contains(TEXT("\"status\":\"PASS\"")));
		TestTrue(TEXT("VectorHook Anchored pulls only the player to 120cm."), State->Result->Value.Contains(TEXT("\"hookAnchored120cm\":true")));
		TestTrue(TEXT("VectorHook Heavy remains covered by the frozen D sequence."), State->Result->Value.Contains(TEXT("\"hookHeavy120cm\":true")));
		TestTrue(TEXT("Frozen PlayerSkill checkpoint-D runner leaves runtime clean."), State->Result->Value.Contains(TEXT("\"runtimeClean\":true")));
		return true;
	}));
	AddCommand(new FEndPlayMapCommand());
	AddCommand(new FFunctionLatentCommand([this]()
	{
		const bool bStopped = !GEditor || !GEditor->PlayWorld;
		if (!bStopped)
		{
			return false;
		}
		TestTrue(TEXT("Frozen PlayerSkill checkpoint-D regression ended its PIE session."), bStopped);
		return true;
	}));

	return true;
}

#endif
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

/**
 * Fixed C smoke. It validates only the fixed ShieldMinion registry entry and
 * its guarding/shield contracts; the frozen PlayerSkill checkpoint-D smoke
 * remains the authoritative formal VectorHook execution test. This runner
 * neither accepts caller data nor saves a map or content package.
 */
class FCheckpointCRunner : public TSharedFromThis<FCheckpointCRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FCheckpointCRunner> Runner = MakeShared<FCheckpointCRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Runner->FinishError(TEXT("RunPIEEnemyCheckpointCSmoke requires an active authoritative in-process PIE world."));
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
		APRPlayerCharacter* Player = Controller ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		if (!PlayWorld || !Subsystem || !Player)
		{
			FinishError(TEXT("Fixed C smoke lost its PIE world, EnemySubsystem, or formal player character."));
			return false;
		}
		if (!SpawnedEnemy.IsValid())
		{
			APREnemyCharacter* Enemy = nullptr;
			const FTransform SpawnTransform(Player->GetActorRotation(), Player->GetActorLocation() + FVector(130.0f, 0.0f, 0.0f));
			const EPREnemySpawnStatus Status = Subsystem->SpawnEnemyPrototype(
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.ShieldMinion")), SpawnTransform, SpawnId, Enemy);
			if (Status == EPREnemySpawnStatus::NotReady && PlayWorld->GetTimeSeconds() - StartedAt < 5.0)
			{
				return true;
			}
			if (Status != EPREnemySpawnStatus::Spawned || !Enemy || !Enemy->IsEnemyInitialized())
			{
				FinishError(TEXT("Fixed C smoke could not spawn initialized ShieldMinion from the fixed whitelist."));
				return false;
			}
			UPRAbilitySystemComponent* EnemyASC = Enemy->GetProjectRAbilitySystemComponent();
			const UPRAttributeSet* Attributes = Enemy->GetAttributeSet();
			const FGameplayTag Guarding = UPRTagLibrary::GetStateGuardingTag();
			if (!EnemyASC || !Attributes || EnemyASC->GetOwnerActor() != Enemy || EnemyASC->GetAvatarActor() != Enemy
				|| !FMath::IsNearlyEqual(Attributes->GetHealth(), 140.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetShield(), 80.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetAttackPower(), 12.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetMoveSpeed(), 300.0f)
				|| Enemy->GetAbilityTargetMobility() != EPRAbilityTargetMobility::Heavy
				|| EnemyASC->GetTagCount(Guarding) != 1)
			{
				FinishError(TEXT("ShieldMinion GAS, P0 attributes, Heavy mobility, or initial unique Guarding contract was invalid."));
				return false;
			}
			UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
			if (!Combat)
			{
				FinishError(TEXT("Fixed C smoke could not resolve CombatSubsystem."));
				return false;
			}
			SpawnedEnemy = Enemy;
			SpawnedAt = PlayWorld->GetTimeSeconds();
			CombatEventHandle = Combat->OnCombatEvent().AddLambda([this](const FPRCombatEvent& Event)
			{
				if (Event.Target.Get() == SpawnedEnemy.Get() && Event.SourceId == TEXT("EnemyCheckpointCSmoke"))
				{
					ShieldEvents.Add(Event);
				}
				if (Event.Instigator.Get() == SpawnedEnemy.Get() && Event.SourceId == TEXT("ShieldBash"))
				{
					++ShieldBashEventCount;
					LastShieldBashDamage = Event.RawDamage;
				}
			});
			return true;
		}

		APREnemyCharacter* Enemy = SpawnedEnemy.Get();
		UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		if (!Enemy || !Combat)
		{
			FinishError(TEXT("Fixed C smoke lost its ShieldMinion or CombatSubsystem."));
			return false;
		}

		if (!bShieldBashObserved)
		{
			if (ShieldBashEventCount == 0)
			{
				if (PlayWorld->GetTimeSeconds() - SpawnedAt < 4.0)
				{
					return true;
				}
				FinishError(TEXT("ShieldMinion did not produce its fixed ShieldBash CombatEvent within four seconds."));
				return false;
			}
			if (ShieldBashEventCount != 1 || !FMath::IsNearlyEqual(LastShieldBashDamage, 16.0f))
			{
				FinishError(TEXT("ShieldMinion did not produce exactly one 16-point ShieldBash CombatEvent."));
				return false;
			}
			bShieldBashObserved = true;
			if (UPREnemyBrainComponent* Brain = Enemy->GetEnemyBrain()) Brain->StopBrain();
			if (APREnemyAIController* EnemyController = Cast<APREnemyAIController>(Enemy->GetController())) EnemyController->StopEnemyStateTree();
			return true;
		}

		UPRAbilitySystemComponent* EnemyASC = Enemy->GetProjectRAbilitySystemComponent();
		const UPRAttributeSet* Attributes = Enemy->GetAttributeSet();
		if (!EnemyASC || !Attributes)
		{
			FinishError(TEXT("Fixed C smoke lost ShieldMinion ASC or AttributeSet."));
			return false;
		}
		if (!bDamageVerified)
		{
			auto ApplyFixedDamage = [Combat, Player, Enemy](const float RawDamage)
			{
				FPRDamageRequest Request;
				Request.SourceId = TEXT("EnemyCheckpointCSmoke");
				Request.DamageSource = Player;
				Request.Instigator = Player;
				Request.Target = Enemy;
				Request.RawDamage = RawDamage;
				Request.ImpactOrigin = Player->GetActorLocation();
				Request.IncomingDirection = (Enemy->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
				return Combat->ApplyDamage(Request);
			};
			if (ApplyFixedDamage(30.0f) != EPRCombatRequestStatus::Applied
				|| !FMath::IsNearlyEqual(Attributes->GetShield(), 50.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetHealth(), 140.0f)
				|| EnemyASC->GetTagCount(UPRTagLibrary::GetStateGuardingTag()) != 1
				|| ApplyFixedDamage(70.0f) != EPRCombatRequestStatus::Applied
				|| !FMath::IsNearlyEqual(Attributes->GetShield(), 0.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetHealth(), 120.0f)
				|| EnemyASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
				|| EnemyASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag())
				|| ApplyFixedDamage(10.0f) != EPRCombatRequestStatus::Applied)
			{
				FinishError(TEXT("ShieldMinion spill-over, Guarding removal, or no-Stunned contract was invalid."));
				return false;
			}
			if (ShieldEvents.Num() != 3
				|| !ShieldEvents[0].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldAbsorbedTag())
				|| ShieldEvents[0].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag())
				|| !ShieldEvents[1].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldAbsorbedTag())
				|| !ShieldEvents[1].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag())
				|| !ShieldEvents[1].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseHealthDamagedTag())
				|| ShieldEvents[2].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag()))
			{
				FinishError(TEXT("ShieldMinion spill-over CombatEvent response ordering was invalid."));
				return false;
			}
			EnemyASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 25.0f);
			if (EnemyASC->GetTagCount(UPRTagLibrary::GetStateGuardingTag()) != 1)
			{
				FinishError(TEXT("Formal Shield restoration did not re-add exactly one Guarding tag."));
				return false;
			}
			bDamageVerified = true;
			return true;
		}

		FinishSuccess();
		return false;
	}

	APRPlayerCharacter* GetPlayer() const
	{
		UWorld* PlayWorld = World.Get();
		APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
		return Controller ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
	}

	void CleanupRuntime()
	{
		if (UWorld* PlayWorld = World.Get())
		{
			if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>(); Combat && CombatEventHandle.IsValid())
			{
				Combat->OnCombatEvent().Remove(CombatEventHandle);
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
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"prototype\":\"ShieldMinion\",\"health\":140,\"shield\":80,\"guardingUnique\":true,\"shieldBrokenSingle\":true,\"shieldBashCombatEvent\":16,\"runtimeClean\":true}"));
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
	TArray<FPRCombatEvent> ShieldEvents;
	int32 ShieldBashEventCount = 0;
	float LastShieldBashDamage = 0.0f;
	bool bShieldBashObserved = false;
	bool bDamageVerified = false;
};

/**
 * Fixed D smoke. It spawns only the whitelisted EliteAuditGuard and proves its
 * first ShieldBreak response and Staggered lifecycle. The frozen PlayerSkill
 * checkpoint-D smoke remains the authority for player-side VectorHook input;
 * this runner proves the real Elite exposes Anchored mobility without a type
 * dependency. It accepts no caller data and never saves a package or map.
 */
class FCheckpointDRunner : public TSharedFromThis<FCheckpointDRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FCheckpointDRunner> Runner = MakeShared<FCheckpointDRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->Result->AddToRoot();
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Runner->FinishError(TEXT("RunPIEEnemyCheckpointDSmoke requires an active authoritative in-process PIE world."));
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
		APRPlayerCharacter* Player = Controller ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		if (!PlayWorld || !Subsystem || !Player)
		{
			FinishError(TEXT("Fixed D smoke lost its PIE world, EnemySubsystem, or formal player character."));
			return false;
		}

		if (!SpawnedEnemy.IsValid())
		{
			APREnemyCharacter* Enemy = nullptr;
			const FTransform SpawnTransform(Player->GetActorRotation(), Player->GetActorLocation() + FVector(150.0f, 0.0f, 0.0f));
			const EPREnemySpawnStatus Status = Subsystem->SpawnEnemyPrototype(
				FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard")), SpawnTransform, SpawnId, Enemy);
			if (Status == EPREnemySpawnStatus::NotReady && PlayWorld->GetTimeSeconds() - StartedAt < 5.0)
			{
				return true;
			}
			if (Status != EPREnemySpawnStatus::Spawned || !Enemy || !Enemy->IsEnemyInitialized())
			{
				FinishError(TEXT("Fixed D smoke could not spawn initialized EliteAuditGuard from the fixed whitelist."));
				return false;
			}
			UPRAbilitySystemComponent* EnemyASC = Enemy->GetProjectRAbilitySystemComponent();
			const UPRAttributeSet* Attributes = Enemy->GetAttributeSet();
			if (!EnemyASC || !Attributes || EnemyASC->GetOwnerActor() != Enemy || EnemyASC->GetAvatarActor() != Enemy
				|| !FMath::IsNearlyEqual(Attributes->GetHealth(), 300.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetShield(), 150.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetAttackPower(), 15.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetMoveSpeed(), 350.0f)
				|| Enemy->GetAbilityTargetMobility() != EPRAbilityTargetMobility::Anchored
				|| EnemyASC->GetTagCount(UPRTagLibrary::GetStateGuardingTag()) != 1)
			{
				FinishError(TEXT("EliteAuditGuard GAS, P0 attributes, Anchored mobility, or initial unique Guarding contract was invalid."));
				return false;
			}
			UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
			if (!Combat)
			{
				FinishError(TEXT("Fixed D smoke could not resolve CombatSubsystem."));
				return false;
			}
			SpawnedEnemy = Enemy;
			SpawnedAt = PlayWorld->GetTimeSeconds();
			CombatEventHandle = Combat->OnCombatEvent().AddLambda([this](const FPRCombatEvent& Event)
			{
				if (Event.Instigator.Get() == SpawnedEnemy.Get() && Event.SourceId == TEXT("EliteStrike"))
				{
					++EliteStrikeEventCount;
					LastEliteStrikeDamage = Event.RawDamage;
				}
				if (Event.Target.Get() == SpawnedEnemy.Get() && Event.SourceId == TEXT("EnemyCheckpointDSmoke"))
				{
					ShieldEvents.Add(Event);
				}
			});
			return true;
		}

		APREnemyCharacter* Enemy = SpawnedEnemy.Get();
		UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		UPRAbilitySystemComponent* EnemyASC = Enemy ? Enemy->GetProjectRAbilitySystemComponent() : nullptr;
		const UPRAttributeSet* Attributes = Enemy ? Enemy->GetAttributeSet() : nullptr;
		UPREnemyBrainComponent* Brain = Enemy ? Enemy->GetEnemyBrain() : nullptr;
		if (!Enemy || !Combat || !EnemyASC || !Attributes || !Brain)
		{
			FinishError(TEXT("Fixed D smoke lost EliteAuditGuard runtime state."));
			return false;
		}

		if (!bEliteStrikeObserved)
		{
			if (EliteStrikeEventCount == 0 && PlayWorld->GetTimeSeconds() - SpawnedAt < 5.0)
			{
				return true;
			}
			if (EliteStrikeEventCount != 1 || !FMath::IsNearlyEqual(LastEliteStrikeDamage, 24.0f))
			{
				FinishError(TEXT("EliteAuditGuard did not produce exactly one 24-point EliteStrike CombatEvent."));
				return false;
			}
			bEliteStrikeObserved = true;
			return true;
		}

		auto ApplyFixedDamage = [Combat, Player, Enemy](const float RawDamage)
		{
			FPRDamageRequest Request;
			Request.SourceId = TEXT("EnemyCheckpointDSmoke");
			Request.DamageSource = Player;
			Request.Instigator = Player;
			Request.Target = Enemy;
			Request.RawDamage = RawDamage;
			Request.ImpactOrigin = Player->GetActorLocation();
			Request.IncomingDirection = (Enemy->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
			return Combat->ApplyDamage(Request);
		};

		if (!bFirstBreakApplied)
		{
			if (ApplyFixedDamage(150.0f) != EPRCombatRequestStatus::Applied
				|| !FMath::IsNearlyEqual(Attributes->GetShield(), 0.0f)
				|| !FMath::IsNearlyEqual(Attributes->GetHealth(), 300.0f)
				|| EnemyASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag())
				|| EnemyASC->GetTagCount(UPRTagLibrary::GetStateStunnedTag()) != 1
				|| Brain->GetRuntimeState().BrainState != EPREnemyBrainState::Staggered
				|| Brain->GetRuntimeState().AttackPhase != EPREnemyAttackPhase::None)
			{
				FinishError(TEXT("Elite first ShieldBreak did not produce the unique Stunned/Staggered attack-cancel contract."));
				return false;
			}
			if (ShieldEvents.Num() != 1
				|| !ShieldEvents[0].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldAbsorbedTag())
				|| !ShieldEvents[0].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag())
				|| ShieldEvents[0].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseHealthDamagedTag()))
			{
				FinishError(TEXT("Elite first ShieldBreak CombatEvent response payload was invalid."));
				return false;
			}
			FirstBreakAt = PlayWorld->GetTimeSeconds();
			bFirstBreakApplied = true;
			return true;
		}

		if (!bStunnedRecovered)
		{
			const double ElapsedStunSeconds = PlayWorld->GetTimeSeconds() - FirstBreakAt;
			if (ElapsedStunSeconds < 0.70)
			{
				if (Brain->GetRuntimeState().BrainState != EPREnemyBrainState::Staggered || EnemyASC->GetTagCount(UPRTagLibrary::GetStateStunnedTag()) != 1)
				{
					FinishError(TEXT("Elite Stunned did not remain in Staggered state for its frozen duration."));
					return false;
				}
				return true;
			}
			if (EnemyASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag())
				|| Brain->GetRuntimeState().BrainState == EPREnemyBrainState::Staggered)
			{
				if (ElapsedStunSeconds < 1.25)
				{
					return true;
				}
				FinishError(TEXT("Elite Stunned did not recover through controlled reevaluation after the frozen duration."));
				return false;
			}
			bStunnedRecovered = true;
			return true;
		}

		if (!bSecondBreakVerified)
		{
			EnemyASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 150.0f);
			if (EnemyASC->GetTagCount(UPRTagLibrary::GetStateGuardingTag()) != 1
				|| ApplyFixedDamage(150.0f) != EPRCombatRequestStatus::Applied
				|| EnemyASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag())
				|| EnemyASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()))
			{
				FinishError(TEXT("Elite Shield restoration or the one-shot second ShieldBreak response contract was invalid."));
				return false;
			}
			bSecondBreakVerified = true;
			return true;
		}

		FinishSuccess();
		return false;
	}

	void CleanupRuntime()
	{
		if (UWorld* PlayWorld = World.Get())
		{
			if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>(); Combat && CombatEventHandle.IsValid())
			{
				Combat->OnCombatEvent().Remove(CombatEventHandle);
			}
			if (SpawnId.IsValid())
			{
				if (UPREnemySubsystem* Subsystem = PlayWorld->GetSubsystem<UPREnemySubsystem>())
				{
					Subsystem->DespawnEnemy(SpawnId);
				}
			}
		}
		CombatEventHandle.Reset();
		SpawnId.Invalidate();
		SpawnedEnemy.Reset();
	}

	void FinishSuccess()
	{
		CleanupRuntime();
		if (Result.IsValid())
		{
			UToolCallAsyncResultString* CompletedResult = Result.Get();
			CompletedResult->SetValue(TEXT("{\"status\":\"PASS\",\"prototype\":\"EliteAuditGuard\",\"health\":300,\"shield\":150,\"anchoredMobility\":true,\"guardingUnique\":true,\"firstShieldBreakStunnedOnce\":true,\"staggeredRecovery\":true,\"eliteStrikeCombatEvent\":24,\"runtimeClean\":true}"));
			CompletedResult->RemoveFromRoot();
		}
	}

	void FinishError(const TCHAR* Message)
	{
		CleanupRuntime();
		if (Result.IsValid())
		{
			UToolCallAsyncResultString* CompletedResult = Result.Get();
			CompletedResult->SetError(Message);
			CompletedResult->RemoveFromRoot();
		}
	}

	TWeakObjectPtr<UWorld> World;
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	double StartedAt = 0.0;
	double SpawnedAt = 0.0;
	double FirstBreakAt = 0.0;
	FGuid SpawnId;
	TWeakObjectPtr<APREnemyCharacter> SpawnedEnemy;
	FDelegateHandle CombatEventHandle;
	TArray<FPRCombatEvent> ShieldEvents;
	int32 EliteStrikeEventCount = 0;
	float LastEliteStrikeDamage = 0.0f;
	bool bEliteStrikeObserved = false;
	bool bFirstBreakApplied = false;
	bool bStunnedRecovered = false;
	bool bSecondBreakVerified = false;
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

UToolCallAsyncResultString* UPREnemyAutomationToolset::RunPIEEnemyCheckpointCSmoke()
{
	return PREnemyAutomationToolset::FCheckpointCRunner::Start();
}

UToolCallAsyncResultString* UPREnemyAutomationToolset::RunPIEEnemyCheckpointDSmoke()
{
	return PREnemyAutomationToolset::FCheckpointDRunner::Start();
}

#if WITH_AUTOMATION_TESTS

namespace PREnemyCheckpointDPIETest
{
struct FRunState
{
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	double StartedAtSeconds = 0.0;
	bool bStarted = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyCheckpointDPIEAutomationTest,
	"ProjectRAuthoringTools.Enemy.PIE.CheckpointD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPREnemyCheckpointDPIEAutomationTest::RunTest(const FString& Parameters)
{
	using namespace PREnemyCheckpointDPIETest;

	const TSharedRef<FRunState> State = MakeShared<FRunState>();
	State->StartedAtSeconds = FPlatformTime::Seconds();
	AddCommand(new FEditorLoadMap(TEXT("/Game/ProjectR/Maps/L_CombatGym")));
	AddCommand(new FStartPIECommand(false));
	AddCommand(new FFunctionLatentCommand([this, State]()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld))
		{
			if (FPlatformTime::Seconds() - State->StartedAtSeconds > 12.0)
			{
				AddError(TEXT("Fixed D PIE smoke did not start an authoritative in-process PIE world within twelve seconds."));
				return true;
			}
			return false;
		}

		if (!State->bStarted)
		{
			State->Result = TStrongObjectPtr<UToolCallAsyncResultString>(UPREnemyAutomationToolset::RunPIEEnemyCheckpointDSmoke());
			State->bStarted = true;
			TestNotNull(TEXT("Fixed D PIE runner returns an async result."), State->Result.Get());
			return false;
		}

		if (!State->Result.IsValid())
		{
			AddError(TEXT("Fixed D PIE runner result became invalid before completion."));
			return true;
		}

		if (!State->Result->bIsComplete)
		{
			if (FPlatformTime::Seconds() - State->StartedAtSeconds > 12.0)
			{
				AddError(TEXT("Fixed D PIE runner did not complete within twelve seconds while editor automation was ticking PIE."));
				return true;
			}
			return false;
		}

		TestTrue(TEXT("Fixed D PIE runner completes without an error."), State->Result->Error.IsEmpty());
		TestTrue(TEXT("Fixed D PIE runner reports PASS."), State->Result->Value.Contains(TEXT("\"status\":\"PASS\"")));
		TestTrue(TEXT("Fixed D PIE runner proves EliteAuditGuard."), State->Result->Value.Contains(TEXT("\"prototype\":\"EliteAuditGuard\"")));
		TestTrue(TEXT("Fixed D PIE runner proves runtime cleanup."), State->Result->Value.Contains(TEXT("\"runtimeClean\":true")));
		return true;
	}));
	AddCommand(new FEndPlayMapCommand());
	AddCommand(new FFunctionLatentCommand([this]()
	{
		const bool bStopped = !GEditor || !GEditor->PlayWorld;
		if (!bStopped)
		{
			return false;
		}
		TestTrue(TEXT("Fixed D PIE smoke ended its PIE session."), bStopped);
		return true;
	}));

	return true;
}

#endif
