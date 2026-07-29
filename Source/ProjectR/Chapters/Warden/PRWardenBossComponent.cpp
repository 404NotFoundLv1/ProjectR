// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Warden/PRWardenBossComponent.h"

#include "Abilities/PRAttributeSet.h"
#include "Chapters/PRChapterContentRegistryDataAsset.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemyCharacter.h"
#include "Kismet/GameplayStatics.h"

UPRWardenBossComponent::UPRWardenBossComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRWardenBossComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>())
		{
			BoundCombat = Combat;
			CombatHandle = Combat->OnCombatEvent().AddUObject(this, &UPRWardenBossComponent::HandleCombatEvent);
		}
	}
}

void UPRWardenBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(PredictiveTimer), World->GetTimerManager().ClearTimer(LockdownWarningTimer), World->GetTimerManager().ClearTimer(LockdownTimer), World->GetTimerManager().ClearTimer(RiskMarkTimer);
	if (UPRCombatSubsystem* Combat = BoundCombat.Get()) Combat->OnCombatEvent().Remove(CombatHandle);
	CombatHandle.Reset();
	BoundCombat.Reset();
	RiskCounterSkills.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRWardenBossComponent::ConfigureChapterState(const int32 RiskPressure)
{
	RuntimeState.RiskPressure = FMath::Clamp(RiskPressure, 0, 4);
}

const FPRWardenBossRuntimeState& UPRWardenBossComponent::GetRuntimeState() const { return RuntimeState; }
APREnemyCharacter* UPRWardenBossComponent::GetBoss() const { return Cast<APREnemyCharacter>(GetOwner()); }
APawn* UPRWardenBossComponent::GetPlayerPawn() const { return UGameplayStatics::GetPlayerPawn(this, 0); }

bool UPRWardenBossComponent::IsLegalP0Skill(const FGameplayTag AbilityTag) const
{
	static const TSet<FGameplayTag> P0Skills = {
		FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.FireSlash"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.ThunderDrop"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.VectorHook"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Skill.CounterProofWall"), false)};
	return AbilityTag.IsValid() && P0Skills.Contains(AbilityTag);
}

EPRWardenArenaLane UPRWardenBossComponent::GetRelativeLane(const AActor* Player) const
{
	const AActor* Boss = GetBoss();
	if (!Boss || !Player) return EPRWardenArenaLane::None;
	const float DeltaX = Player->GetActorLocation().X - Boss->GetActorLocation().X;
	return DeltaX < -100.0f ? EPRWardenArenaLane::Left : DeltaX > 100.0f ? EPRWardenArenaLane::Right : EPRWardenArenaLane::Center;
}

void UPRWardenBossComponent::HandleCombatEvent(const FPRCombatEvent& Event)
{
	APREnemyCharacter* Boss = GetBoss();
	if (!Boss || !Event.EventId.IsValid()) return;
	const bool bSkill = IsLegalP0Skill(Event.AbilityTag);
	const bool bPlayerSkill = bSkill && Event.Instigator.Get() == GetPlayerPawn();
	if (bPlayerSkill)
	{
		if (RuntimeState.Phase == EPRWardenBossPhase::PredictiveAttack && Event.AbilityTag != RuntimeState.PredictedSkillTag)
		{
			RuntimeState.bPredictiveAttackCountered = true;
		}
		if (RuntimeState.Phase == EPRWardenBossPhase::PredictiveAttack
			&& Event.AbilityTag == FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"), false))
		{
			RuntimeState.bPredictiveAttackCountered = true;
		}
		if (Event.Target.Get() == Boss && RuntimeState.Phase == EPRWardenBossPhase::RiskMark && !RiskCounterSkills.Contains(Event.AbilityTag) && RuntimeState.RiskLayers > 0)
		{
			RiskCounterSkills.Add(Event.AbilityTag);
			--RuntimeState.RiskLayers;
		}
	}
	if (Event.Target.Get() == Boss && bPlayerSkill) RuntimeState.PredictedSkillTag = Event.AbilityTag;
	if (Event.Target.Get() == Boss) EvaluatePhases();
	if (Event.Target.Get() == Boss && Event.bFatal) PublishCompletion();
}

void UPRWardenBossComponent::EvaluatePhases()
{
	const APREnemyCharacter* Boss = GetBoss();
	const UPRAttributeSet* Attributes = Boss ? Boss->GetAttributeSet() : nullptr;
	if (!Attributes || Attributes->GetMaxHealth() <= UE_SMALL_NUMBER || RuntimeState.Phase == EPRWardenBossPhase::Defeated) return;
	const float Ratio = Attributes->GetHealth() / Attributes->GetMaxHealth();
	if (Ratio <= 0.25f && RuntimeState.Phase < EPRWardenBossPhase::RiskMark) { EnterRiskMark(); return; }
	if (Ratio <= 0.50f && RuntimeState.Phase < EPRWardenBossPhase::PlatformLockdown) { EnterPlatformLockdown(); return; }
	if (Ratio <= 0.75f && RuntimeState.Phase < EPRWardenBossPhase::PredictiveAttack) EnterPredictiveAttack();
}

void UPRWardenBossComponent::EnterPredictiveAttack()
{
	RuntimeState.Phase = EPRWardenBossPhase::PredictiveAttack;
	if (!RuntimeState.PredictedSkillTag.IsValid() || !GetWorld()) { RuntimeState.bDegradedNoOp = true; return; }
	GetWorld()->GetTimerManager().SetTimer(PredictiveTimer, this, &UPRWardenBossComponent::ResolvePredictiveAttack, 1.0f, false);
}

void UPRWardenBossComponent::ResolvePredictiveAttack()
{
	if (!RuntimeState.bPredictiveAttackCountered) ApplyPlayerDamage(10.0f, TEXT("Warden.PredictiveAttack"));
}

void UPRWardenBossComponent::EnterPlatformLockdown()
{
	RuntimeState.Phase = EPRWardenBossPhase::PlatformLockdown;
	RuntimeState.LockedLane = GetRelativeLane(GetPlayerPawn());
	if (RuntimeState.LockedLane == EPRWardenArenaLane::None || !GetWorld()) { RuntimeState.bDegradedNoOp = true; return; }
	GetWorld()->GetTimerManager().SetTimer(LockdownWarningTimer, this, &UPRWardenBossComponent::BeginPlatformLockdownWindow, 1.25f, false);
}

void UPRWardenBossComponent::BeginPlatformLockdownWindow()
{
	if (GetWorld()) GetWorld()->GetTimerManager().SetTimer(LockdownTimer, this, &UPRWardenBossComponent::ResolvePlatformLockdown, 3.0f, false);
}

void UPRWardenBossComponent::ResolvePlatformLockdown()
{
	RuntimeState.bPlatformLockdownCountered = GetRelativeLane(GetPlayerPawn()) != RuntimeState.LockedLane;
	if (!RuntimeState.bPlatformLockdownCountered) ApplyPlayerDamage(10.0f, TEXT("Warden.PlatformLockdown"));
}

void UPRWardenBossComponent::EnterRiskMark()
{
	RuntimeState.Phase = EPRWardenBossPhase::RiskMark;
	RuntimeState.RiskLayers = FMath::Clamp(1 + RuntimeState.RiskPressure, 1, 4);
	RiskCounterSkills.Reset();
	if (!GetWorld()) { RuntimeState.bDegradedNoOp = true; return; }
	GetWorld()->GetTimerManager().SetTimer(RiskMarkTimer, this, &UPRWardenBossComponent::ResolveRiskMark, 4.0f, false);
}

void UPRWardenBossComponent::ResolveRiskMark()
{
	RuntimeState.bRiskMarkCountered = RuntimeState.RiskLayers == 0;
	if (RuntimeState.RiskLayers > 0) ApplyPlayerDamage(5.0f * RuntimeState.RiskLayers, TEXT("Warden.RiskMark"));
}

void UPRWardenBossComponent::ApplyPlayerDamage(const float Damage, const FName SourceId)
{
	UPRCombatSubsystem* Combat = BoundCombat.Get();
	APawn* Player = GetPlayerPawn();
	APREnemyCharacter* Boss = GetBoss();
	if (!Combat || !Player || !Boss) { RuntimeState.bDegradedNoOp = true; return; }
	FPRDamageRequest Request;
	Request.SourceId = SourceId;
	Request.DamageSource = this;
	Request.Instigator = Boss;
	Request.Target = Player;
	Request.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.AuditorCounter"), false);
	Request.RawDamage = Damage;
	Request.ImpactOrigin = Boss->GetActorLocation();
	Request.IncomingDirection = (Player->GetActorLocation() - Boss->GetActorLocation()).GetSafeNormal();
	Combat->ApplyDamage(Request);
}

void UPRWardenBossComponent::PublishCompletion()
{
	if (bCompletionPublished) return;
	bCompletionPublished = true;
	RuntimeState.Phase = EPRWardenBossPhase::Defeated;
	FPRPrototypeRunResult Result;
	Result.CompletionId = FGuid::NewGuid();
	Result.BossId = UPRChapterContentRegistryDataAsset::GetWardenBossId();
	if (const APREnemyCharacter* Boss = GetBoss()) Result.BossSpawnId = Boss->GetSpawnId();
	Result.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (UWorld* World = GetWorld()) if (UPRBossSubsystem* BossSubsystem = World->GetSubsystem<UPRBossSubsystem>()) BossSubsystem->PublishPrototypeRunCompleted(Result);
}
