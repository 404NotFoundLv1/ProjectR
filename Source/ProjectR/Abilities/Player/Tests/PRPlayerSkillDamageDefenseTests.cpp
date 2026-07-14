// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "Core/PRTagLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"

namespace PRPlayerSkillDefenseAutomation
{
class FScopedWorld
{
public:
	FScopedWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
	}

	~FScopedWorld()
	{
		if (World)
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	}

	UWorld* Get() const { return World; }

private:
	TObjectPtr<UWorld> World;
};

template <typename T>
T* SpawnBlueprintActor(UWorld* World, const TCHAR* ClassPath)
{
	UClass* ActorClass = LoadClass<T>(nullptr, ClassPath);
	return ActorClass ? World->SpawnActor<T>(ActorClass) : nullptr;
}
} // namespace PRPlayerSkillDefenseAutomation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerSkillDamageDefenseTest,
	"ProjectR.PlayerSkill.Runtime.DamageBurningAndDefense",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRPlayerSkillDamageDefenseTest::RunTest(const FString& Parameters)
{
	using namespace PRPlayerSkillDefenseAutomation;
	AddExpectedError(TEXT("No GameplayCueNotifyPaths were specified"),
		EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("Combat clamped damage from 500000 to 200000"),
		EAutomationExpectedErrorFlags::Contains, 1);
	const FPRDamageRequest Defaults;
	TestTrue(TEXT("Impact origin defaults zero"), Defaults.ImpactOrigin.IsZero());
	TestTrue(TEXT("Incoming direction defaults zero"), Defaults.IncomingDirection.IsZero());
	TestEqual(TEXT("Blocked status is appended"),
		static_cast<uint8>(EPRCombatRequestStatus::RejectedBlocked), static_cast<uint8>(5));

	FScopedWorld World;
	APRPlayerState* PlayerState = SpawnBlueprintActor<APRPlayerState>(
		World.Get(), TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerState.BP_PlayerState_C"));
	APRPlayerController* Controller = SpawnBlueprintActor<APRPlayerController>(
		World.Get(), TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerController.BP_PlayerController_C"));
	APRPlayerSkillMitigationTestCharacter* Character =
		World.Get()->SpawnActor<APRPlayerSkillMitigationTestCharacter>();
	if (!TestNotNull(TEXT("PlayerState spawned"), PlayerState)
		|| !TestNotNull(TEXT("Controller spawned"), Controller)
		|| !TestNotNull(TEXT("Mitigation character spawned"), Character))
	{
		return false;
	}
	Controller->SetPlayerState(PlayerState);
	Controller->Possess(Character);
	UPRAbilitySystemComponent* ASC = PlayerState->GetProjectRAbilitySystemComponent();
	const UPRAttributeSet* Attributes = PlayerState->GetAttributeSet();
	UPRCombatSubsystem* Combat = World.Get()->GetSubsystem<UPRCombatSubsystem>();
	if (!TestNotNull(TEXT("ASC exists"), ASC)
		|| !TestNotNull(TEXT("Attributes exist"), Attributes)
		|| !TestNotNull(TEXT("Combat subsystem exists"), Combat))
	{
		return false;
	}

	TArray<FPRCombatEvent> Events;
	const FDelegateHandle EventHandle = Combat->OnCombatEvent().AddLambda(
		[&Events](const FPRCombatEvent& Event)
		{
			Events.Add(Event);
		});
	FPRDamageRequest Request;
	Request.SourceId = TEXT("Automation.PlayerSkill");
	Request.DamageSource = PlayerState;
	Request.Instigator = Character;
	Request.Target = Character;
	Request.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.CounterProofWall"));
	Request.RawDamage = 500000.0f;
	Request.ImpactOrigin = Character->GetActorLocation() - FVector(100.0f, 0.0f, 0.0f);
	Request.IncomingDirection = FVector(10.0f, 0.0f, 0.0f);

	FGameplayTagContainer GuardTags;
	GuardTags.AddTag(UPRTagLibrary::GetCombatResponseGuardBlockedTag());
	Character->ConfigureMitigation(EPRCombatMitigationResult::Blocked, GuardTags);
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateInvulnerableTag(), 1, EGameplayTagReplicationState::TagOnly);
	const float InitialHealth = Attributes->GetHealth();
	const float InitialShield = Attributes->GetShield();
	const int32 InitialEffects = ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
	TestEqual(TEXT("Mitigation runs before Invulnerable"),
		Combat->ApplyDamage(Request), EPRCombatRequestStatus::RejectedBlocked);
	TestEqual(TEXT("Mitigation evaluated once"), Character->GetMitigationEvaluationCount(), 1);
	TestTrue(TEXT("IncomingDirection normalized before mitigation"),
		Character->GetLastIncomingDirection().Equals(FVector::ForwardVector));
	TestEqual(TEXT("Blocked preserves Health"), Attributes->GetHealth(), InitialHealth);
	TestEqual(TEXT("Blocked preserves Shield"), Attributes->GetShield(), InitialShield);
	TestEqual(TEXT("Blocked adds no ActiveEffect"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), InitialEffects);
	TestEqual(TEXT("Blocked broadcasts exactly once"), Events.Num(), 1);
	if (Events.Num() == 1)
	{
		const FPRCombatEvent& Blocked = Events[0];
		TestEqual(TEXT("Blocked event tag"), Blocked.EventTag, UPRTagLibrary::GetCombatEventDamageRejectedTag());
		TestEqual(TEXT("Blocked damage clamps"), Blocked.RawDamage, 200000.0f);
		TestEqual(TEXT("Blocked HealthDamage zero"), Blocked.HealthDamage, 0.0f);
		TestEqual(TEXT("Blocked ShieldAbsorbed zero"), Blocked.ShieldAbsorbed, 0.0f);
		TestEqual(TEXT("Blocked RemainingHealth"), Blocked.RemainingHealth, InitialHealth);
		TestEqual(TEXT("Blocked RemainingShield"), Blocked.RemainingShield, InitialShield);
		TestFalse(TEXT("Blocked is not fatal"), Blocked.bFatal);
		TestEqual(TEXT("Blocked response count"), Blocked.ResponseTags.Num(), 1);
		TestTrue(TEXT("Blocked response tag"), Blocked.ResponseTags.HasTagExact(
			UPRTagLibrary::GetCombatResponseGuardBlockedTag()));
	}
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateInvulnerableTag(), 0, EGameplayTagReplicationState::TagOnly);
	Request.RawDamage = 10.0f;

	FGameplayTagContainer InvalidMitigationTags;
	InvalidMitigationTags.AddTag(UPRTagLibrary::GetStateGuardingTag());
	Character->ConfigureMitigation(EPRCombatMitigationResult::Blocked, InvalidMitigationTags);
	AddExpectedError(TEXT("returned Blocked without valid Combat.Response tags"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestEqual(TEXT("Blocked without response tag is invalid"),
		Combat->ApplyDamage(Request), EPRCombatRequestStatus::Invalid);
	TestEqual(TEXT("Invalid Blocked emits no event"), Events.Num(), 1);

	Character->ConfigureMitigation(EPRCombatMitigationResult::Blocked, GuardTags);
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateDeadTag(), 1, EGameplayTagReplicationState::TagOnly);
	const int32 EvaluationsBeforeDead = Character->GetMitigationEvaluationCount();
	TestEqual(TEXT("Dead rejection precedes mitigation"),
		Combat->ApplyDamage(Request), EPRCombatRequestStatus::RejectedDead);
	TestEqual(TEXT("Dead does not evaluate mitigation"),
		Character->GetMitigationEvaluationCount(), EvaluationsBeforeDead);
	TestEqual(TEXT("Dead rejection emits once"), Events.Num(), 2);
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateDeadTag(), 0, EGameplayTagReplicationState::TagOnly);

	Character->ConfigureMitigation(EPRCombatMitigationResult::NotHandled, FGameplayTagContainer());
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateInvulnerableTag(), 1, EGameplayTagReplicationState::TagOnly);
	TestEqual(TEXT("NotHandled falls through to Invulnerable"),
		Combat->ApplyDamage(Request), EPRCombatRequestStatus::RejectedInvulnerable);
	TestEqual(TEXT("Invulnerable rejection emits once"), Events.Num(), 3);
	ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateInvulnerableTag(), 0, EGameplayTagReplicationState::TagOnly);

	Request.IncomingDirection = FVector::ZeroVector;
	AddExpectedError(TEXT("zero IncomingDirection"), EAutomationExpectedErrorFlags::Contains, 1);
	TestEqual(TEXT("Skill damage requires direction"),
		Combat->ApplyDamage(Request), EPRCombatRequestStatus::Invalid);
	TestEqual(TEXT("Invalid direction emits no event"), Events.Num(), 3);

	APRPlayerSkillTestTarget* Subject = World.Get()->SpawnActor<APRPlayerSkillTestTarget>();
	Subject->ConfigureTarget(TEXT("Outcome.Target"), true);
	FPRCombatOutcomeRequest Outcome;
	Outcome.SourceId = TEXT("Automation.PlayerSkill");
	Outcome.AbilitySource = PlayerState;
	Outcome.Instigator = Character;
	Outcome.Target = Subject;
	Outcome.AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"));
	Outcome.ResponseTags.AddTag(UPRTagLibrary::GetCombatResponseDisplacementAppliedTag());
	TestTrue(TEXT("Valid AbilityOutcome publishes"), Combat->PublishAbilityOutcome(Outcome));
	TestEqual(TEXT("AbilityOutcome emits once"), Events.Num(), 4);
	if (Events.Num() == 4)
	{
		const FPRCombatEvent& Event = Events.Last();
		TestEqual(TEXT("AbilityOutcome event tag"), Event.EventTag, UPRTagLibrary::GetCombatEventAbilityOutcomeTag());
		TestEqual(TEXT("AbilityOutcome target ID uses subject fallback"), Event.TargetId, FName(TEXT("Outcome.Target")));
		TestEqual(TEXT("AbilityOutcome raw zero"), Event.RawDamage, 0.0f);
		TestEqual(TEXT("AbilityOutcome shield zero"), Event.ShieldAbsorbed, 0.0f);
		TestEqual(TEXT("AbilityOutcome health zero"), Event.HealthDamage, 0.0f);
		TestEqual(TEXT("AbilityOutcome remaining Health zero"), Event.RemainingHealth, 0.0f);
		TestEqual(TEXT("AbilityOutcome remaining Shield zero"), Event.RemainingShield, 0.0f);
		TestFalse(TEXT("AbilityOutcome not fatal"), Event.bFatal);
		TestTrue(TEXT("AbilityOutcome response preserved"), Event.ResponseTags.HasTagExact(
			UPRTagLibrary::GetCombatResponseDisplacementAppliedTag()));
	}
	Outcome.ResponseTags.Reset();
	Outcome.ResponseTags.AddTag(UPRTagLibrary::GetStateGuardingTag());
	AddExpectedError(TEXT("Combat rejected an invalid AbilityOutcome request"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Non-response AbilityOutcome is rejected"), Combat->PublishAbilityOutcome(Outcome));
	TestEqual(TEXT("Invalid AbilityOutcome emits no event"), Events.Num(), 4);

	Combat->OnCombatEvent().Remove(EventHandle);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
