// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Headmind/PRHeadmindProjectionBossComponent.h"

#include "Chapters/Headmind/PRHeadmindProjectionBoss.h"
#include "Chapters/PRChapterSubsystem.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Companions/PRCompanionSubsystem.h"
#include "Enemies/Bosses/PRAuditorBossComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Roguelike/Progression/PRProgressionSubsystem.h"
#include "TimerManager.h"

UPRHeadmindProjectionBossComponent::UPRHeadmindProjectionBossComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UPRHeadmindProjectionBossComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UPRCombatSubsystem* Combat = World->GetSubsystem<UPRCombatSubsystem>())
		{
			BoundCombat = Combat;
			CombatHandle = Combat->OnCombatEvent().AddUObject(this, &UPRHeadmindProjectionBossComponent::HandleCombatEvent);
		}
	}
}

void UPRHeadmindProjectionBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTransientState();
	if (UPRCombatSubsystem* Combat = BoundCombat.Get()) Combat->OnCombatEvent().Remove(CombatHandle);
	CombatHandle.Reset(); BoundCombat.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPRHeadmindProjectionBossComponent::ConfigureChapterState(const int32 InSynthesisPressure, const FGameplayTag InPrimaryRuleId, const FGameplayTag InSecondaryRuleId, const bool bFusionAvailable)
{
	RuntimeState.SynthesisPressure = FMath::Clamp(InSynthesisPressure, 0, 4);
	RuntimeState.PrimaryRuleId = InPrimaryRuleId;
	RuntimeState.SecondaryRuleId = InSecondaryRuleId;
	RuntimeState.bDirectiveFusionAvailable = bFusionAvailable;
	RuntimeState.bDegradedNoOp = !bFusionAvailable;
	if (APRHeadmindProjectionBoss* Boss = GetBoss())
	{
		Boss->SetHeadmindPresentation(
			bFusionAvailable ? FText::FromString(FString::Printf(TEXT("DIRECTIVE FUSION: %s + %s"), *InPrimaryRuleId.ToString(), *InSecondaryRuleId.ToString())) : FText::FromString(TEXT("HEADMIND DIRECTIVE FUSION UNAVAILABLE")),
			bFusionAvailable ? FColor::Yellow : FColor::Silver, true);
	}
	PublishRuntimeState();
}

const FPRHeadmindBossRuntimeState& UPRHeadmindProjectionBossComponent::GetRuntimeState() const { return RuntimeState; }
APRHeadmindProjectionBoss* UPRHeadmindProjectionBossComponent::GetBoss() const { return Cast<APRHeadmindProjectionBoss>(GetOwner()); }
APawn* UPRHeadmindProjectionBossComponent::GetPlayerPawn() const { return UGameplayStatics::GetPlayerPawn(this, 0); }

void UPRHeadmindProjectionBossComponent::HandleCombatEvent(const FPRCombatEvent& Event)
{
	if (Event.Target.Get() != GetBoss()) return;
	EvaluateBasePhase();
}

void UPRHeadmindProjectionBossComponent::EvaluateBasePhase()
{
	APRHeadmindProjectionBoss* Boss = GetBoss();
	UPRAuditorBossComponent* Base = Boss ? Boss->GetAuditorBossComponent() : nullptr;
	if (!Base) { RuntimeState.bDegradedNoOp = true; PublishRuntimeState(); return; }
	if (!bBasiliskStarted && Base->GetPhase() >= EPRAuditorBossPhase::PredictionShield) StartBasiliskJudgment();
	if (Base->GetPhase() == EPRAuditorBossPhase::Defeated) { RuntimeState.Phase = EPRHeadmindBossPhase::Defeated; ClearTransientState(); }
	PublishRuntimeState();
}

void UPRHeadmindProjectionBossComponent::StartBasiliskJudgment()
{
	bBasiliskStarted = true;
	RuntimeState.Phase = EPRHeadmindBossPhase::BasiliskJudgment;
	FreezeOpportunity();
	if (APRHeadmindProjectionBoss* Boss = GetBoss()) Boss->SetHeadmindPresentation(FText::FromString(TEXT("BASILISK JUDGMENT: TRIPLE RESONANCE DEFERRED")), FColor::Cyan, true);
	if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(BasiliskTimer, this, &UPRHeadmindProjectionBossComponent::EndBasiliskJudgment, 5.0f, false);
	else RuntimeState.bDegradedNoOp = true;
	PublishRuntimeState();
}

void UPRHeadmindProjectionBossComponent::EndBasiliskJudgment()
{
	RuntimeState.TripleResonance.bWindowActive = false;
	if (RuntimeState.Phase != EPRHeadmindBossPhase::Defeated) RuntimeState.Phase = EPRHeadmindBossPhase::DirectiveFusion;
	PublishRuntimeState();
}

void UPRHeadmindProjectionBossComponent::FreezeOpportunity()
{
	FPRTripleResonanceOpportunitySnapshot Opportunity;
	if (!ReadTripleEligibility(Opportunity))
	{
		Opportunity.State = EPRTripleResonanceOpportunityState::DegradedNoOp;
		Opportunity.FallbackReason = TEXT("Headmind.TripleResonanceUnavailable");
		RuntimeState.bDegradedNoOp = true;
	}
	Opportunity.bWindowActive = true;
	if (const APRHeadmindProjectionBoss* Boss = GetBoss())
	{
		if (const UPRAuditorBossComponent* Base = Boss->GetAuditorBossComponent()) Opportunity.PredictedSkillTag = Base->GetPredictedAbilityTag();
	}
	RuntimeState.TripleResonance = Opportunity;
	PublishRuntimeState();
}

bool UPRHeadmindProjectionBossComponent::ReadTripleEligibility(FPRTripleResonanceOpportunitySnapshot& OutSnapshot) const
{
	OutSnapshot = FPRTripleResonanceOpportunitySnapshot();
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UPRChapterSubsystem* Chapters = GameInstance ? GameInstance->GetSubsystem<UPRChapterSubsystem>() : nullptr;
	UPRProgressionSubsystem* Progression = GameInstance ? GameInstance->GetSubsystem<UPRProgressionSubsystem>() : nullptr;
	UPRCompanionSubsystem* Companions = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
	FPRChapterSnapshot ChapterSnapshot; FPRProgressionSnapshot ProgressionSnapshot;
	if (!Chapters || !Progression || !Companions || !Chapters->GetSnapshot(ChapterSnapshot) || !Progression->GetProgressionSnapshot(ProgressionSnapshot)) return false;
	const FPrimaryAssetId TripleNode(TEXT("ProgressionNode"), TEXT("BondTripleResonance"));
	if (!ChapterSnapshot.bHasTripleResonancePrerequisite || !ProgressionSnapshot.UnlockedNodeIds.Contains(TripleNode)) return true;
	for (const FGameplayTag& CompanionId : FPRCompanionContract::GetCanonicalCompanionIds())
	{
		FPRCompanionRelationshipRecord Record;
		if (!Companions->GetRelationshipSnapshot(CompanionId, Record)) return false;
		if (Record.State.Trust < 70 || Record.State.Overload != 0) return true;
	}
	OutSnapshot.State = EPRTripleResonanceOpportunityState::EligibleDeferredToV072;
	return true;
}

void UPRHeadmindProjectionBossComponent::ClearTransientState()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(BasiliskTimer);
	RuntimeState.TripleResonance.bWindowActive = false;
	PublishRuntimeState();
}

void UPRHeadmindProjectionBossComponent::PublishRuntimeState() const
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UPRChapterSubsystem* Chapters = GameInstance->GetSubsystem<UPRChapterSubsystem>())
		{
			Chapters->PublishHeadmindBossRuntimeState(RuntimeState);
		}
	}
}
