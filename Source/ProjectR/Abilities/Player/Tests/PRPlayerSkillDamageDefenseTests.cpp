// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRGA_FireSlash.h"
#include "Abilities/Player/PRGA_CounterProofWall.h"
#include "Abilities/Player/PRGA_ShadowThrust.h"
#include "Abilities/Player/PRSkillDecoyActor.h"
#include "Abilities/Player/PRPlayerSkillComponent.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillSubsystem.h"
#include "Abilities/Player/Tests/PRPlayerSkillTestTypes.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "Core/PRTagLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

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
	void Tick(const float DeltaSeconds) const
	{
		++GFrameCounter;
		World->Tick(ELevelTick::LEVELTICK_All, DeltaSeconds);
	}
	void TickFor(const float Duration, const float Step = 1.0f / 120.0f) const
	{
		float Elapsed = 0.0f;
		while (Elapsed + UE_SMALL_NUMBER < Duration)
		{
			const float Delta = FMath::Min(Step, Duration - Elapsed);
			Tick(Delta);
			Elapsed += Delta;
		}
	}

private:
	TObjectPtr<UWorld> World;
};

template <typename T>
T* SpawnBlueprintActor(UWorld* World, const TCHAR* ClassPath)
{
	UClass* ActorClass = LoadClass<T>(nullptr, ClassPath);
	return ActorClass ? World->SpawnActor<T>(ActorClass) : nullptr;
}

struct FCombatPlayer
{
	APRPlayerState* PlayerState = nullptr;
	APRPlayerController* Controller = nullptr;
	APRPlayerSkillCombatTestCharacter* Character = nullptr;
	UPRAbilitySystemComponent* ASC = nullptr;
	const UPRAttributeSet* Attributes = nullptr;
};

bool SpawnCombatPlayer(UWorld* World, const FVector Location, const FName TargetId, FCombatPlayer& OutPlayer)
{
	OutPlayer.PlayerState = SpawnBlueprintActor<APRPlayerState>(
		World, TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerState.BP_PlayerState_C"));
	OutPlayer.Controller = SpawnBlueprintActor<APRPlayerController>(
		World, TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerController.BP_PlayerController_C"));
	OutPlayer.Character = World->SpawnActor<APRPlayerSkillCombatTestCharacter>();
	if (!OutPlayer.PlayerState || !OutPlayer.Controller || !OutPlayer.Character)
	{
		return false;
	}
	OutPlayer.Character->SetActorLocation(Location);
	OutPlayer.Character->ConfigureTarget(TargetId, true);
	OutPlayer.Controller->SetPlayerState(OutPlayer.PlayerState);
	OutPlayer.Controller->Possess(OutPlayer.Character);
	OutPlayer.ASC = OutPlayer.PlayerState->GetProjectRAbilitySystemComponent();
	OutPlayer.Attributes = OutPlayer.PlayerState->GetAttributeSet();
	if (!OutPlayer.ASC || !OutPlayer.Attributes)
	{
		return false;
	}
	OutPlayer.Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 100.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxShieldAttribute(), 0.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 0.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxEnergyAttribute(), 100.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), 100.0f);
	OutPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetAttackPowerAttribute(), 10.0f);
	return true;
}

FGameplayAbilitySpecHandle GrantFormalSkill(
	UPRAbilitySystemComponent& ASC,
	const UPRPlayerSkillDataAsset& SkillData)
{
	for (const FGameplayAbilitySpec& ExistingSpec : ASC.GetActivatableAbilities())
	{
		// The formal DefaultAbilitySet is granted by the PlayerState fixture. Its
		// stable SourceObject is the definitive identity; adding a second spec
		// would make input and CombatEvent assertions ambiguous.
		if (ExistingSpec.SourceObject.Get() == &SkillData
			|| (ExistingSpec.Ability
				&& ExistingSpec.Ability->GetClass() == SkillData.AbilityClass.Get()
				&& ExistingSpec.GetDynamicSpecSourceTags().HasTagExact(SkillData.AbilityTag)
				&& ExistingSpec.GetDynamicSpecSourceTags().HasTagExact(SkillData.InputTag)))
		{
			return ExistingSpec.Handle;
		}
	}
	FGameplayAbilitySpec Spec(SkillData.AbilityClass, 1, INDEX_NONE,
		const_cast<UPRPlayerSkillDataAsset*>(&SkillData));
	Spec.GetDynamicSpecSourceTags().AddTag(SkillData.AbilityTag);
	Spec.GetDynamicSpecSourceTags().AddTag(SkillData.InputTag);
	return ASC.GiveAbility(Spec);
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
	APRSkillDecoyActor* Decoy = World.Get()->SpawnActor<APRSkillDecoyActor>();
	if (!TestNotNull(TEXT("Afterimage decoy actor is available"), Decoy))
	{
		return false;
	}
	TestFalse(TEXT("Fresh decoy rejects a null attacker"), Decoy->ConsumeAttackProxy(nullptr));
	Decoy->Destroy();
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
	Subject->Destroy();

	FCombatPlayer SkillSource;
	FCombatPlayer SkillTarget;
	if (!TestTrue(TEXT("Formal skill source fixture spawns"), SpawnCombatPlayer(
		World.Get(), FVector::ZeroVector, TEXT("Automation.SkillSource"), SkillSource))
		|| !TestTrue(TEXT("Formal skill target fixture spawns"), SpawnCombatPlayer(
			World.Get(), FVector(200.0f, 0.0f, 0.0f), TEXT("Automation.SkillTarget"), SkillTarget)))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}
	SkillSource.Character->GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	TestTrue(TEXT("Formal side-view right facing resolves along positive X"),
		SkillSource.Character->GetMesh()->GetRightVector().Equals(FVector::ForwardVector, 0.01f));
	const FVector DecoySpawnLocation(321.0f, 17.0f, 89.0f);
	APRSkillDecoyActor* RuntimeDecoy = World.Get()->SpawnActor<APRSkillDecoyActor>(
		APRSkillDecoyActor::StaticClass(), DecoySpawnLocation, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Runtime decoy spawned"), RuntimeDecoy))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}
	TestTrue(TEXT("Afterimage decoy retains its authoritative spawn location"),
		RuntimeDecoy->GetActorLocation().Equals(DecoySpawnLocation, 0.01f));
	RuntimeDecoy->InitializeProxy(
		SkillSource.Character,
		TEXT("Automation.AfterimageDecoy"),
		0.14f,
		1.5f);
	const int32 DecoyOutcomeStart = Events.Num();
	TestTrue(TEXT("First legal attacker consumes the decoy"),
		RuntimeDecoy->ConsumeAttackProxy(SkillTarget.Character));
	TestFalse(TEXT("A consumed decoy rejects a second attacker"),
		RuntimeDecoy->ConsumeAttackProxy(SkillTarget.Character));
	TestEqual(TEXT("Decoy consumption publishes exactly one AbilityOutcome"),
		Events.Num(), DecoyOutcomeStart + 1);
	if (Events.Num() == DecoyOutcomeStart + 1)
	{
		const FPRCombatEvent& DecoyOutcome = Events.Last();
		TestEqual(TEXT("Decoy consumption is an AbilityOutcome"),
			DecoyOutcome.EventTag, UPRTagLibrary::GetCombatEventAbilityOutcomeTag());
		TestTrue(TEXT("Decoy consumption preserves its response"),
			DecoyOutcome.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseDecoyConsumedTag()));
		TestTrue(TEXT("Consumption inside the window marks PerfectTiming"),
			DecoyOutcome.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePerfectTimingTag()));
	}
	const UPRPlayerSkillDataAsset* ThunderData = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_ThunderDrop.DA_Skill_ThunderDrop"));
	UClass* StunnedEffectClass = LoadClass<UGameplayEffect>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Stunned.GE_State_Stunned_C"));
	UPRPlayerSkillSubsystem* ThunderSubsystem = World.Get()->GetSubsystem<UPRPlayerSkillSubsystem>();
	if (!TestNotNull(TEXT("Formal ThunderDrop data exists"), ThunderData)
		|| !TestNotNull(TEXT("ThunderDrop Stunned effect exists"), StunnedEffectClass)
		|| !TestNotNull(TEXT("PlayerSkill subsystem exists"), ThunderSubsystem))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}
	FPRPlayerSkillExecutionSnapshot ThunderSnapshot;
	ThunderSnapshot.AbilityTag = ThunderData->AbilityTag;
	ThunderSnapshot.InputTag = ThunderData->InputTag;
	ThunderSnapshot.SourceActor = SkillSource.Character;
	ThunderSnapshot.Origin = SkillSource.Character->GetActorLocation();
	ThunderSnapshot.AimDirection = FVector::ForwardVector;
	ThunderSnapshot.AttackPower = 10.0f;
	ThunderSnapshot.BaseDamage = ThunderData->BaseDamage;
	ThunderSnapshot.AttackPowerScale = ThunderData->AttackPowerScale;
	TestTrue(TEXT("ThunderDrop schedules exactly one delayed impact"),
		ThunderSubsystem->ScheduleThunderDrop(
			ThunderSnapshot,
			SkillTarget.Character->GetActorLocation(),
			*const_cast<UPRPlayerSkillDataAsset*>(ThunderData),
			StunnedEffectClass));
	World.TickFor(0.59f);
	TestEqual(TEXT("ThunderDrop has no early damage before 0.60 seconds"),
		SkillTarget.Attributes->GetHealth(), 100.0f);
	World.TickFor(0.02f);
	TestEqual(TEXT("ThunderDrop applies 25 + 2.0 * AttackPower once"),
		SkillTarget.Attributes->GetHealth(), 55.0f);
	TestTrue(TEXT("ThunderDrop applies Stunned after successful damage"),
		SkillTarget.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()));
	World.TickFor(0.8f);
	TestFalse(TEXT("ThunderDrop Stunned naturally expires before B regression"),
		SkillTarget.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()));
	SkillTarget.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);

	const UPRPlayerSkillDataAsset* GuardData = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_CounterProofWall.DA_Skill_CounterProofWall"));
	if (!TestNotNull(TEXT("Formal CounterProofWall data exists"), GuardData))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}
	UPRGA_CounterProofWall* GuardCDO = GuardData->AbilityClass
		? Cast<UPRGA_CounterProofWall>(GuardData->AbilityClass->GetDefaultObject()) : nullptr;
	if (!TestNotNull(TEXT("Formal CounterProofWall Blueprint uses the native guard parent"), GuardCDO))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}
	FCombatPlayer GuardPlayer;
	FCombatPlayer GuardAttacker;
	if (!TestTrue(TEXT("CounterProofWall player fixture spawns"), SpawnCombatPlayer(
		World.Get(), FVector::ZeroVector, TEXT("Automation.GuardPlayer"), GuardPlayer))
		|| !TestTrue(TEXT("CounterProofWall attacker fixture spawns"), SpawnCombatPlayer(
			World.Get(), FVector(200.0f, 0.0f, 0.0f), TEXT("Automation.GuardAttacker"), GuardAttacker)))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}
	GuardPlayer.Character->GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	const FGameplayAbilitySpecHandle GuardHandle = GrantFormalSkill(*GuardPlayer.ASC, *GuardData);
	if (!TestTrue(TEXT("Formal CounterProofWall spec grants"), GuardHandle.IsValid()))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}
	FGameplayTagContainer GuardingTags;
	GuardingTags.AddTag(UPRTagLibrary::GetStateGuardingTag());
	const FGameplayEffectQuery GuardingQuery =
		FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(GuardingTags);
	const float GuardInitialHealth = GuardPlayer.Attributes->GetHealth();
	const float GuardInitialShield = GuardPlayer.Attributes->GetShield();
	const int32 GuardEventStart = Events.Num();
	GuardPlayer.ASC->AbilityInputTagPressed(GuardData->InputTag);
	TestEqual(TEXT("CounterProofWall press enters guarding Active phase"),
		GuardPlayer.Character->GetPlayerSkillComponent()->GetCurrentPhase(), EPRPlayerSkillPhase::Active);
	TestTrue(TEXT("CounterProofWall grants State.Guarding"),
		GuardPlayer.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	TestEqual(TEXT("CounterProofWall press commits exactly 20 Energy"),
		GuardPlayer.Attributes->GetEnergy(), 80.0f);
	TestEqual(TEXT("CounterProofWall owns one Guarding effect"),
		GuardPlayer.ASC->GetActiveEffects(GuardingQuery).Num(), 1);
	GuardPlayer.ASC->AbilityInputTagPressed(GuardData->InputTag);
	TestEqual(TEXT("Repeated CounterProofWall press does not recommit Energy"),
		GuardPlayer.Attributes->GetEnergy(), 80.0f);
	TestEqual(TEXT("Repeated CounterProofWall press does not stack Guarding"),
		GuardPlayer.ASC->GetActiveEffects(GuardingQuery).Num(), 1);

	FPRDamageRequest GuardDamage;
	GuardDamage.SourceId = TEXT("Automation.GuardAttacker");
	GuardDamage.DamageSource = GuardAttacker.PlayerState;
	GuardDamage.Instigator = GuardAttacker.Character;
	GuardDamage.Target = GuardPlayer.Character;
	GuardDamage.AbilityTag = GuardData->AbilityTag;
	GuardDamage.RawDamage = 10.0f;
	GuardDamage.ImpactOrigin = GuardAttacker.Character->GetActorLocation();
	// Source -> target travels along -X; target -> source therefore aligns with its +X facing.
	GuardDamage.IncomingDirection = -FVector::ForwardVector;
	TestEqual(TEXT("CounterProofWall blocks a legal front hit"),
		Combat->ApplyDamage(GuardDamage), EPRCombatRequestStatus::RejectedBlocked);
	TestEqual(TEXT("CounterProofWall first block preserves Health"),
		GuardPlayer.Attributes->GetHealth(), GuardInitialHealth);
	TestEqual(TEXT("CounterProofWall first block preserves Shield"),
		GuardPlayer.Attributes->GetShield(), GuardInitialShield);
	TestEqual(TEXT("CounterProofWall first block emits exactly one rejection"),
		Events.Num(), GuardEventStart + 1);
	if (Events.Num() == GuardEventStart + 1)
	{
		const FPRCombatEvent& PerfectBlock = Events.Last();
		TestEqual(TEXT("CounterProofWall block is DamageRejected"),
			PerfectBlock.EventTag, UPRTagLibrary::GetCombatEventDamageRejectedTag());
		TestTrue(TEXT("CounterProofWall block carries GuardBlocked"),
			PerfectBlock.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseGuardBlockedTag()));
		TestTrue(TEXT("CounterProofWall opening block carries PerfectTiming"),
			PerfectBlock.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePerfectTimingTag()));
	}
	TestEqual(TEXT("CounterProofWall blocks another legal hit in one activation"),
		Combat->ApplyDamage(GuardDamage), EPRCombatRequestStatus::RejectedBlocked);
	TestEqual(TEXT("CounterProofWall repeated block still preserves Health"),
		GuardPlayer.Attributes->GetHealth(), GuardInitialHealth);
	World.TickFor(0.16f);
	const int32 PostPerfectEventStart = Events.Num();
	TestEqual(TEXT("CounterProofWall remains active outside the perfect window"),
		Combat->ApplyDamage(GuardDamage), EPRCombatRequestStatus::RejectedBlocked);
	if (Events.Num() == PostPerfectEventStart + 1)
	{
		const FPRCombatEvent& NormalBlock = Events.Last();
		TestTrue(TEXT("CounterProofWall later block retains GuardBlocked"),
			NormalBlock.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseGuardBlockedTag()));
		TestFalse(TEXT("CounterProofWall later block has no PerfectTiming"),
			NormalBlock.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponsePerfectTimingTag()));
	}
	GuardDamage.IncomingDirection = FVector::ForwardVector;
	TestEqual(TEXT("CounterProofWall permits a back hit through normal Combat"),
		Combat->ApplyDamage(GuardDamage), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("CounterProofWall back hit damages Health"),
		GuardPlayer.Attributes->GetHealth(), GuardInitialHealth - 10.0f);
	GuardPlayer.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), GuardInitialHealth);
	FGameplayTagContainer InvalidDirectionTags;
	GuardDamage.IncomingDirection = FVector::ZeroVector;
	TestEqual(TEXT("CounterProofWall never blocks a zero incoming direction"),
		GuardPlayer.Character->EvaluateIncomingDamage(GuardDamage, InvalidDirectionTags),
		EPRCombatMitigationResult::NotHandled);
	TestTrue(TEXT("CounterProofWall zero direction adds no response tags"), InvalidDirectionTags.IsEmpty());

	GuardPlayer.ASC->AbilityInputTagReleased(GuardData->InputTag);
	TestEqual(TEXT("CounterProofWall release returns the phase to Idle"),
		GuardPlayer.Character->GetPlayerSkillComponent()->GetCurrentPhase(), EPRPlayerSkillPhase::Idle);
	TestFalse(TEXT("CounterProofWall release clears State.Guarding"),
		GuardPlayer.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	TestEqual(TEXT("CounterProofWall release removes its Guarding effect"),
		GuardPlayer.ASC->GetActiveEffects(GuardingQuery).Num(), 0);
	GuardPlayer.ASC->AbilityInputTagReleased(GuardData->InputTag);
	TestEqual(TEXT("Repeated CounterProofWall release remains idempotent"),
		GuardPlayer.ASC->GetActiveEffects(GuardingQuery).Num(), 0);

	World.TickFor(6.01f);
	GuardPlayer.ASC->AbilityInputTagPressed(GuardData->InputTag);
	TestTrue(TEXT("CounterProofWall can reactivate after its cooldown"),
		GuardPlayer.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	World.TickFor(1.01f);
	TestEqual(TEXT("CounterProofWall held guard ends at the one second maximum"),
		GuardPlayer.Character->GetPlayerSkillComponent()->GetCurrentPhase(), EPRPlayerSkillPhase::Idle);
	TestFalse(TEXT("CounterProofWall maximum duration clears State.Guarding"),
		GuardPlayer.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	TestEqual(TEXT("CounterProofWall maximum duration leaves no Guarding effect"),
		GuardPlayer.ASC->GetActiveEffects(GuardingQuery).Num(), 0);

	const UPRPlayerSkillDataAsset* FireData = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_FireSlash.DA_Skill_FireSlash"));
	const UPRPlayerSkillDataAsset* ShadowData = LoadObject<UPRPlayerSkillDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_ShadowThrust.DA_Skill_ShadowThrust"));
	if (!TestNotNull(TEXT("Formal FireSlash data exists"), FireData)
		|| !TestNotNull(TEXT("Formal ShadowThrust data exists"), ShadowData))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}

	UPRPlayerSkillGameplayAbility* FireCDO = FireData->AbilityClass
		? FireData->AbilityClass->GetDefaultObject<UPRPlayerSkillGameplayAbility>() : nullptr;
	UPRPlayerSkillGameplayAbility* ShadowCDO = ShadowData->AbilityClass
		? ShadowData->AbilityClass->GetDefaultObject<UPRPlayerSkillGameplayAbility>() : nullptr;
	TestNotNull(TEXT("Formal FireSlash Blueprint uses the native FireSlash parent"),
		Cast<UPRGA_FireSlash>(FireCDO));
	TestNotNull(TEXT("Formal ShadowThrust Blueprint uses the native ShadowThrust parent"),
		Cast<UPRGA_ShadowThrust>(ShadowCDO));
	if (FireCDO && ShadowCDO)
	{
		TestEqual(TEXT("Formal FireSlash uses OnInputTriggered"),
			FireCDO->GetActivationPolicy(), EPRAbilityActivationPolicy::OnInputTriggered);
		TestEqual(TEXT("Formal FireSlash and ShadowThrust share the same GAS net execution policy"),
			FireCDO->GetNetExecutionPolicy(), ShadowCDO->GetNetExecutionPolicy());
		TestEqual(TEXT("Formal FireSlash and ShadowThrust share InstancedPerActor"),
			FireCDO->GetInstancingPolicy(), ShadowCDO->GetInstancingPolicy());
	}
	FClassProperty* BurningProperty = FireCDO
		? FindFProperty<FClassProperty>(FireCDO->GetClass(), TEXT("BurningEffectClass")) : nullptr;
	if (TestNotNull(TEXT("FireSlash has a Burning effect binding"), BurningProperty))
	{
		BurningProperty->SetObjectPropertyValue_InContainer(
			FireCDO, UPRPlayerSkillBurningTestEffect::StaticClass());
	}

	const FGameplayAbilitySpecHandle FireHandle = GrantFormalSkill(*SkillSource.ASC, *FireData);
	TestTrue(TEXT("Formal FireSlash spec granted for runtime test"), FireHandle.IsValid());
	const FGameplayAbilitySpec* FireSpec = SkillSource.ASC->FindAbilitySpecFromHandle(FireHandle);
	TestFalse(TEXT("Formal FireSlash Spec starts inactive"), FireSpec && FireSpec->IsActive());
	TestTrue(TEXT("Formal FireSlash Spec preserves its InputTag"), FireSpec
		&& FireSpec->GetDynamicSpecSourceTags().HasTagExact(FireData->InputTag));
	FPRAbilityRuntimeState FireRuntimeState;
	TestTrue(TEXT("ASC resolves FireSlash by its exact InputTag"),
		SkillSource.ASC->GetAbilityRuntimeStateByInputTag(FireData->InputTag, FireRuntimeState));
	const int32 FireEventStart = Events.Num();
	SkillSource.ASC->AbilityInputTagPressed(FireData->InputTag);
	TestEqual(TEXT("FireSlash enters Startup immediately"),
		SkillSource.Character->GetPlayerSkillComponent()->GetCurrentPhase(),
		EPRPlayerSkillPhase::Startup);
	World.TickFor(0.16f);
	TestEqual(TEXT("FireSlash enters Recovery after its initial hit"),
		SkillSource.Character->GetPlayerSkillComponent()->GetCurrentPhase(),
		EPRPlayerSkillPhase::Recovery);
	TestEqual(TEXT("FireSlash commits exactly 25 Energy"), SkillSource.Attributes->GetEnergy(), 75.0f);
	TestEqual(TEXT("FireSlash initial hit uses 15 + 1.0 AP"), SkillTarget.Attributes->GetHealth(), 75.0f);
	TestTrue(TEXT("FireSlash initial hit grants State.Burning"),
		SkillTarget.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateBurningTag()));
	World.TickFor(1.51f);
	TestEqual(TEXT("Burning deals three ticks of 4 + 0.1 AP"),
		SkillTarget.Attributes->GetHealth(), 60.0f);
	TestFalse(TEXT("Burning tag clears after its third tick"),
		SkillTarget.ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateBurningTag()));
	int32 FireDamageEvents = 0;
	for (int32 Index = FireEventStart; Index < Events.Num(); ++Index)
	{
		if (Events[Index].AbilityTag == FireData->AbilityTag
			&& Events[Index].Instigator.Get() == SkillSource.Character
			&& Events[Index].Target.Get() == SkillTarget.Character
			&& Events[Index].EventTag == UPRTagLibrary::GetCombatEventDamageTag())
		{
			++FireDamageEvents;
		}
	}
	TestEqual(TEXT("FireSlash publishes one initial and three Burning damage events"), FireDamageEvents, 4);

	UPRPlayerSkillSubsystem* SkillSubsystem = World.Get()->GetSubsystem<UPRPlayerSkillSubsystem>();
	const UPRFireSlashSkillFragment* FireFragment = Cast<UPRFireSlashSkillFragment>(FireData->SkillFragment);
	if (!TestNotNull(TEXT("PlayerSkill subsystem exists for Burning lifecycle"), SkillSubsystem)
		|| !TestNotNull(TEXT("Formal FireSlash fragment exists for Burning lifecycle"), FireFragment))
	{
		Combat->OnCombatEvent().Remove(EventHandle);
		return false;
	}
	FPRPlayerSkillExecutionSnapshot CriticalSnapshot;
	CriticalSnapshot.SourceActor = SkillSource.Character;
	CriticalSnapshot.AbilityTag = ShadowData->AbilityTag;
	CriticalSnapshot.AimDirection = FVector::ForwardVector;
	CriticalSnapshot.AttackPower = 10.0f;
	SkillTarget.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);
	TestEqual(TEXT("Explicit critical input applies through Combat"),
		SkillSubsystem->ApplySkillDamage(
			CriticalSnapshot,
			*SkillTarget.Character,
			*ShadowCDO,
			SkillSource.Character->GetActorLocation(),
			25.0f,
			1.5f,
			true,
			FVector::ForwardVector),
		EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Critical formula is (25 + 1.5 AP) x 1.5 without RNG"),
		SkillTarget.Attributes->GetHealth(), 40.0f);
	FGameplayTagContainer BurningTags;
	BurningTags.AddTag(UPRTagLibrary::GetStateBurningTag());
	const FGameplayEffectQuery BurningQuery =
		FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(BurningTags);
	FPRPlayerSkillExecutionSnapshot BurningSnapshot;
	BurningSnapshot.SourceActor = SkillSource.Character;
	BurningSnapshot.AbilityTag = FireData->AbilityTag;
	BurningSnapshot.AimDirection = FVector::ForwardVector;
	BurningSnapshot.AttackPower = 10.0f;
	SkillTarget.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);
	TestTrue(TEXT("Burning lifecycle fixture applies its first instance"),
		SkillSubsystem->ApplyOrRefreshBurning(
			BurningSnapshot,
			*SkillTarget.Character,
			*FireCDO,
			UPRPlayerSkillBurningTestEffect::StaticClass(),
			*FireFragment));
	World.TickFor(0.25f);
	FPRPlayerSkillExecutionSnapshot RefreshedSnapshot = BurningSnapshot;
	RefreshedSnapshot.SourceActor = Character;
	RefreshedSnapshot.AttackPower = 20.0f;
	TestTrue(TEXT("A different source refreshes the one Burning instance"),
		SkillSubsystem->ApplyOrRefreshBurning(
			RefreshedSnapshot,
			*SkillTarget.Character,
			*FireCDO,
			UPRPlayerSkillBurningTestEffect::StaticClass(),
			*FireFragment));
	TestEqual(TEXT("Burning refresh never stacks a second ActiveEffect"),
		SkillTarget.ASC->GetActiveEffects(BurningQuery).Num(), 1);
	World.TickFor(1.51f);
	TestEqual(TEXT("Burning refresh restarts three ticks from the latest snapshot"),
		SkillTarget.Attributes->GetHealth(), 82.0f);
	TestEqual(TEXT("Refreshed Burning clears its only ActiveEffect"),
		SkillTarget.ASC->GetActiveEffects(BurningQuery).Num(), 0);

	SkillTarget.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);
	TestTrue(TEXT("Burning can be applied before an external early removal"),
		SkillSubsystem->ApplyOrRefreshBurning(
			BurningSnapshot,
			*SkillTarget.Character,
			*FireCDO,
			UPRPlayerSkillBurningTestEffect::StaticClass(),
			*FireFragment));
	TestEqual(TEXT("External removal removes the one Burning effect"),
		SkillTarget.ASC->RemoveActiveEffects(BurningQuery), 1);
	World.TickFor(0.51f);
	TestEqual(TEXT("Externally removed Burning produces no later Tick"),
		SkillTarget.Attributes->GetHealth(), 100.0f);

	TestTrue(TEXT("Burning can be applied before target death"),
		SkillSubsystem->ApplyOrRefreshBurning(
			BurningSnapshot,
			*SkillTarget.Character,
			*FireCDO,
			UPRPlayerSkillBurningTestEffect::StaticClass(),
			*FireFragment));
	SkillTarget.ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateDeadTag(), 1, EGameplayTagReplicationState::TagOnly);
	World.TickFor(0.51f);
	TestEqual(TEXT("Target death clears Burning without a Tick"),
		SkillTarget.Attributes->GetHealth(), 100.0f);
	TestEqual(TEXT("Target death clears the Burning ActiveEffect"),
		SkillTarget.ASC->GetActiveEffects(BurningQuery).Num(), 0);
	SkillTarget.ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateDeadTag(), 0, EGameplayTagReplicationState::TagOnly);

	SkillTarget.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);
	const FGameplayAbilitySpecHandle ShadowHandle = GrantFormalSkill(*SkillSource.ASC, *ShadowData);
	TestTrue(TEXT("Formal ShadowThrust spec granted for runtime test"), ShadowHandle.IsValid());
	const int32 ShadowEventStart = Events.Num();
	SkillSource.ASC->AbilityInputTagPressed(ShadowData->InputTag);
	World.TickFor(0.09f);
	TestEqual(TEXT("ShadowThrust commits exactly 20 Energy"), SkillSource.Attributes->GetEnergy(), 55.0f);
	TestEqual(TEXT("ShadowThrust enters Active after startup"),
		SkillSource.Character->GetPlayerSkillComponent()->GetCurrentPhase(), EPRPlayerSkillPhase::Active);
	TestTrue(TEXT("ShadowThrust owns a controlled displacement request"),
		SkillSource.Character->GetPlayerSkillComponent()->GetActiveDisplacementRequestId().IsValid());
	TestTrue(TEXT("ShadowThrust displacement is backed by RootMotionSource"),
		SkillSource.Character->GetCharacterMovement()->CurrentRootMotion.HasActiveRootMotionSources());
	// This transient world has no game-mode movement loop. Mirror the targeting
	// automation fixture by advancing to the RootMotionSource endpoint; the
	// ability still owns and evaluates the actual 0 -> 450 path.
	SkillSource.Character->SetActorLocation(FVector(450.0f, 0.0f, 0.0f));
	World.TickFor(0.31f);
	TestEqual(TEXT("ShadowThrust uses 25 + 1.5 AP once"), SkillTarget.Attributes->GetHealth(), 60.0f);
	int32 ShadowDamageEvents = 0;
	int32 ShadowOutcomeEvents = 0;
	for (int32 Index = ShadowEventStart; Index < Events.Num(); ++Index)
	{
		if (Events[Index].AbilityTag != ShadowData->AbilityTag)
		{
			continue;
		}
		if (Events[Index].Instigator.Get() != SkillSource.Character
			|| Events[Index].Target.Get() != SkillTarget.Character)
		{
			continue;
		}
		ShadowDamageEvents += Events[Index].EventTag == UPRTagLibrary::GetCombatEventDamageTag() ? 1 : 0;
		ShadowOutcomeEvents += Events[Index].EventTag == UPRTagLibrary::GetCombatEventAbilityOutcomeTag() ? 1 : 0;
	}
	TestEqual(TEXT("ShadowThrust hits a TargetId at most once"), ShadowDamageEvents, 1);
	TestTrue(TEXT("ShadowThrust moves forward without leaving the X/Z plane"),
		SkillSource.Character->GetActorLocation().X > 0.0f
			&& FMath::IsNearlyZero(SkillSource.Character->GetActorLocation().Y, 1.0f));
	TestTrue(TEXT("Completed ShadowThrust publishes at most one displacement outcome"),
		ShadowOutcomeEvents <= 1);

	SkillTarget.ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 100.0f);
	TestTrue(TEXT("Burning can be applied before World Cleanup"),
		SkillSubsystem->ApplyOrRefreshBurning(
			BurningSnapshot,
			*SkillTarget.Character,
			*FireCDO,
			UPRPlayerSkillBurningTestEffect::StaticClass(),
			*FireFragment));
	FWorldDelegates::OnWorldCleanup.Broadcast(World.Get(), true, true);
	TestEqual(TEXT("World Cleanup removes the Burning ActiveEffect"),
		SkillTarget.ASC->GetActiveEffects(BurningQuery).Num(), 0);
	World.TickFor(0.51f);
	TestEqual(TEXT("World Cleanup leaves no Burning Timer damage"),
		SkillTarget.Attributes->GetHealth(), 100.0f);

	Combat->OnCombatEvent().Remove(EventHandle);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
