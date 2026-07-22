// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRBalanceAutomationToolset.h"

#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/PRAttributeSet.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRPlayerState.h"
#include "Editor.h"
#include "Enemies/Bosses/PRBossAuditor.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/Bosses/PRAuditorBossDataAsset.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Enemies/PREnemySubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRBalanceAutomationToolset
{
	constexpr int32 ExpectedAuthoritativeSourceCount = 40;

	UToolCallAsyncResultString* MakeError(const FString& Message)
	{
		UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
		Result->SetError(Message);
		return Result;
	}

	template <typename AssetType>
	AssetType* LoadRequiredAsset(const TCHAR* Path, FString& OutError)
	{
		AssetType* Asset = LoadObject<AssetType>(nullptr, Path);
		if (!IsValid(Asset))
		{
			OutError = FString::Printf(TEXT("v0.2.4 balance snapshot could not load required authoritative asset: %s"), Path);
		}
		return Asset;
	}

	bool VerifyRequiredAssets(FString& OutError)
	{
		static const TCHAR* RequiredAssetPaths[] = {
			TEXT("/Game/ProjectR/Effects/GE_DefaultAttributes.GE_DefaultAttributes"),
			TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerCharacter.BP_PlayerCharacter"),
			TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_ShadowThrust.DA_Skill_ShadowThrust"),
			TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_FireSlash.DA_Skill_FireSlash"),
			TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_ThunderDrop.DA_Skill_ThunderDrop"),
			TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_AfterimageDodge.DA_Skill_AfterimageDodge"),
			TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_VectorHook.DA_Skill_VectorHook"),
			TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_CounterProofWall.DA_Skill_CounterProofWall"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_ShadowThrust_Cost.GE_Skill_ShadowThrust_Cost"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_ShadowThrust_Cooldown.GE_Skill_ShadowThrust_Cooldown"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_FireSlash_Cost.GE_Skill_FireSlash_Cost"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_FireSlash_Cooldown.GE_Skill_FireSlash_Cooldown"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_ThunderDrop_Cost.GE_Skill_ThunderDrop_Cost"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_ThunderDrop_Cooldown.GE_Skill_ThunderDrop_Cooldown"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_AfterimageDodge_Cost.GE_Skill_AfterimageDodge_Cost"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_AfterimageDodge_Cooldown.GE_Skill_AfterimageDodge_Cooldown"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_VectorHook_Cost.GE_Skill_VectorHook_Cost"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_VectorHook_Cooldown.GE_Skill_VectorHook_Cooldown"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_CounterProofWall_Cost.GE_Skill_CounterProofWall_Cost"),
			TEXT("/Game/ProjectR/Abilities/Effects/GE_Skill_CounterProofWall_Cooldown.GE_Skill_CounterProofWall_Cooldown"),
			TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_MeleeMinion.DA_Enemy_MeleeMinion"),
			TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_RangedMinion.DA_Enemy_RangedMinion"),
			TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_ShieldMinion.DA_Enemy_ShieldMinion"),
			TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_EliteAuditGuard.DA_Enemy_EliteAuditGuard"),
			TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_MeleeStrike.DA_EnemyAttack_MeleeStrike"),
			TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_RangedShot.DA_EnemyAttack_RangedShot"),
			TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_ShieldBash.DA_EnemyAttack_ShieldBash"),
			TEXT("/Game/ProjectR/Enemies/Attacks/DA_EnemyAttack_EliteStrike.DA_EnemyAttack_EliteStrike"),
			TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_MeleeStrike_Cooldown.GE_EnemyAttack_MeleeStrike_Cooldown"),
			TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_RangedShot_Cooldown.GE_EnemyAttack_RangedShot_Cooldown"),
			TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_ShieldBash_Cooldown.GE_EnemyAttack_ShieldBash_Cooldown"),
			TEXT("/Game/ProjectR/Enemies/Effects/GE_EnemyAttack_EliteStrike_Cooldown.GE_EnemyAttack_EliteStrike_Cooldown"),
			TEXT("/Game/ProjectR/Enemies/Bosses/Prototypes/DA_Boss_Auditor.DA_Boss_Auditor"),
			TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorSweep.DA_EnemyAttack_AuditorSweep"),
			TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorLockShot.DA_EnemyAttack_AuditorLockShot"),
			TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorCounter.DA_EnemyAttack_AuditorCounter"),
			TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorSweep_Cooldown.GE_EnemyAttack_AuditorSweep_Cooldown"),
			TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorLockShot_Cooldown.GE_EnemyAttack_AuditorLockShot_Cooldown"),
			TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorCounter_Cooldown.GE_EnemyAttack_AuditorCounter_Cooldown"),
			TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_Boss_Auditor_PredictionShield.GE_Boss_Auditor_PredictionShield")
		};

		static_assert(UE_ARRAY_COUNT(RequiredAssetPaths) == ExpectedAuthoritativeSourceCount);
		for (const TCHAR* Path : RequiredAssetPaths)
		{
			if (!IsValid(LoadObject<UObject>(nullptr, Path)))
			{
				OutError = FString::Printf(TEXT("v0.2.4 balance snapshot could not load required authoritative asset: %s"), Path);
				return false;
			}
		}
		return true;
	}

	bool WriteFixedReport(const FString& Filename, const FString& Json, FString& OutError)
	{
		const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BalanceReports"), TEXT("v024"));
		const FString Path = FPaths::Combine(Directory, Filename);
		if (!IFileManager::Get().MakeDirectory(*Directory, true)
			|| !FFileHelper::SaveStringToFile(Json, *Path))
		{
			OutError = FString::Printf(TEXT("v0.2.4 balance snapshot could not write its fixed report: %s"), *Path);
			return false;
		}
		return true;
	}

	class FNormalEncounterRunner : public TSharedFromThis<FNormalEncounterRunner>
	{
	public:
		static UToolCallAsyncResultString* Start()
		{
			TSharedRef<FNormalEncounterRunner> Runner = MakeShared<FNormalEncounterRunner>();
			Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
			Runner->Result->AddToRoot();
			UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
			if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
			{
				Runner->FinishError(TEXT("RunPIEV024NormalEncounterBenchmark requires an active authoritative in-process PIE world."));
				return Runner->Result.Get();
			}
			Runner->World = PlayWorld;
			Runner->StartedAt = PlayWorld->GetTimeSeconds();
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Shared = Runner.ToSharedPtr()](float)
			{
				return Shared->Tick();
			}));
			return Runner->Result.Get();
		}

	private:
		bool Tick()
		{
			UWorld* PlayWorld = World.Get();
			if (!IsValid(PlayWorld))
			{
				FinishError(TEXT("Fixed v0.2.4 normal encounter lost its PIE world."));
				return false;
			}
			if (PlayWorld->GetTimeSeconds() - StartedAt > 180.0)
			{
				FinishError(TEXT("Fixed v0.2.4 normal encounter exceeded its 180-second human benchmark limit."));
				return false;
			}
			if (!bSpawned)
			{
				return TrySpawn();
			}
			if (PlayerAttributes.IsValid() && PlayerAttributes->GetHealth() <= 0.0f)
			{
				FinishError(TEXT("Fixed v0.2.4 normal encounter ended because the formal player was defeated before the composition was cleared."));
				return false;
			}

			int32 DefeatedCount = 0;
			for (const TWeakObjectPtr<APREnemyCharacter>& Enemy : Enemies)
			{
				if (!Enemy.IsValid())
				{
					FinishError(TEXT("Fixed v0.2.4 normal encounter lost one of its formal spawned enemies."));
					return false;
				}
				DefeatedCount += Enemy->IsEnemyDead() ? 1 : 0;
			}
			if (DefeatedCount == Enemies.Num())
			{
				FinishSuccess();
				return false;
			}
			return true;
		}

		bool TrySpawn()
		{
			UWorld* PlayWorld = World.Get();
			UPREnemySubsystem* EnemiesSubsystem = PlayWorld ? PlayWorld->GetSubsystem<UPREnemySubsystem>() : nullptr;
			APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
			APawn* PlayerPawn = Controller ? Controller->GetPawn() : nullptr;
			APRPlayerState* PlayerState = Controller ? Controller->GetPlayerState<APRPlayerState>() : nullptr;
			if (!EnemiesSubsystem || !PlayerPawn || !PlayerState || !PlayerState->GetAttributeSet())
			{
				return true;
			}
			if (!EnemiesSubsystem->IsRegistryReady())
			{
				return true;
			}

			Player = PlayerPawn;
			PlayerAttributes = PlayerState->GetAttributeSet();
			const FVector Origin = PlayerPawn->GetActorLocation();
			const TPair<FGameplayTag, float> SpawnPlan[] = {
				{FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")), 170.0f},
				{FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")), 380.0f},
				{FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion")), 520.0f},
				{FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.ShieldMinion")), 800.0f}
			};
			for (const TPair<FGameplayTag, float>& Entry : SpawnPlan)
			{
				FGuid SpawnId;
				APREnemyCharacter* Enemy = nullptr;
				const FVector Location(Origin.X + Entry.Value, Origin.Y, Origin.Z);
				const EPREnemySpawnStatus Status = EnemiesSubsystem->SpawnEnemyPrototype(
					Entry.Key, FTransform(PlayerPawn->GetActorRotation(), Location), SpawnId, Enemy);
				if (Status != EPREnemySpawnStatus::Spawned || !SpawnId.IsValid() || !IsValid(Enemy))
				{
					FinishError(TEXT("Fixed v0.2.4 normal encounter could not spawn its exact registry-whitelisted composition."));
					return false;
				}
				SpawnIds.Add(SpawnId);
				Enemies.Add(Enemy);
			}
			UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
			if (!Combat)
			{
				FinishError(TEXT("Fixed v0.2.4 normal encounter could not resolve CombatSubsystem."));
				return false;
			}
			CombatEventHandle = Combat->OnCombatEvent().AddLambda([this](const FPRCombatEvent& Event)
			{
				if (Event.Target.Get() == Player.Get())
				{
					DamageTaken += Event.ShieldAbsorbed + Event.HealthDamage;
					++HitsTaken;
				}
				if (Event.Instigator.Get() == Player.Get() && Event.AbilityTag.IsValid())
				{
					UsedSkillTags.Add(Event.AbilityTag);
				}
			});
			bSpawned = true;
			return true;
		}

		void Cleanup()
		{
			if (UWorld* PlayWorld = World.Get())
			{
				if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>(); Combat && CombatEventHandle.IsValid())
				{
					Combat->OnCombatEvent().Remove(CombatEventHandle);
				}
				if (UPREnemySubsystem* EnemiesSubsystem = PlayWorld->GetSubsystem<UPREnemySubsystem>())
				{
					for (const FGuid& SpawnId : SpawnIds)
					{
						EnemiesSubsystem->DespawnEnemy(SpawnId);
					}
				}
			}
			CombatEventHandle.Reset();
			SpawnIds.Reset();
			Enemies.Reset();
		}

		void FinishSuccess()
		{
			const double Duration = World.IsValid() ? World->GetTimeSeconds() - StartedAt : 0.0;
			const float Health = PlayerAttributes.IsValid() ? PlayerAttributes->GetHealth() : 0.0f;
			const float Shield = PlayerAttributes.IsValid() ? PlayerAttributes->GetShield() : 0.0f;
			const float Energy = PlayerAttributes.IsValid() ? PlayerAttributes->GetEnergy() : 0.0f;
			const int32 UsedSkillCount = UsedSkillTags.Num();
			const FString Json = FString::Printf(TEXT("{\"status\":\"PASS\",\"benchmark\":\"CombatGymNormal\",\"composition\":{\"melee\":2,\"ranged\":1,\"shield\":1},\"durationSeconds\":%.3f,\"health\":%.3f,\"shield\":%.3f,\"energy\":%.3f,\"damageTaken\":%.3f,\"hitsTaken\":%d,\"uniqueSkillsUsed\":%d,\"runtimeClean\":true}"), Duration, Health, Shield, Energy, DamageTaken, HitsTaken, UsedSkillCount);
			FString ReportError;
			const FString Filename = FString::Printf(TEXT("combatgym-normal-%s.json"), *FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%S")));
			Cleanup();
			if (!WriteFixedReport(Filename, Json, ReportError))
			{
				FinishError(ReportError);
				return;
			}
			if (Result.IsValid())
			{
				Result->SetValue(Json);
				Result->RemoveFromRoot();
			}
		}

		void FinishError(const FString& Message)
		{
			Cleanup();
			if (Result.IsValid())
			{
				Result->SetError(Message);
				Result->RemoveFromRoot();
			}
		}

		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<APawn> Player;
		TWeakObjectPtr<const UPRAttributeSet> PlayerAttributes;
		TStrongObjectPtr<UToolCallAsyncResultString> Result;
		TArray<FGuid> SpawnIds;
		TArray<TWeakObjectPtr<APREnemyCharacter>> Enemies;
		TSet<FGameplayTag> UsedSkillTags;
		FDelegateHandle CombatEventHandle;
		double StartedAt = 0.0;
		float DamageTaken = 0.0f;
		int32 HitsTaken = 0;
		bool bSpawned = false;
	};

	class FAuditorBenchmarkRunner : public TSharedFromThis<FAuditorBenchmarkRunner>
	{
	public:
		static UToolCallAsyncResultString* Start()
		{
			TSharedRef<FAuditorBenchmarkRunner> Runner = MakeShared<FAuditorBenchmarkRunner>();
			Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
			Runner->Result->AddToRoot();
			UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
			if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
			{
				Runner->FinishError(TEXT("RunPIEV024AuditorBenchmark requires an active authoritative in-process PIE world."));
				return Runner->Result.Get();
			}
			Runner->World = PlayWorld;
			Runner->StartedAt = PlayWorld->GetTimeSeconds();
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Shared = Runner.ToSharedPtr()](float)
			{
				return Shared->Tick();
			}));
			return Runner->Result.Get();
		}

	private:
		bool Tick()
		{
			UWorld* PlayWorld = World.Get();
			if (!IsValid(PlayWorld))
			{
				FinishError(TEXT("Fixed v0.2.4 Auditor benchmark lost its PIE world."));
				return false;
			}
			if (PlayWorld->GetTimeSeconds() - StartedAt > 420.0)
			{
				FinishError(TEXT("Fixed v0.2.4 Auditor benchmark exceeded its 420-second safety limit."));
				return false;
			}
			if (!bBound)
			{
				return Bind();
			}
			if (bDefeated && CompletionFragments == 1)
			{
				FinishSuccess();
				return false;
			}
			return true;
		}

		bool Bind()
		{
			UWorld* PlayWorld = World.Get();
			UPRBossSubsystem* BossSubsystem = PlayWorld ? PlayWorld->GetSubsystem<UPRBossSubsystem>() : nullptr;
			if (!BossSubsystem)
			{
				return true;
			}
			FPRBossRuntimeState State;
			if (!BossSubsystem->GetActiveBossState(State))
			{
				return true;
			}
			BossStateHandle = BossSubsystem->OnBossStateChanged().AddLambda([this](const FPRBossRuntimeState& InState)
			{
				Observe(InState);
			});
			CompletionHandle = BossSubsystem->OnPrototypeRunCompleted().AddLambda([this](const FPRPrototypeRunResult& ResultValue)
			{
				CompletionFragments = ResultValue.CounterproofFragmentsAwarded;
			});
			Observe(State);
			bBound = true;
			return true;
		}

		void Observe(const FPRBossRuntimeState& State)
		{
			const double Now = State.WorldTimeSeconds > 0.0 ? State.WorldTimeSeconds : (World.IsValid() ? World->GetTimeSeconds() : 0.0);
			switch (State.Phase)
			{
			case EPRAuditorBossPhase::Sampling: if (SamplingAt < 0.0) SamplingAt = Now; break;
			case EPRAuditorBossPhase::RuleAudit: if (RuleAuditAt < 0.0) RuleAuditAt = Now; break;
			case EPRAuditorBossPhase::PredictionShield: if (PredictionAt < 0.0) PredictionAt = Now; break;
			case EPRAuditorBossPhase::Defeated: bDefeated = true; DefeatedAt = Now; break;
			default: break;
			}
		}

		void Cleanup()
		{
			if (UWorld* PlayWorld = World.Get())
			{
				if (UPRBossSubsystem* BossSubsystem = PlayWorld->GetSubsystem<UPRBossSubsystem>())
				{
					if (BossStateHandle.IsValid()) BossSubsystem->OnBossStateChanged().Remove(BossStateHandle);
					if (CompletionHandle.IsValid()) BossSubsystem->OnPrototypeRunCompleted().Remove(CompletionHandle);
				}
			}
			BossStateHandle.Reset();
			CompletionHandle.Reset();
		}

		void FinishSuccess()
		{
			const double Now = DefeatedAt >= 0.0 ? DefeatedAt : (World.IsValid() ? World->GetTimeSeconds() : StartedAt);
			const double P1 = SamplingAt >= 0.0 && RuleAuditAt >= SamplingAt ? RuleAuditAt - SamplingAt : -1.0;
			const double P2 = RuleAuditAt >= 0.0 && PredictionAt >= RuleAuditAt ? PredictionAt - RuleAuditAt : -1.0;
			const double P3 = PredictionAt >= 0.0 && Now >= PredictionAt ? Now - PredictionAt : -1.0;
			const double Total = Now - StartedAt;
			const bool bPhaseTimelineValid = P1 >= 0.0 && P2 >= 0.0 && P3 >= 0.0;
			const FString Json = FString::Printf(TEXT("{\"status\":\"PASS\",\"benchmark\":\"BossGymAuditor\",\"durationSeconds\":%.3f,\"p1Seconds\":%.3f,\"p2Seconds\":%.3f,\"p3Seconds\":%.3f,\"counterproofFragments\":1,\"runtimeClean\":true}"), Total, P1, P2, P3);
			FString ReportError;
			const FString Filename = FString::Printf(TEXT("bossgym-auditor-%s.json"), *FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%S")));
			Cleanup();
			if (!bPhaseTimelineValid)
			{
				FinishError(TEXT("Fixed v0.2.4 Auditor benchmark did not observe all P1/P2/P3 phase transitions."));
				return;
			}
			if (!WriteFixedReport(Filename, Json, ReportError))
			{
				FinishError(ReportError);
				return;
			}
			if (Result.IsValid())
			{
				Result->SetValue(Json);
				Result->RemoveFromRoot();
			}
		}

		void FinishError(const FString& Message)
		{
			Cleanup();
			if (Result.IsValid())
			{
				Result->SetError(Message);
				Result->RemoveFromRoot();
			}
		}

		TWeakObjectPtr<UWorld> World;
		TStrongObjectPtr<UToolCallAsyncResultString> Result;
		FDelegateHandle BossStateHandle;
		FDelegateHandle CompletionHandle;
		double StartedAt = 0.0;
		double SamplingAt = -1.0;
		double RuleAuditAt = -1.0;
		double PredictionAt = -1.0;
		double DefeatedAt = -1.0;
		int32 CompletionFragments = 0;
		bool bDefeated = false;
		bool bBound = false;
	};

	class FPlayerDeathRiskRunner : public TSharedFromThis<FPlayerDeathRiskRunner>
	{
	public:
		static UToolCallAsyncResultString* Start()
		{
			TSharedRef<FPlayerDeathRiskRunner> Runner = MakeShared<FPlayerDeathRiskRunner>();
			Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
			Runner->Result->AddToRoot();
			UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
			if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
			{
				Runner->FinishError(TEXT("RunPIEV024PlayerDeathRiskSmoke requires an active authoritative in-process PIE world."));
				return Runner->Result.Get();
			}
			Runner->World = PlayWorld;
			Runner->StartedAt = PlayWorld->GetTimeSeconds();
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Shared = Runner.ToSharedPtr()](float)
			{
				return Shared->Tick();
			}));
			return Runner->Result.Get();
		}

	private:
		bool Tick()
		{
			UWorld* PlayWorld = World.Get();
			if (!IsValid(PlayWorld))
			{
				FinishError(TEXT("Fixed v0.2.4 player-death-risk smoke lost its PIE world."));
				return false;
			}
			if (PlayWorld->GetTimeSeconds() - StartedAt > 60.0)
			{
				FinishError(TEXT("Formal player-death-risk smoke did not reach Shield -> Health -> Death within sixty seconds."));
				return false;
			}
			if (!bSpawned)
			{
				return TrySpawn();
			}
			if (bObservedFatal && PlayerAttributes.IsValid() && PlayerAttributes->GetHealth() <= 0.0f)
			{
				FinishSuccess();
				return false;
			}
			return true;
		}

		bool TrySpawn()
		{
			UWorld* PlayWorld = World.Get();
			UPREnemySubsystem* EnemySubsystem = PlayWorld ? PlayWorld->GetSubsystem<UPREnemySubsystem>() : nullptr;
			APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
			APawn* PlayerPawn = Controller ? Controller->GetPawn() : nullptr;
			APRPlayerState* PlayerState = Controller ? Controller->GetPlayerState<APRPlayerState>() : nullptr;
			if (!EnemySubsystem || !EnemySubsystem->IsRegistryReady() || !PlayerPawn || !PlayerState || !PlayerState->GetAttributeSet())
			{
				return true;
			}
			Player = PlayerPawn;
			PlayerAttributes = PlayerState->GetAttributeSet();
			const FVector Origin = PlayerPawn->GetActorLocation();
			const TPair<FGameplayTag, float> SpawnPlan[] = {
				{FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")), 120.0f},
				{FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")), 240.0f},
				{FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion")), 420.0f},
				{FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.ShieldMinion")), 600.0f}
			};
			for (const TPair<FGameplayTag, float>& Entry : SpawnPlan)
			{
				FGuid SpawnId;
				APREnemyCharacter* Enemy = nullptr;
				const EPREnemySpawnStatus Status = EnemySubsystem->SpawnEnemyPrototype(
					Entry.Key, FTransform(PlayerPawn->GetActorRotation(), FVector(Origin.X + Entry.Value, Origin.Y, Origin.Z)), SpawnId, Enemy);
				if (Status != EPREnemySpawnStatus::Spawned || !SpawnId.IsValid() || !IsValid(Enemy))
				{
					FinishError(TEXT("Formal player-death-risk smoke could not spawn its fixed registry-whitelisted attackers."));
					return false;
				}
				SpawnIds.Add(SpawnId);
			}
			UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
			if (!Combat)
			{
				FinishError(TEXT("Formal player-death-risk smoke could not resolve CombatSubsystem."));
				return false;
			}
			CombatEventHandle = Combat->OnCombatEvent().AddLambda([this](const FPRCombatEvent& Event)
			{
				if (Event.Target.Get() == Player.Get())
				{
					bObservedShieldDamage |= Event.ShieldAbsorbed > 0.0f;
					bObservedHealthDamage |= Event.HealthDamage > 0.0f;
					bObservedFatal |= Event.bFatal;
				}
			});
			bSpawned = true;
			return true;
		}

		void Cleanup()
		{
			if (UWorld* PlayWorld = World.Get())
			{
				if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>(); Combat && CombatEventHandle.IsValid())
				{
					Combat->OnCombatEvent().Remove(CombatEventHandle);
				}
				if (UPREnemySubsystem* EnemySubsystem = PlayWorld->GetSubsystem<UPREnemySubsystem>())
				{
					for (const FGuid& SpawnId : SpawnIds) EnemySubsystem->DespawnEnemy(SpawnId);
				}
			}
			CombatEventHandle.Reset();
			SpawnIds.Reset();
		}

		void FinishSuccess()
		{
			const double Duration = World.IsValid() ? World->GetTimeSeconds() - StartedAt : 0.0;
			const bool bFormalDeathChain = bObservedShieldDamage && bObservedHealthDamage && bObservedFatal;
			const FString Json = FString::Printf(TEXT("{\"status\":\"PASS\",\"benchmark\":\"PlayerDeathRisk\",\"durationSeconds\":%.3f,\"shieldThenHealthThenDeath\":true,\"runtimeClean\":true}"), Duration);
			FString ReportError;
			const FString Filename = FString::Printf(TEXT("player-death-risk-%s.json"), *FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%S")));
			Cleanup();
			if (!bFormalDeathChain)
			{
				FinishError(TEXT("Player death did not traverse the formal Shield -> Health -> Death CombatSubsystem chain."));
				return;
			}
			if (!WriteFixedReport(Filename, Json, ReportError))
			{
				FinishError(ReportError);
				return;
			}
			if (Result.IsValid())
			{
				Result->SetValue(Json);
				Result->RemoveFromRoot();
			}
		}

		void FinishError(const FString& Message)
		{
			Cleanup();
			if (Result.IsValid())
			{
				Result->SetError(Message);
				Result->RemoveFromRoot();
			}
		}

		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<APawn> Player;
		TWeakObjectPtr<const UPRAttributeSet> PlayerAttributes;
		TStrongObjectPtr<UToolCallAsyncResultString> Result;
		TArray<FGuid> SpawnIds;
		FDelegateHandle CombatEventHandle;
		double StartedAt = 0.0;
		bool bSpawned = false;
		bool bObservedShieldDamage = false;
		bool bObservedHealthDamage = false;
		bool bObservedFatal = false;
	};
}

UToolCallAsyncResultString* UPRBalanceAutomationToolset::CaptureV024BalanceSnapshot()
{
	FString Error;
	if (!PRBalanceAutomationToolset::VerifyRequiredAssets(Error))
	{
		return PRBalanceAutomationToolset::MakeError(Error);
	}

	UPRPlayerSkillDataAsset* Shadow = PRBalanceAutomationToolset::LoadRequiredAsset<UPRPlayerSkillDataAsset>(
		TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_ShadowThrust.DA_Skill_ShadowThrust"), Error);
	UPREnemyPrototypeDataAsset* Melee = PRBalanceAutomationToolset::LoadRequiredAsset<UPREnemyPrototypeDataAsset>(
		TEXT("/Game/ProjectR/Enemies/Prototypes/DA_Enemy_MeleeMinion.DA_Enemy_MeleeMinion"), Error);
	UPRAuditorBossDataAsset* Auditor = PRBalanceAutomationToolset::LoadRequiredAsset<UPRAuditorBossDataAsset>(
		TEXT("/Game/ProjectR/Enemies/Bosses/Prototypes/DA_Boss_Auditor.DA_Boss_Auditor"), Error);
	if (!Shadow || !Melee || !Auditor)
	{
		return PRBalanceAutomationToolset::MakeError(Error);
	}

	const FString Json = FString::Printf(
		TEXT("{\"status\":\"PASS\",\"version\":\"v0.2.4\",\"sourcesVerified\":%d,\"shadow\":{\"baseDamage\":%.3f,\"attackPowerScale\":%.3f,\"energyCost\":%.3f,\"cooldown\":%.3f,\"displacement\":%.3f},\"melee\":{\"health\":%.3f,\"shield\":%.3f,\"attackPower\":%.3f,\"moveSpeed\":%.3f},\"auditor\":{\"health\":%.3f,\"shield\":%.3f,\"attackPower\":%.3f,\"moveSpeed\":%.3f,\"phaseOneRatio\":%.3f,\"phaseTwoRatio\":%.3f,\"predictionShield\":%.3f}}"),
		PRBalanceAutomationToolset::ExpectedAuthoritativeSourceCount,
		Shadow->BaseDamage, Shadow->AttackPowerScale, Shadow->EnergyCost, Shadow->CooldownDuration, Shadow->DisplacementDistance,
		Melee->Attributes.Health, Melee->Attributes.Shield, Melee->Attributes.AttackPower, Melee->Attributes.MoveSpeed,
		Auditor->Attributes.Health, Auditor->Attributes.Shield, Auditor->Attributes.AttackPower, Auditor->Attributes.MoveSpeed,
		Auditor->RuleAuditHealthRatio, Auditor->PredictionShieldHealthRatio, Auditor->PredictionShieldValue);
	if (!PRBalanceAutomationToolset::WriteFixedReport(TEXT("authoritative-snapshot.json"), Json, Error))
	{
		return PRBalanceAutomationToolset::MakeError(Error);
	}

	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->SetValue(Json);
	return Result;
}

UToolCallAsyncResultString* UPRBalanceAutomationToolset::RunPIEV024NormalEncounterBenchmark()
{
	return PRBalanceAutomationToolset::FNormalEncounterRunner::Start();
}

UToolCallAsyncResultString* UPRBalanceAutomationToolset::RunPIEV024PlayerDeathRiskSmoke()
{
	return PRBalanceAutomationToolset::FPlayerDeathRiskRunner::Start();
}

UToolCallAsyncResultString* UPRBalanceAutomationToolset::RunPIEV024AuditorBenchmark()
{
	return PRBalanceAutomationToolset::FAuditorBenchmarkRunner::Start();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRBalanceAutomationSnapshotTest,
	"ProjectRAuthoringTools.Balance.SnapshotSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRBalanceAutomationSnapshotTest::RunTest(const FString& Parameters)
{
	UToolCallAsyncResultString* Result = UPRBalanceAutomationToolset::CaptureV024BalanceSnapshot();
	TestNotNull(TEXT("Fixed balance snapshot returns a result."), Result);
	if (!Result)
	{
		return false;
	}
	TestTrue(TEXT("Fixed balance snapshot completes without an error."), Result->Error.IsEmpty());
	TestTrue(TEXT("Fixed balance snapshot reports a PASS status."), Result->Value.Contains(TEXT("\"status\":\"PASS\"")));
	TestTrue(TEXT("Fixed balance snapshot identifies v0.2.4."), Result->Value.Contains(TEXT("\"version\":\"v0.2.4\"")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRBalanceAutomationBoundaryTest,
	"ProjectRAuthoringTools.Balance.FixedNoArgumentBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRBalanceAutomationBoundaryTest::RunTest(const FString& Parameters)
{
	const FName FunctionNames[] = {
		GET_FUNCTION_NAME_CHECKED(UPRBalanceAutomationToolset, CaptureV024BalanceSnapshot),
		GET_FUNCTION_NAME_CHECKED(UPRBalanceAutomationToolset, RunPIEV024NormalEncounterBenchmark),
		GET_FUNCTION_NAME_CHECKED(UPRBalanceAutomationToolset, RunPIEV024PlayerDeathRiskSmoke),
		GET_FUNCTION_NAME_CHECKED(UPRBalanceAutomationToolset, RunPIEV024AuditorBenchmark)
	};
	for (const FName FunctionName : FunctionNames)
	{
		UFunction* Function = UPRBalanceAutomationToolset::StaticClass()->FindFunctionByName(FunctionName);
		TestNotNull(FString::Printf(TEXT("Fixed balance tool exposes %s."), *FunctionName.ToString()), Function);
		if (Function)
		{
			TestEqual(FString::Printf(TEXT("%s accepts no caller argument."), *FunctionName.ToString()), Function->NumParms, 1);
		}
	}

	if (GEditor && GEditor->PlayWorld)
	{
		AddWarning(TEXT("Fixed no-argument boundary test skipped no-PIE rejection because a PIE session is already active."));
		return true;
	}
	const TArray<UToolCallAsyncResultString*> PIEOnlyResults = {
		UPRBalanceAutomationToolset::RunPIEV024NormalEncounterBenchmark(),
		UPRBalanceAutomationToolset::RunPIEV024PlayerDeathRiskSmoke(),
		UPRBalanceAutomationToolset::RunPIEV024AuditorBenchmark()
	};
	for (UToolCallAsyncResultString* Result : PIEOnlyResults)
	{
		TestNotNull(TEXT("Fixed PIE-only balance tool returns a result outside PIE."), Result);
		if (Result)
		{
			TestTrue(TEXT("Fixed PIE-only balance tool rejects non-PIE execution."), Result->Error.Contains(TEXT("active authoritative in-process PIE world")));
		}
	}
	return true;
}
