// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Auditor/PRAuditorChapterBossComponent.h"

#include "Chapters/Auditor/PRAuditorChapterBoss.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Director/PRPlayerProfileSubsystem.h"
#include "Director/PRPlayerProfileTypes.h"
#include "Engine/GameInstance.h"
#include "Enemies/Bosses/PRAuditorBossComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UPRAuditorChapterBossComponent::UPRAuditorChapterBossComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPRAuditorChapterBossComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>())
		{
			BoundCombat = Combat;
			CombatHandle = Combat->OnCombatEvent().AddUObject(this, &UPRAuditorChapterBossComponent::HandleCombatEvent);
		}
	}
	CaptureHabitProjection();
}

void UPRAuditorChapterBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTransientState();
	if (UPRCombatSubsystem* Combat = BoundCombat.Get()) Combat->OnCombatEvent().Remove(CombatHandle);
	CombatHandle.Reset();
	BoundCombat.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRAuditorChapterBossComponent::ConfigureChapterState(const int32 AuditPressure)
{
	RuntimeState.AuditPressure = FMath::Clamp(AuditPressure, 0, 4);
}

const FPRAuditorChapterBossRuntimeState& UPRAuditorChapterBossComponent::GetRuntimeState() const { return RuntimeState; }
APRAuditorChapterBoss* UPRAuditorChapterBossComponent::GetBoss() const { return Cast<APRAuditorChapterBoss>(GetOwner()); }
APawn* UPRAuditorChapterBossComponent::GetPlayerPawn() const { return UGameplayStatics::GetPlayerPawn(this, 0); }

bool UPRAuditorChapterBossComponent::IsLegalP0Skill(const FGameplayTag AbilityTag) const
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

void UPRAuditorChapterBossComponent::CaptureHabitProjection()
{
	RuntimeState.HabitProjection = FPRAuditorHabitProjection();
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UPRPlayerProfileSubsystem* Profile = GameInstance ? GameInstance->GetSubsystem<UPRPlayerProfileSubsystem>() : nullptr;
	FPRPlayerProfileSnapshot Snapshot;
	if (!Profile || !Profile->GetSnapshot(Snapshot))
	{
		RuntimeState.HabitProjection.FallbackReason = TEXT("Auditor.HabitProjectionUnavailable");
		return;
	}
	int32 BestCount = 0;
	for (const FPRPlayerProfileSkillMetric& Metric : Snapshot.SkillMetrics)
	{
		if (!IsLegalP0Skill(Metric.SkillTag)) continue;
		if (Metric.UseCount > BestCount || (Metric.UseCount == BestCount && Metric.UseCount > 0 && (!RuntimeState.HabitProjection.DominantSkillTag.IsValid() || Metric.SkillTag.ToString() < RuntimeState.HabitProjection.DominantSkillTag.ToString())))
		{
			BestCount = Metric.UseCount;
			RuntimeState.HabitProjection.DominantSkillTag = Metric.SkillTag;
		}
	}
	const float Distance = Snapshot.CombatDistance.AverageDistanceCm;
	RuntimeState.HabitProjection.DistanceBand = Distance <= 350.0f ? EPRAuditorDistanceBand::Near : Distance <= 700.0f ? EPRAuditorDistanceBand::Mid : EPRAuditorDistanceBand::Far;
	const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.AfterimageDodge"), false);
	const FPRPlayerProfileSkillMetric* Dodge = Snapshot.SkillMetrics.FindByPredicate([DodgeTag](const FPRPlayerProfileSkillMetric& Metric) { return Metric.SkillTag == DodgeTag; });
	RuntimeState.HabitProjection.bDodgeHeavy = Dodge && Dodge->UseCount >= 3;
	RuntimeState.HabitProjection.bAvailable = RuntimeState.HabitProjection.DominantSkillTag.IsValid();
	if (!RuntimeState.HabitProjection.bAvailable) RuntimeState.HabitProjection.FallbackReason = TEXT("Auditor.HabitProjectionUnavailable");
}

void UPRAuditorChapterBossComponent::HandleCombatEvent(const FPRCombatEvent& Event)
{
	APRAuditorChapterBoss* Boss = GetBoss();
	if (!Boss || !Event.EventId.IsValid()) return;
	if (Event.Target.Get() == Boss && Event.Instigator.Get() == GetPlayerPawn() && IsLegalP0Skill(Event.AbilityTag))
	{
		if (RuntimeState.Phase == EPRAuditorChapterBossPhase::RepeatedBuildAudit && !RepeatedBuildSkills.Contains(Event.AbilityTag))
		{
			RepeatedBuildSkills.Add(Event.AbilityTag);
			RuntimeState.RemainingAuditUnits = FMath::Max(0, RuntimeState.RemainingAuditUnits - 1);
			if (RuntimeState.RemainingAuditUnits == 0)
			{
				RuntimeState.bRepeatedBuildCountered = true;
				if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(RepeatedBuildTimer);
				ShowMechanic(TEXT("REPEATED BUILD AUDIT COUNTERED"), FColor::Green);
			}
		}
		else if (RuntimeState.Phase == EPRAuditorChapterBossPhase::VerdictEscalation && !VerdictSkills.Contains(Event.AbilityTag))
		{
			VerdictSkills.Add(Event.AbilityTag);
			RuntimeState.RemainingVerdictSkills = FMath::Max(0, RuntimeState.RemainingVerdictSkills - 1);
			if (RuntimeState.RemainingVerdictSkills == 0)
			{
				RuntimeState.bVerdictCountered = true;
				if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(VerdictTimer);
				ShowMechanic(TEXT("VERDICT ESCALATION COUNTERED"), FColor::Green);
			}
		}
	}
	if (Event.Target.Get() == Boss) EvaluateBasePhase();
}

void UPRAuditorChapterBossComponent::EvaluateBasePhase()
{
	APRAuditorChapterBoss* Boss = GetBoss();
	UPRAuditorBossComponent* Base = Boss ? Boss->GetAuditorBossComponent() : nullptr;
	if (!Base)
	{
		MarkDegradedNoOp();
		return;
	}
	if (!bRepeatedBuildStarted && Base->GetPhase() >= EPRAuditorBossPhase::RuleAudit) StartRepeatedBuildAudit();
	if (!bVerdictStarted && Base->GetPhase() >= EPRAuditorBossPhase::PredictionShield) StartVerdictEscalation();
	if (Base->GetPhase() == EPRAuditorBossPhase::Defeated)
	{
		RuntimeState.Phase = EPRAuditorChapterBossPhase::Defeated;
		ClearTransientState();
	}
}

void UPRAuditorChapterBossComponent::StartRepeatedBuildAudit()
{
	bRepeatedBuildStarted = true;
	RuntimeState.Phase = EPRAuditorChapterBossPhase::RepeatedBuildAudit;
	RuntimeState.RemainingAuditUnits = FMath::Clamp(1 + RuntimeState.AuditPressure, 1, 4);
	RepeatedBuildSkills.Reset();
	if (!GetWorld() || !BoundCombat.IsValid()) { MarkDegradedNoOp(); return; }
	ShowMechanic(TEXT("REPEATED BUILD AUDIT: USE DISTINCT SKILLS"), FColor::Yellow);
	GetWorld()->GetTimerManager().SetTimer(RepeatedBuildTimer, this, &UPRAuditorChapterBossComponent::ResolveRepeatedBuildAudit, 5.0f, false);
}

void UPRAuditorChapterBossComponent::ResolveRepeatedBuildAudit()
{
	if (!RuntimeState.bRepeatedBuildCountered)
	{
		ShowMechanic(TEXT("REPEATED BUILD AUDIT FAILED"), FColor::Red);
		ApplyPlayerDamage(5.0f * RuntimeState.RemainingAuditUnits, TEXT("Auditor.RepeatedBuildAudit"));
	}
}

void UPRAuditorChapterBossComponent::StartVerdictEscalation()
{
	bVerdictStarted = true;
	RuntimeState.Phase = EPRAuditorChapterBossPhase::VerdictEscalation;
	RuntimeState.RemainingVerdictSkills = FMath::Clamp(1 + RuntimeState.AuditPressure / 2, 1, 3) + 1;
	VerdictSkills.Reset();
	if (!GetWorld() || !BoundCombat.IsValid()) { MarkDegradedNoOp(); return; }
	ShowMechanic(TEXT("VERDICT ESCALATION: BREAK THE PREDICTION"), FColor::Yellow);
	GetWorld()->GetTimerManager().SetTimer(VerdictTimer, this, &UPRAuditorChapterBossComponent::ResolveVerdictEscalation, 6.0f, false);
}

void UPRAuditorChapterBossComponent::ResolveVerdictEscalation()
{
	if (!RuntimeState.bVerdictCountered)
	{
		ShowMechanic(TEXT("VERDICT ESCALATION FAILED"), FColor::Red);
		ApplyPlayerDamage(5.0f * RuntimeState.RemainingVerdictSkills, TEXT("Auditor.VerdictEscalation"));
	}
}

void UPRAuditorChapterBossComponent::ApplyPlayerDamage(const float Damage, const FName SourceId)
{
	UPRCombatSubsystem* Combat = BoundCombat.Get();
	APawn* Player = GetPlayerPawn();
	APRAuditorChapterBoss* Boss = GetBoss();
	if (!Combat || !Player || !Boss) { MarkDegradedNoOp(); return; }
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

void UPRAuditorChapterBossComponent::ClearTransientState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RepeatedBuildTimer);
		World->GetTimerManager().ClearTimer(VerdictTimer);
	}
	RepeatedBuildSkills.Reset();
	VerdictSkills.Reset();
}

void UPRAuditorChapterBossComponent::ShowMechanic(const TCHAR* Text, const FColor& Color)
{
	if (APRAuditorChapterBoss* Boss = GetBoss()) Boss->SetChapterMechanicPresentation(FText::FromString(Text), Color, true);
}

void UPRAuditorChapterBossComponent::MarkDegradedNoOp()
{
	RuntimeState.bDegradedNoOp = true;
	ShowMechanic(TEXT("AUDITOR CHAPTER MECHANIC DEGRADED (NO-OP)"), FColor::Silver);
}
