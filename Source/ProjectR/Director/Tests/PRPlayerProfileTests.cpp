// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilityTypes.h"
#include "Combat/PRCombatTypes.h"
#include "Director/PRPlayerProfileSubsystem.h"
#include "Divergence/PRDivergenceTypes.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "QTE/PRQTETypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerProfileContractTest,
	"ProjectR.Director.Profile.ValueSnapshotAndSessionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRPlayerProfileContractTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UPRPlayerProfileSubsystem* Profile = NewObject<UPRPlayerProfileSubsystem>(GameInstance);
	FPRPlayerProfileSnapshot Snapshot;
	TestFalse(TEXT("A profile has no snapshot before a successful session begins"), Profile->GetSnapshot(Snapshot));
	Profile->BeginProfileSessionForAutomation();
	TestTrue(TEXT("A new session exposes a value-only snapshot"), Profile->GetSnapshot(Snapshot));
	TestEqual(TEXT("Snapshot schema remains frozen at one"), Snapshot.SchemaVersion, 1);
	TestTrue(TEXT("Profile session id is generated once per session"), Snapshot.ProfileSessionId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerProfileAbilitySamplingTest,
	"ProjectR.Director.Profile.AbilityLifecycleSampling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRPlayerProfileAbilitySamplingTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UPRPlayerProfileSubsystem* Profile = NewObject<UPRPlayerProfileSubsystem>(GameInstance);
	Profile->BeginProfileSessionForAutomation();

	FPRAbilityLifecycleEvent Event;
	Event.EventType = EPRAbilityLifecycleEventType::Committed;
	Event.AbilityState.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
	Profile->InjectAbilityLifecycleForAutomation(Event);

	FPRPlayerProfileSnapshot Snapshot;
	TestTrue(TEXT("A committed PlayerSkill is retained as a bounded value-only sample"), Profile->GetSnapshot(Snapshot));
	TestEqual(TEXT("The committed skill creates exactly one metric"), Snapshot.SkillMetrics.Num(), 1);
	TestEqual(TEXT("The committed skill increments its CommitCount"), Snapshot.SkillMetrics[0].CommitCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerProfileStableEventSamplingTest,
	"ProjectR.Director.Profile.StableEventSamplingAndDeduplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPRPlayerProfileStableEventSamplingTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UPRPlayerProfileSubsystem* Profile = NewObject<UPRPlayerProfileSubsystem>(GameInstance);
	Profile->BeginProfileSessionForAutomation();

	FPRCombatEvent Combat;
	Combat.EventId = FGuid::NewGuid();
	Combat.SourceId = TEXT("Player");
	Combat.TargetId = TEXT("Enemy.Test");
	Combat.HealthDamage = 12.0f;
	Profile->InjectCombatEventForAutomation(Combat);
	Profile->InjectCombatEventForAutomation(Combat);

	FPRQTEResult QTE;
	QTE.ResultId = FGuid::NewGuid();
	QTE.ResultTag = FGameplayTag::RequestGameplayTag(TEXT("QTE.Result.Success"));
	QTE.TimingGrade = EPRQTETimingGrade::Perfect;
	Profile->InjectQTEResultForAutomation(QTE);

	FPRRelationshipChangedEvent Relationship;
	Relationship.CompanionId = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"));
	Relationship.CurrentState.Trust = 60;
	Relationship.CurrentState.Affection = 55;
	Profile->InjectRelationshipChangedForAutomation(Relationship);

	FPRDivergenceResult Divergence;
	Divergence.ResultId = FGuid::NewGuid();
	Divergence.CompanionId = Relationship.CompanionId;
	Divergence.Choice = EPRDivergenceChoice::Rescue;
	Divergence.Resolution = EPRDivergenceResolution::Applied;
	Divergence.FutureDisposition = EPRDivergenceFutureDisposition::RescueEvacuationRequested;
	Profile->InjectDivergenceResultForAutomation(Divergence);

	FPRPlayerProfileSnapshot Snapshot;
	TestTrue(TEXT("Stable event samples publish a profile snapshot"), Profile->GetSnapshot(Snapshot));
	TestEqual(TEXT("Duplicate combat EventId is consumed once"), Snapshot.Resources.DamageDealt, 12.0f);
	TestEqual(TEXT("A QTE result produces one bounded count"), Snapshot.QTEResultCounts.Num(), 1);
	TestEqual(TEXT("Perfect timing is projected as a stable value"), Snapshot.QTEPerfectTimingCount, 1);
	TestEqual(TEXT("Relationship updates keep one stable companion record"), Snapshot.Relationships.Num(), 1);
	TestEqual(TEXT("Divergence result retains its stable identifier"), Snapshot.LastDivergence.ResultId, Divergence.ResultId);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
