// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRQTEAutomationToolset.h"

#include "Abilities/PRAttributeSet.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Companions/PRCompanionPawn.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRTagLibrary.h"
#include "Editor.h"
#include "Enemies/Bosses/PRBossAuditor.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "QTE/PRQTESubsystem.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRQTEAutomationToolset
{
enum class ESmokeKind : uint8 { Axiom, Kindle, Null };

class FPIEQTESmokeRunner final : public TSharedFromThis<FPIEQTESmokeRunner>
{
public:
	static UToolCallAsyncResultString* Start(const ESmokeKind InKind, const bool bInAutoSubmit = true)
	{
		TSharedRef<FPIEQTESmokeRunner> Runner = MakeShared<FPIEQTESmokeRunner>();
		Runner->Kind = InKind;
		Runner->bAutoSubmit = bInAutoSubmit;
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!Runner->World.IsValid() || Runner->World->GetNetMode() == NM_Client)
		{
			Runner->Fail(TEXT("Fixed QTE smoke requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		Runner->StartedAt = Runner->World->GetTimeSeconds();
		if (UPRQTESubsystem* QTE = Runner->World->GetSubsystem<UPRQTESubsystem>())
		{
			Runner->ResultHandle = QTE->OnQTEResult().AddLambda([Weak = Runner.ToWeakPtr()](const FPRQTEResult& Event)
			{
				if (const TSharedPtr<FPIEQTESmokeRunner> Pinned = Weak.Pin()) Pinned->OnResult(Event);
			});
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float)
		{
			const bool bContinue = KeepAlive->Tick();
			return bContinue && !KeepAlive->bCompleted;
		}));
		return Runner->Result.Get();
	}

private:
	FGameplayTag CompanionTag() const
	{
		return Kind == ESmokeKind::Axiom ? FPRCompanionContract::AxiomTag()
			: Kind == ESmokeKind::Kindle ? FPRCompanionContract::KindleTag() : FPRCompanionContract::NullTag();
	}
	FGameplayTag SpawnTag() const
	{
		return Kind == ESmokeKind::Axiom ? FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard"))
			: FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion"));
	}
	FName ExpectedQTEId() const
	{
		return Kind == ESmokeKind::Axiom ? TEXT("Axiom_WeaknessAxiom")
			: Kind == ESmokeKind::Kindle ? TEXT("Kindle_DetonationRelay") : TEXT("Null_VulnerabilityBackstab");
	}
	FGameplayTag AcceptedInput() const
	{
		return Kind == ESmokeKind::Kindle ? UPRTagLibrary::GetInputAttackTag() : UPRTagLibrary::GetInputSkillShadowThrustTag();
	}
	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		if (!PlayWorld || PlayWorld->GetTimeSeconds() - StartedAt > 15.0)
		{
			Fail(TEXT("Fixed QTE smoke timed out before a single deterministic result."));
			return false;
		}
		UGameInstance* Instance = PlayWorld->GetGameInstance();
		UPRCompanionSubsystem* Companions = Instance ? Instance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
		UPREnemySubsystem* Enemies = PlayWorld->GetSubsystem<UPREnemySubsystem>();
		UPRQTESubsystem* QTE = PlayWorld->GetSubsystem<UPRQTESubsystem>();
		UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		APlayerController* Controller = PlayWorld->GetFirstPlayerController();
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		if (!Companions || !Enemies || !QTE || !Combat || !Player || !QTE->IsRegistryReady()) return true;
		if (Stage == 0 && QTE->GetRuntimeState().State != EPRQTEState::Idle) return true;
		if (Stage == 0)
		{
			const EPRPrimaryCompanionSelectionResult Select = Companions->SelectPrimaryCompanion(CompanionTag());
			if (Select == EPRPrimaryCompanionSelectionResult::RejectedRegistryUnavailable) return true;
			if (!Companions->GetSyncState().PrimaryCompanionId.MatchesTagExact(CompanionTag()))
			{
				Fail(TEXT("Fixed QTE smoke could not select its formal primary companion.")); return false;
			}
			Stage = 1; return true;
		}
		if (Stage == 1)
		{
			APREnemyCharacter* Spawned = nullptr;
			const EPREnemySpawnStatus Status = Enemies->SpawnEnemyPrototype(SpawnTag(), FTransform(Player->GetActorRotation(), Player->GetActorLocation() + FVector(300.0f, 0.0f, 0.0f)), SpawnId, Spawned);
			if (Status == EPREnemySpawnStatus::NotReady) return true;
			if (Status != EPREnemySpawnStatus::Spawned || !Spawned) { Fail(TEXT("Fixed QTE smoke could not spawn its whitelist target.")); return false; }
			Enemy = Spawned; Stage = 2; return true;
		}
		if (Stage == 2)
		{
			APREnemyCharacter* Target = Enemy.Get();
			if (!Target || !Target->IsEnemyInitialized()) return true;
			if (Kind == ESmokeKind::Null)
			{
				FPRCombatOutcomeRequest Outcome; Outcome.SourceId = TEXT("QTEFixedSmoke"); Outcome.AbilitySource = Player; Outcome.Instigator = Player; Outcome.Target = Target;
				Outcome.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge")); Outcome.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseDecoyConsumedTag());
				if (!Combat->PublishAbilityOutcome(Outcome)) { Fail(TEXT("Fixed Null QTE smoke could not publish the formal DecoyConsumed outcome.")); return false; }
			}
			else
			{
				FPRDamageRequest Damage; Damage.SourceId = TEXT("QTEFixedSmoke"); Damage.DamageSource = Player; Damage.Instigator = Player; Damage.Target = Target;
				Damage.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust")); Damage.RawDamage = Kind == ESmokeKind::Axiom ? 160.0f : 1.0f;
				Damage.ImpactOrigin = Player->GetActorLocation(); Damage.IncomingDirection = (Target->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
				if (Combat->ApplyDamage(Damage) != EPRCombatRequestStatus::Applied) { Fail(TEXT("Fixed QTE smoke could not produce its formal CombatEvent.")); return false; }
			}
			Stage = 3; return true;
		}
		if (Stage == 3)
		{
			if (QTE->GetRuntimeState().State != EPRQTEState::Active) return true;
			if (!bAutoSubmit)
			{
				Stage = 4;
				return true;
			}
			if (!QTE->SubmitSemanticInput(AcceptedInput(), PlayWorld->GetTimeSeconds())) { Fail(TEXT("Fixed QTE smoke could not submit the accepted semantic input.")); return false; }
			Stage = 4; return true;
		}
		return true;
	}
	void OnResult(const FPRQTEResult& Event)
	{
		if (bCompleted) return;
		if (Event.QTEId.PrimaryAssetName != ExpectedQTEId()) { Fail(TEXT("Fixed QTE smoke resolved a different QTE than its formal contract.")); return; }
		if (bAutoSubmit && !Event.ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultSuccessTag())) return;
		bCompleted = true;
		Cleanup();
		Result->SetValue(FString::Printf(TEXT("{\"status\":\"%s\",\"companion\":\"%s\",\"qte\":\"%s\",\"result\":\"%s\",\"runtimeClean\":true}"),
			bAutoSubmit ? TEXT("PASS") : TEXT("PREVIEW_COMPLETE"), *Event.CompanionId.ToString(), *Event.QTEId.ToString(), *Event.ResultTag.ToString()));
	}
	void Cleanup()
	{
		if (UWorld* PlayWorld = World.Get())
		{
			if (UPRQTESubsystem* QTE = PlayWorld->GetSubsystem<UPRQTESubsystem>(); QTE && ResultHandle.IsValid()) QTE->OnQTEResult().Remove(ResultHandle);
			if (UPREnemySubsystem* Enemies = PlayWorld->GetSubsystem<UPREnemySubsystem>(); Enemies && SpawnId.IsValid()) Enemies->DespawnEnemy(SpawnId);
		}
		ResultHandle.Reset(); SpawnId.Invalidate(); Enemy.Reset();
	}
	void Fail(const FString& Message)
	{
		if (bCompleted) return;
		bCompleted = true;
		Cleanup();
		Result->SetError(Message);
	}

	ESmokeKind Kind = ESmokeKind::Axiom; int32 Stage = 0; double StartedAt = 0.0; bool bAutoSubmit = true; bool bCompleted = false; TWeakObjectPtr<UWorld> World; TWeakObjectPtr<APREnemyCharacter> Enemy; FGuid SpawnId; FDelegateHandle ResultHandle; TStrongObjectPtr<UToolCallAsyncResultString> Result;
};

enum class EContractKind : uint8
{
	AxiomProbability, AxiomPhaseCut, AxiomCooperation,
	KindleBreakout, KindleBurnout, KindleReverseBurn,
	NullJudgment, NullGarbage, NullVerdict
};

/** One no-argument runner per remaining QTE. It creates only transient whitelist enemies and always removes them. */
class FPIEQTEContractRunner final : public TSharedFromThis<FPIEQTEContractRunner>
{
public:
	static UToolCallAsyncResultString* Start(const EContractKind InKind)
	{
		TSharedRef<FPIEQTEContractRunner> Runner = MakeShared<FPIEQTEContractRunner>();
		Runner->Kind = InKind;
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!Runner->World.IsValid() || Runner->World->GetNetMode() == NM_Client)
		{
			Runner->Fail(TEXT("Fixed QTE contract runner requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		Runner->StartedAt = Runner->World->GetTimeSeconds();
		Runner->StartedAtWallSeconds = FPlatformTime::Seconds();
		if (UPRQTESubsystem* QTE = Runner->World->GetSubsystem<UPRQTESubsystem>())
		{
			Runner->ResultHandle = QTE->OnQTEResult().AddLambda([Weak = Runner.ToWeakPtr()](const FPRQTEResult& Event)
			{
				if (const TSharedPtr<FPIEQTEContractRunner> Pinned = Weak.Pin()) Pinned->OnResult(Event);
			});
		}
		if (UPRCombatSubsystem* Combat = Runner->World->GetSubsystem<UPRCombatSubsystem>())
		{
			Runner->CombatHandle = Combat->OnCombatEvent().AddLambda([Weak = Runner.ToWeakPtr()](const FPRCombatEvent& Event)
			{
				if (const TSharedPtr<FPIEQTEContractRunner> Pinned = Weak.Pin())
				{
					Pinned->LastHealthDamage = Event.HealthDamage;
					Pinned->LastRemainingHealth = Event.RemainingHealth;
					Pinned->bLastCombatEventFatal = Event.bFatal;
				}
			});
		}
		if (UPRCompanionRuntimeSubsystem* Runtime = Runner->World->GetSubsystem<UPRCompanionRuntimeSubsystem>())
		{
			Runner->SupportHandle = Runtime->OnSupportEvent().AddLambda([Weak = Runner.ToWeakPtr()](const FPRCompanionSupportEvent& Event)
			{
				if (const TSharedPtr<FPIEQTEContractRunner> Pinned = Weak.Pin())
				{
					if (Event.CompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag())
						&& Event.Result == EPRCompanionSupportResult::Applied)
					{
						++Pinned->AxiomAppliedSupportCount;
						Pinned->NextAxiomSupportWorldTime = Event.WorldTimeSeconds + Event.EffectiveIntervalSeconds;
					}
				}
			});
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float)
		{
			const bool bContinue = KeepAlive->Tick();
			return bContinue && !KeepAlive->bCompleted;
		}));
		return Runner->Result.Get();
	}

private:
	bool IsBossScenario() const
	{
		return Kind == EContractKind::AxiomPhaseCut || Kind == EContractKind::AxiomCooperation || Kind == EContractKind::NullVerdict;
	}
	bool IsSupportScenario() const { return Kind == EContractKind::AxiomPhaseCut || Kind == EContractKind::KindleBreakout; }
	FGameplayTag CompanionTag() const
	{
		if (Kind == EContractKind::AxiomProbability || Kind == EContractKind::AxiomPhaseCut || Kind == EContractKind::AxiomCooperation) return FPRCompanionContract::AxiomTag();
		if (Kind == EContractKind::KindleBreakout || Kind == EContractKind::KindleBurnout || Kind == EContractKind::KindleReverseBurn) return FPRCompanionContract::KindleTag();
		return FPRCompanionContract::NullTag();
	}
	FName ExpectedQTEId() const
	{
		switch (Kind)
		{
		case EContractKind::AxiomProbability: return TEXT("Axiom_ProbabilityShield");
		case EContractKind::AxiomPhaseCut: return TEXT("Axiom_PhaseCut");
		case EContractKind::AxiomCooperation: return TEXT("Axiom_CooperativeRefutation");
		case EContractKind::KindleBreakout: return TEXT("Kindle_Breakout");
		case EContractKind::KindleBurnout: return TEXT("Kindle_BurnoutTornado");
		case EContractKind::KindleReverseBurn: return TEXT("Kindle_ReverseBurnRescue");
		case EContractKind::NullJudgment: return TEXT("Null_DecoyJudgment");
		case EContractKind::NullGarbage: return TEXT("Null_GarbageCollection");
		default: return TEXT("Null_VerdictTamper");
		}
	}
	FGameplayTag AcceptedInput() const
	{
		switch (Kind)
		{
		case EContractKind::AxiomProbability: case EContractKind::KindleReverseBurn: return UPRTagLibrary::GetInputSkillCounterProofWallTag();
		case EContractKind::AxiomPhaseCut: return UPRTagLibrary::GetInputSkillThunderDropTag();
		case EContractKind::AxiomCooperation: case EContractKind::NullVerdict: return UPRTagLibrary::GetInputInteractTag();
		case EContractKind::KindleBreakout: return UPRTagLibrary::GetInputSkillFireSlashTag();
		case EContractKind::KindleBurnout: case EContractKind::NullJudgment: return UPRTagLibrary::GetInputExecuteTag();
		default: return UPRTagLibrary::GetInputAttackTag();
		}
	}
	double TimeoutSeconds() const { return IsSupportScenario() ? 50.0 : (IsBossScenario() ? 30.0 : 15.0); }

	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		if (!PlayWorld || FPlatformTime::Seconds() - StartedAtWallSeconds > TimeoutSeconds())
		{
			Fail(FString::Printf(TEXT("Fixed QTE contract %s timed out at stage %d (last health damage %.2f, remaining health %.2f, fatal %s, boss attack phase %d, Axiom applied supports %d, P2 Counter armed %s)."),
				*ExpectedQTEId().ToString(), Stage, LastHealthDamage, LastRemainingHealth, bLastCombatEventFatal ? TEXT("true") : TEXT("false"),
				LastBossAttackPhase, AxiomAppliedSupportCount, bPhaseCutCounterArmed ? TEXT("true") : TEXT("false")));
			return false;
		}
		UGameInstance* Instance = PlayWorld->GetGameInstance();
		UPRCompanionSubsystem* Companions = Instance ? Instance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
		UPRQTESubsystem* QTE = PlayWorld->GetSubsystem<UPRQTESubsystem>();
		UPREnemySubsystem* Enemies = PlayWorld->GetSubsystem<UPREnemySubsystem>();
		UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		APlayerController* Controller = PlayWorld->GetFirstPlayerController();
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		if (!Companions || !QTE || !Combat || !Player || !QTE->IsRegistryReady()) return true;
		if (Stage == 0)
		{
			const FGameplayTag InitialCompanion = IsBossScenario()
				? FPRCompanionContract::NullTag() : CompanionTag();
			if (Companions->SelectPrimaryCompanion(InitialCompanion) == EPRPrimaryCompanionSelectionResult::RejectedRegistryUnavailable) return true;
			if (!Companions->GetSyncState().PrimaryCompanionId.MatchesTagExact(InitialCompanion)) { Fail(TEXT("Fixed QTE contract could not select its primary companion.")); return false; }
			Stage = 1;
			return true;
		}
		if (IsBossScenario()) return TickBossScenario(*Companions, *QTE, *Combat, *Player);
		if (!Enemies) return true;
		if (Stage == 1)
		{
			APRCompanionPawn* CompanionPawn = nullptr;
			if (IsSupportScenario())
			{
				UPRCompanionRuntimeSubsystem* Runtime = PlayWorld->GetSubsystem<UPRCompanionRuntimeSubsystem>();
				CompanionPawn = Runtime ? Runtime->GetActiveCompanionPawn() : nullptr;
				if (!CompanionPawn) return true;
			}
			const int32 Count = (Kind == EContractKind::KindleBreakout || Kind == EContractKind::KindleBurnout) ? 3 : 1;
			const FGameplayTag SpawnTag = Kind == EContractKind::AxiomProbability ? FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")) : FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion"));
			for (int32 Index = 0; Index < Count; ++Index)
			{
				APREnemyCharacter* Spawned = nullptr;
				const FVector Anchor = CompanionPawn ? CompanionPawn->GetActorLocation() : Player->GetActorLocation();
				const FTransform SpawnTransform(Player->GetActorRotation(), Anchor + FVector(120.0f + 40.0f * Index, 0.0f, 0.0f));
				FGuid SpawnId;
				const EPREnemySpawnStatus Status = Enemies->SpawnEnemyPrototype(SpawnTag, SpawnTransform, SpawnId, Spawned);
				if (Status == EPREnemySpawnStatus::NotReady) return true;
				if (Status != EPREnemySpawnStatus::Spawned || !Spawned) { Fail(TEXT("Fixed QTE contract could not spawn its whitelist target.")); return false; }
				SpawnIds.Add(SpawnId); Targets.Add(Spawned);
			}
			Stage = 2;
			return true;
		}
		if (Stage >= 2)
		{
			for (const TWeakObjectPtr<APREnemyCharacter>& Target : Targets)
			{
				if (!Target.IsValid() || !Target->IsEnemyInitialized()) return true;
			}
			MaintainKindleBreakoutTargetControl();
		}
		if (!Targets.IsEmpty() && !Targets[0].IsValid()) { Fail(TEXT("Fixed QTE contract lost its transient target.")); return false; }
		if (Stage == 2)
		{
			if (IsSupportScenario())
			{
				if (QTE->GetRuntimeState().State == EPRQTEState::Active) Stage = 3;
				return true;
			}
			if (Kind == EContractKind::KindleReverseBurn)
			{
				if (!ApplyEnemyDamage(*Combat, *Targets[0].Get(), *Player, 100.0f)) { Fail(TEXT("ReverseBurn setup could not apply its first formal nonfatal hit.")); return false; }
				DeferredThresholdTime = PlayWorld->GetTimeSeconds() + 0.25;
				Stage = 21;
				return true;
			}
			if (Kind == EContractKind::NullGarbage)
			{
				const UPRAttributeSet* Attributes = Targets[0]->GetAttributeSet();
				if (!Attributes || !FMath::IsFinite(Attributes->GetHealth()) || Attributes->GetHealth() <= UE_SMALL_NUMBER)
				{
					Fail(TEXT("GarbageCollection setup could not read a positive initialized target Health value."));
					return false;
				}
				ThresholdFirstDamage = Attributes->GetHealth() * 0.50f;
				ThresholdSecondDamage = Attributes->GetHealth() * 0.35f;
				if (!ApplyPlayerDamage(*Combat, *Player, *Targets[0].Get(), TEXT("Skill.FireSlash"), ThresholdFirstDamage)) { Fail(TEXT("GarbageCollection setup could not apply its first formal proportional hit.")); return false; }
				DeferredThresholdTime = PlayWorld->GetTimeSeconds() + 0.25;
				Stage = 21;
				return true;
			}
			if (!TriggerStandardScenario(*Combat, *Player)) return false;
			Stage = 3;
			return true;
		}
		if (Stage == 21)
		{
			if (PlayWorld->GetTimeSeconds() < DeferredThresholdTime) return true;
			const bool bApplied = Kind == EContractKind::KindleReverseBurn
				? ApplyEnemyDamage(*Combat, *Targets[0].Get(), *Player, 30.0f)
				: ApplyPlayerDamage(*Combat, *Player, *Targets[0].Get(), TEXT("Skill.FireSlash"), ThresholdSecondDamage);
			if (!bApplied) { Fail(FString::Printf(TEXT("Fixed QTE threshold setup could not apply its second formal hit (Combat status %d)."), static_cast<int32>(LastCombatStatus))); return false; }
			Stage = 3;
			return true;
		}
		if (Stage == 3)
		{
			if (QTE->GetRuntimeState().State != EPRQTEState::Active) return true;
			const FPRQTERuntimeState State = QTE->GetRuntimeState();
			if (State.QTEId.PrimaryAssetName != ExpectedQTEId())
			{
				Fail(FString::Printf(TEXT("Fixed QTE contract activated %s instead of %s."), *State.QTEId.ToString(), *ExpectedQTEId().ToString()));
				return false;
			}
			if (!QTE->SubmitSemanticInput(AcceptedInput(), PlayWorld->GetTimeSeconds()))
			{
				Fail(FString::Printf(TEXT("Fixed QTE contract rejected input %s for %s."), *AcceptedInput().ToString(), *State.QTEId.ToString()));
				return false;
			}
			Stage = Kind == EContractKind::NullGarbage ? 4 : 5;
			return true;
		}
		if (Stage == 4)
		{
			if (APREnemyCharacter* Target = Targets[0].Get())
			{
				if (!ApplyPlayerDamage(*Combat, *Player, *Target, TEXT("Skill.BasicAttack"), 200.0f)) { Fail(TEXT("GarbageCollection could not verify a formal BasicAttack kill.")); return false; }
				Stage = 5;
			}
			return true;
		}
		return true;
	}

	bool TickBossScenario(UPRCompanionSubsystem& Companions, UPRQTESubsystem& QTE, UPRCombatSubsystem& Combat, APawn& Player)
	{
		APRBossAuditor* Boss = FindBoss();
		if (!Boss || !Boss->IsEnemyInitialized()) return true;
		if (!bBossScenarioPositioned)
		{
			BossScenarioPlayerStart = Player.GetActorTransform();
			Player.SetActorLocation(
				FVector(Boss->GetActorLocation().X + 220.0f, Boss->GetActorLocation().Y, Boss->GetActorLocation().Z),
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			bBossScenarioPositioned = true;
		}
		UPRBossSubsystem* Bosses = World.Get()->GetSubsystem<UPRBossSubsystem>();
		FPRBossRuntimeState BossState;
		if (Bosses && Bosses->GetActiveBossState(BossState)) LastBossAttackPhase = BossState.AttackPhase;
		if (Kind == EContractKind::AxiomPhaseCut)
		{
			if (!bPhaseCutSeeded)
			{
				if (!ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 50.0f)
					|| !ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 50.0f)
					|| !ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 50.0f)
					|| !ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 280.0f))
				{
					Fail(TEXT("Axiom PhaseCut could not enter Auditor RuleAudit through formal CombatSubsystem sampling hits."));
					return false;
				}
				bPhaseCutSeeded = true;
				return true;
			}
			if (BossState.Phase != EPRAuditorBossPhase::RuleAudit) return true;
			if (!bPhaseCutAxiomSelected)
			{
				if (!Companions.GetSyncState().PrimaryCompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag()))
				{
					Companions.SelectPrimaryCompanion(FPRCompanionContract::AxiomTag());
					return true;
				}
				bPhaseCutAxiomSelected = true;
				return true;
			}
			if (!bPhaseCutCounterArmed
				&& NextAxiomSupportWorldTime > 0.0
				&& World.Get()->GetTimeSeconds() >= NextAxiomSupportWorldTime - 0.25)
			{
				if (!ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 1.0f))
				{
					Fail(TEXT("Axiom PhaseCut could not formally arm AuditorCounter before the next Axiom support."));
					return false;
				}
				bPhaseCutCounterArmed = true;
			}
			if (QTE.GetRuntimeState().State == EPRQTEState::Active)
			{
				if (QTE.GetRuntimeState().QTEId.PrimaryAssetName != ExpectedQTEId())
				{
					Fail(FString::Printf(TEXT("Axiom PhaseCut expected %s but the active prompt was %s."),
						*ExpectedQTEId().ToString(), *QTE.GetRuntimeState().QTEId.ToString()));
					return false;
				}
				if (!QTE.SubmitSemanticInput(AcceptedInput(), World.Get()->GetTimeSeconds()))
				{
					Fail(TEXT("Axiom PhaseCut could not submit ThunderDrop after a real support-triggered prompt."));
					return false;
				}
				Stage = 6;
				return true;
			}
			return true; // Real Axiom shield support is the only legal trigger; wait for a live Auditor Windup.
		}
		if (!Bosses || !Bosses->GetActiveBossState(BossState)) return true;
		if (Stage == 1)
		{
			if (!ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 50.0f)
				|| !ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 50.0f)
				|| !ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 50.0f)
				|| !ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.ShadowThrust"), 280.0f)) { Fail(TEXT("QTE Boss prediction setup could not create formal sampling events.")); return false; }
			Stage = 2;
			return true;
		}
		if (Stage == 2)
		{
			if (BossState.Phase != EPRAuditorBossPhase::RuleAudit) return true;
			if (!ApplyPlayerDamage(Combat, Player, *Boss, TEXT("Skill.FireSlash"), 300.0f)) { Fail(TEXT("QTE Boss prediction setup could not enter P3 through CombatSubsystem.")); return false; }
			Stage = 3;
			return true;
		}
		if (Stage == 3)
		{
			if (BossState.Phase != EPRAuditorBossPhase::PredictionShield || !BossState.PredictedAbilityTag.IsValid()) return true;
			if (!Companions.GetSyncState().PrimaryCompanionId.MatchesTagExact(CompanionTag()))
			{
				Companions.SelectPrimaryCompanion(CompanionTag());
				return true;
			}
			if (Combat.ApplyDamage(MakePlayerDamageRequest(Player, *Boss, BossState.PredictedAbilityTag, 10.0f)) != EPRCombatRequestStatus::RejectedBlocked)
			{
				Fail(TEXT("QTE Boss prediction setup did not receive the formal PredictionBlocked result."));
				return false;
			}
			Stage = 4;
			return true;
		}
		if (Stage == 4 && QTE.GetRuntimeState().State == EPRQTEState::Active) { Stage = 5; return true; }
		if (Stage == 5)
		{
			if (QTE.GetRuntimeState().State != EPRQTEState::Active) return true;
			if (!QTE.SubmitSemanticInput(AcceptedInput(), World.Get()->GetTimeSeconds())) { Fail(TEXT("QTE Boss contract could not submit Interact.")); return false; }
			Stage = 6;
		}
		return true;
	}

	APRBossAuditor* FindBoss() const
	{
		if (!World.IsValid()) return nullptr;
		for (TActorIterator<APRBossAuditor> It(World.Get()); It; ++It) if ((*It)->IsEnemyInitialized()) return *It;
		return nullptr;
	}
	FPRDamageRequest MakePlayerDamageRequest(APawn& Player, APREnemyCharacter& Target, const FGameplayTag AbilityTag, const float Damage) const
	{
		FPRDamageRequest Request; Request.SourceId = TEXT("QTEFullContractSmoke"); Request.DamageSource = &Player; Request.Instigator = &Player; Request.Target = &Target;
		Request.AbilityTag = AbilityTag; Request.RawDamage = Damage; Request.ImpactOrigin = Player.GetActorLocation(); Request.IncomingDirection = (Target.GetActorLocation() - Player.GetActorLocation()).GetSafeNormal2D();
		return Request;
	}
	bool ApplyPlayerDamage(UPRCombatSubsystem& Combat, APawn& Player, APREnemyCharacter& Target, const TCHAR* AbilityTag, const float Damage)
	{
		LastCombatStatus = Combat.ApplyDamage(MakePlayerDamageRequest(Player, Target, FGameplayTag::RequestGameplayTag(AbilityTag), Damage));
		return LastCombatStatus == EPRCombatRequestStatus::Applied;
	}
	bool ApplyEnemyDamage(UPRCombatSubsystem& Combat, APREnemyCharacter& Source, APawn& Player, const float Damage)
	{
		FPRDamageRequest Request; Request.SourceId = TEXT("QTEFullContractSmokeEnemy"); Request.DamageSource = &Source; Request.Instigator = &Source; Request.Target = &Player;
		Request.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.MeleeStrike")); Request.RawDamage = Damage; Request.ImpactOrigin = Source.GetActorLocation(); Request.IncomingDirection = (Player.GetActorLocation() - Source.GetActorLocation()).GetSafeNormal2D();
		LastCombatStatus = Combat.ApplyDamage(Request);
		return LastCombatStatus == EPRCombatRequestStatus::Applied;
	}
	bool TriggerStandardScenario(UPRCombatSubsystem& Combat, APawn& Player)
	{
		APREnemyCharacter* Target = Targets[0].Get();
		if (!Target || !Target->IsEnemyInitialized()) return false;
		if (Kind == EContractKind::AxiomProbability) return ApplyEnemyDamage(Combat, *Target, Player, 125.0f);
		if (Kind == EContractKind::KindleBurnout)
		{
			for (const TWeakObjectPtr<APREnemyCharacter>& Candidate : Targets)
			{
				if (!Candidate.IsValid() || !ApplyPlayerDamage(Combat, Player, *Candidate.Get(), TEXT("Skill.FireSlash"), 1.0f)) return false;
			}
			return true;
		}
		if (Kind == EContractKind::NullJudgment)
		{
			FPRCombatOutcomeRequest Outcome; Outcome.SourceId = TEXT("QTEFullContractSmoke"); Outcome.AbilitySource = &Player; Outcome.Instigator = &Player; Outcome.Target = Target;
			Outcome.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge")); Outcome.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseDecoyConsumedTag()); Outcome.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponsePerfectTimingTag());
			return Combat.PublishAbilityOutcome(Outcome);
		}
		return false;
	}
	void MaintainKindleBreakoutTargetControl()
	{
		// The fixed Breakout fixture needs three live Light targets long enough for
		// Kindle's real four-second support timer.  Their existing attacks are not
		// part of this scenario: if they reduce the player below the rescue
		// threshold first, the valid Rescue QTE correctly wins by priority.  Keep
		// only these transient fixture targets under the existing Stunned GE; no
		// runtime rule or asset is changed, and DespawnEnemy clears the effects.
		if (Kind != EContractKind::KindleBreakout) return;
		UClass* StunnedClass = LoadClass<UGameplayEffect>(nullptr,
			TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Stunned.GE_State_Stunned_C"));
		if (!StunnedClass) return;
		for (const TWeakObjectPtr<APREnemyCharacter>& Target : Targets)
		{
			UPRAbilitySystemComponent* ASC = Target.IsValid() ? Target->GetProjectRAbilitySystemComponent() : nullptr;
			if (ASC && !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()))
			{
				ASC->ApplyGameplayEffectToSelf(StunnedClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
			}
		}
	}
	void OnResult(const FPRQTEResult& Event)
	{
		if (bCompleted || !Event.ResultTag.MatchesTagExact(UPRTagLibrary::GetQTEResultSuccessTag())) return;
		if (Event.QTEId.PrimaryAssetName != ExpectedQTEId()) { Fail(TEXT("Fixed QTE contract resolved a different QTE than its exact manifest entry.")); return; }
		bCompleted = true;
		Cleanup();
		Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"qte\":\"%s\",\"result\":\"Success\",\"testActorsClean\":true}"), *Event.QTEId.ToString()));
	}
	void Cleanup()
	{
		if (UWorld* PlayWorld = World.Get())
		{
			if (UPRQTESubsystem* QTE = PlayWorld->GetSubsystem<UPRQTESubsystem>(); QTE && ResultHandle.IsValid()) QTE->OnQTEResult().Remove(ResultHandle);
			if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>(); Combat && CombatHandle.IsValid()) Combat->OnCombatEvent().Remove(CombatHandle);
			if (UPRCompanionRuntimeSubsystem* Runtime = PlayWorld->GetSubsystem<UPRCompanionRuntimeSubsystem>(); Runtime && SupportHandle.IsValid()) Runtime->OnSupportEvent().Remove(SupportHandle);
			if (UPREnemySubsystem* Enemies = PlayWorld->GetSubsystem<UPREnemySubsystem>()) for (const FGuid& SpawnId : SpawnIds) Enemies->DespawnEnemy(SpawnId);
		}
		if (bBossScenarioPositioned)
		{
			if (APlayerController* Controller = World.IsValid() ? World->GetFirstPlayerController() : nullptr)
			{
				if (APawn* PlayerPawn = Controller->GetPawn())
				{
					PlayerPawn->SetActorTransform(BossScenarioPlayerStart, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
		}
		ResultHandle.Reset(); CombatHandle.Reset(); SupportHandle.Reset(); SpawnIds.Empty(); Targets.Empty();
	}
	void Fail(const FString& Message)
	{
		if (bCompleted) return;
		bCompleted = true;
		Cleanup();
		Result->SetError(Message);
	}

	EContractKind Kind = EContractKind::AxiomProbability;
	int32 Stage = 0;
	double StartedAt = 0.0;
	double DeferredThresholdTime = 0.0;
	bool bCompleted = false;
	TWeakObjectPtr<UWorld> World;
	TArray<FGuid> SpawnIds;
	TArray<TWeakObjectPtr<APREnemyCharacter>> Targets;
	FDelegateHandle ResultHandle;
	FDelegateHandle CombatHandle;
	FDelegateHandle SupportHandle;
	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	float LastHealthDamage = 0.0f;
	float LastRemainingHealth = 0.0f;
	float ThresholdFirstDamage = 0.0f;
	float ThresholdSecondDamage = 30.0f;
	bool bLastCombatEventFatal = false;
	EPRCombatRequestStatus LastCombatStatus = EPRCombatRequestStatus::Invalid;
	bool bBossScenarioPositioned = false;
	FTransform BossScenarioPlayerStart;
	double StartedAtWallSeconds = 0.0;
	uint8 LastBossAttackPhase = 0;
	int32 AxiomAppliedSupportCount = 0;
	double NextAxiomSupportWorldTime = 0.0;
	bool bPhaseCutSeeded = false;
	bool bPhaseCutAxiomSelected = false;
	bool bPhaseCutCounterArmed = false;
};
}

UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTEAxiomSmoke() { return PRQTEAutomationToolset::FPIEQTESmokeRunner::Start(PRQTEAutomationToolset::ESmokeKind::Axiom); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTEKindleSmoke() { return PRQTEAutomationToolset::FPIEQTESmokeRunner::Start(PRQTEAutomationToolset::ESmokeKind::Kindle); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTENullSmoke() { return PRQTEAutomationToolset::FPIEQTESmokeRunner::Start(PRQTEAutomationToolset::ESmokeKind::Null); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTEAxiomProbabilityShield() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::AxiomProbability); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTEAxiomPhaseCut() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::AxiomPhaseCut); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTEAxiomCooperativeRefutation() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::AxiomCooperation); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTEKindleBreakout() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::KindleBreakout); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTEKindleBurnoutTornado() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::KindleBurnout); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTEKindleReverseBurnRescue() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::KindleReverseBurn); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTENullDecoyJudgment() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::NullJudgment); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTENullGarbageCollection() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::NullGarbage); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::RunPIEQTENullVerdictTamper() { return PRQTEAutomationToolset::FPIEQTEContractRunner::Start(PRQTEAutomationToolset::EContractKind::NullVerdict); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::PreparePIEQTEAxiomHumanPreview() { return PRQTEAutomationToolset::FPIEQTESmokeRunner::Start(PRQTEAutomationToolset::ESmokeKind::Axiom, false); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::PreparePIEQTEKindleHumanPreview() { return PRQTEAutomationToolset::FPIEQTESmokeRunner::Start(PRQTEAutomationToolset::ESmokeKind::Kindle, false); }
UToolCallAsyncResultString* UPRQTEAutomationToolset::PreparePIEQTENullHumanPreview() { return PRQTEAutomationToolset::FPIEQTESmokeRunner::Start(PRQTEAutomationToolset::ESmokeKind::Null, false); }
