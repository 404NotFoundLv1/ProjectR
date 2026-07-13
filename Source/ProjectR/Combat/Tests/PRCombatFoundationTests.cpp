// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Characters/PRPlayerCharacter.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRDamageExecutionCalculation.h"
#include "Core/PRCombatantInterface.h"
#include "Core/PRCombatFeedbackInterface.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "Core/PRTagLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include <limits>

namespace PRCombatAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

class FScopedTestWorld
{
public:
	FScopedTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
	}

	~FScopedTestWorld()
	{
		if (World)
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	}

	UWorld* Get() const
	{
		return World;
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

struct FCombatFixture
{
	explicit FCombatFixture(FAutomationTestBase& Test)
	{
		PlayerState = SpawnBlueprintActor<APRPlayerState>(
			World.Get(),
			TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerState.BP_PlayerState_C"));
		Character = SpawnBlueprintActor<APRPlayerCharacter>(
			World.Get(),
			TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerCharacter.BP_PlayerCharacter_C"));
		Controller = SpawnBlueprintActor<APRPlayerController>(
			World.Get(),
			TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerController.BP_PlayerController_C"));
		if (!Test.TestNotNull(TEXT("PlayerState spawned"), PlayerState)
			|| !Test.TestNotNull(TEXT("Character spawned"), Character)
			|| !Test.TestNotNull(TEXT("Controller spawned"), Controller))
		{
			return;
		}

		Controller->SetPlayerState(PlayerState);
		Controller->Possess(Character);
		ASC = PlayerState->GetProjectRAbilitySystemComponent();
		Attributes = PlayerState->GetAttributeSet();
		Subsystem = World.Get()->GetSubsystem<UPRCombatSubsystem>();
		bValid = Test.TestNotNull(TEXT("ASC exists"), ASC)
			&& Test.TestNotNull(TEXT("AttributeSet exists"), Attributes)
			&& Test.TestNotNull(TEXT("Combat subsystem exists"), Subsystem);
	}

	FPRDamageRequest MakeDamage(float RawDamage) const
	{
		FPRDamageRequest Request;
		Request.SourceId = TEXT("Automation.Combat");
		Request.DamageSource = PlayerState;
		Request.Instigator = Character;
		Request.Target = Character;
		Request.RawDamage = RawDamage;
		return Request;
	}

	FPRReviveRequest MakeRevive() const
	{
		FPRReviveRequest Request;
		Request.SourceId = TEXT("Automation.Revive");
		Request.DamageSource = PlayerState;
		Request.Instigator = Character;
		Request.Target = Character;
		return Request;
	}

	FScopedTestWorld World;
	APRPlayerState* PlayerState = nullptr;
	APRPlayerCharacter* Character = nullptr;
	APRPlayerController* Controller = nullptr;
	UPRAbilitySystemComponent* ASC = nullptr;
	const UPRAttributeSet* Attributes = nullptr;
	UPRCombatSubsystem* Subsystem = nullptr;
	bool bValid = false;
};

bool HasTag(const UPRAbilitySystemComponent* ASC, const FGameplayTag& Tag)
{
	return ASC && ASC->HasMatchingGameplayTag(Tag);
}
} // namespace PRCombatAutomation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatSchemaTest,
	"ProjectR.Combat.Schema.TypesAttributesAndTags",
	PRCombatAutomation::TestFlags)

bool FPRCombatSchemaTest::RunTest(const FString& Parameters)
{
	const FPRDamageRequest DamageDefaults;
	TestTrue(TEXT("Damage SourceId defaults empty"), DamageDefaults.SourceId.IsNone());
	TestEqual(TEXT("Damage defaults zero"), DamageDefaults.RawDamage, 0.0f);
	TestFalse(TEXT("Critical defaults false"), DamageDefaults.bCritical);
	const FPRReviveRequest ReviveDefaults;
	TestEqual(TEXT("Revive health defaults full"), ReviveDefaults.HealthFraction, 1.0f);
	TestEqual(TEXT("Revive shield defaults full"), ReviveDefaults.ShieldFraction, 1.0f);
	TestEqual(TEXT("First request status is Applied"),
		static_cast<uint8>(EPRCombatRequestStatus::Applied), static_cast<uint8>(0));
	TestEqual(TEXT("Last request status is Invalid"),
		static_cast<uint8>(EPRCombatRequestStatus::Invalid), static_cast<uint8>(4));

	UPRAttributeSet* AttributeSet = NewObject<UPRAttributeSet>();
	FProperty* IncomingProperty = UPRAttributeSet::GetIncomingDamageAttribute().GetUProperty();
	if (TestNotNull(TEXT("IncomingDamage is reflected"), IncomingProperty))
	{
		TestTrue(TEXT("IncomingDamage is transient"), IncomingProperty->HasAnyPropertyFlags(CPF_Transient));
		TestFalse(TEXT("IncomingDamage is not replicated"), IncomingProperty->HasAnyPropertyFlags(CPF_Net));
		TestTrue(TEXT("IncomingDamage has no RepNotify"), IncomingProperty->RepNotifyFunc.IsNone());
	}
	TestEqual(TEXT("IncomingDamage defaults zero"), AttributeSet->GetIncomingDamage(), 0.0f);

	const TPair<const TCHAR*, const FGameplayTag&> Tags[] = {
		{TEXT("Combat.Data.Damage"), UPRTagLibrary::GetCombatDataDamageTag()},
		{TEXT("Combat.Event.Damage"), UPRTagLibrary::GetCombatEventDamageTag()},
		{TEXT("Combat.Event.DamageRejected"), UPRTagLibrary::GetCombatEventDamageRejectedTag()},
		{TEXT("Combat.Event.Death"), UPRTagLibrary::GetCombatEventDeathTag()},
		{TEXT("Combat.Event.Revive"), UPRTagLibrary::GetCombatEventReviveTag()},
		{TEXT("Combat.Response.HealthDamaged"), UPRTagLibrary::GetCombatResponseHealthDamagedTag()},
		{TEXT("Combat.Response.ShieldAbsorbed"), UPRTagLibrary::GetCombatResponseShieldAbsorbedTag()},
		{TEXT("Combat.Response.ShieldBroken"), UPRTagLibrary::GetCombatResponseShieldBrokenTag()}};
	for (const TPair<const TCHAR*, const FGameplayTag&>& Entry : Tags)
	{
		TestTrue(Entry.Key, Entry.Value.IsValid());
		TestEqual(Entry.Key, Entry.Value.ToString(), FString(Entry.Key));
	}

	TestTrue(TEXT("PlayerState implements Combatant"), APRPlayerState::StaticClass()->ImplementsInterface(UPRCombatantInterface::StaticClass()));
	TestTrue(TEXT("PlayerCharacter implements Feedback"), APRPlayerCharacter::StaticClass()->ImplementsInterface(UPRCombatFeedbackInterface::StaticClass()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatDamageLifecycleTest,
	"ProjectR.Combat.Runtime.DamageDeathReviveAndEvents",
	PRCombatAutomation::TestFlags)

bool FPRCombatDamageLifecycleTest::RunTest(const FString& Parameters)
{
	PRCombatAutomation::FCombatFixture Fixture(*this);
	if (!Fixture.bValid)
	{
		return false;
	}

	TArray<FPRCombatEvent> Events;
	const FDelegateHandle EventHandle = Fixture.Subsystem->OnCombatEvent().AddLambda(
		[&Events](const FPRCombatEvent& Event)
		{
			Events.Add(Event);
		});

	FPRDamageRequest FirstHit = Fixture.MakeDamage(20.0f);
	FirstHit.bCritical = true;
	FirstHit.DamageTags.AddTag(UPRTagLibrary::GetStateStunnedTag());
	TestEqual(TEXT("Shield hit applies"), Fixture.Subsystem->ApplyDamage(FirstHit), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("First hit keeps Health"), Fixture.Attributes->GetHealth(), 100.0f);
	TestEqual(TEXT("First hit reduces Shield"), Fixture.Attributes->GetShield(), 30.0f);
	TestEqual(TEXT("IncomingDamage is consumed"), Fixture.Attributes->GetIncomingDamage(), 0.0f);

	TestEqual(TEXT("Spill-over applies"), Fixture.Subsystem->ApplyDamage(Fixture.MakeDamage(40.0f)), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Spill-over empties Shield"), Fixture.Attributes->GetShield(), 0.0f);
	TestEqual(TEXT("Spill-over reaches Health"), Fixture.Attributes->GetHealth(), 90.0f);

	Fixture.ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateInvulnerableTag(), 1, EGameplayTagReplicationState::TagOnly);
	TestEqual(TEXT("Invulnerable hit is rejected"),
		Fixture.Subsystem->ApplyDamage(Fixture.MakeDamage(10.0f)),
		EPRCombatRequestStatus::RejectedInvulnerable);
	TestEqual(TEXT("Invulnerable preserves Health"), Fixture.Attributes->GetHealth(), 90.0f);
	Fixture.ASC->SetLooseGameplayTagCount(
		UPRTagLibrary::GetStateInvulnerableTag(), 0, EGameplayTagReplicationState::TagOnly);

	TestEqual(TEXT("Fatal hit applies"), Fixture.Subsystem->ApplyDamage(Fixture.MakeDamage(200000.0f)), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Fatal hit clamps Health to zero"), Fixture.Attributes->GetHealth(), 0.0f);
	TestTrue(TEXT("Dead tag is present"), PRCombatAutomation::HasTag(Fixture.ASC, UPRTagLibrary::GetStateDeadTag()));
	TestFalse(TEXT("Alive tag is absent"), PRCombatAutomation::HasTag(Fixture.ASC, UPRTagLibrary::GetStateAliveTag()));
	TestEqual(TEXT("Death locks movement"), Fixture.Character->GetCharacterMovement()->MovementMode, MOVE_None);

	TestEqual(TEXT("Dead hit is rejected"),
		Fixture.Subsystem->ApplyDamage(Fixture.MakeDamage(1.0f)),
		EPRCombatRequestStatus::RejectedDead);
	TestEqual(TEXT("Revive applies"), Fixture.Subsystem->Revive(Fixture.MakeRevive()), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Revive restores Health"), Fixture.Attributes->GetHealth(), 100.0f);
	TestEqual(TEXT("Revive restores Shield"), Fixture.Attributes->GetShield(), 50.0f);
	TestTrue(TEXT("Alive tag restored"), PRCombatAutomation::HasTag(Fixture.ASC, UPRTagLibrary::GetStateAliveTag()));
	TestFalse(TEXT("Dead tag removed"), PRCombatAutomation::HasTag(Fixture.ASC, UPRTagLibrary::GetStateDeadTag()));
	TestEqual(TEXT("Revive restores walking"), Fixture.Character->GetCharacterMovement()->MovementMode, MOVE_Walking);
	TestEqual(TEXT("Repeated revive rejected"),
		Fixture.Subsystem->Revive(Fixture.MakeRevive()), EPRCombatRequestStatus::RejectedAlive);

	const FGameplayTag ExpectedOrder[] = {
		UPRTagLibrary::GetCombatEventDamageTag(),
		UPRTagLibrary::GetCombatEventDamageTag(),
		UPRTagLibrary::GetCombatEventDamageRejectedTag(),
		UPRTagLibrary::GetCombatEventDamageTag(),
		UPRTagLibrary::GetCombatEventDeathTag(),
		UPRTagLibrary::GetCombatEventDamageRejectedTag(),
		UPRTagLibrary::GetCombatEventReviveTag()};
	const int32 ExpectedEventCount = static_cast<int32>(UE_ARRAY_COUNT(ExpectedOrder));
	TestEqual(TEXT("Event count"), Events.Num(), ExpectedEventCount);
	TSet<FGuid> EventIds;
	for (int32 Index = 0; Index < FMath::Min(Events.Num(), ExpectedEventCount); ++Index)
	{
		TestEqual(*FString::Printf(TEXT("Event order %d"), Index), Events[Index].EventTag, ExpectedOrder[Index]);
		TestTrue(*FString::Printf(TEXT("Event ID %d is valid"), Index), Events[Index].EventId.IsValid());
		EventIds.Add(Events[Index].EventId);
		TestEqual(*FString::Printf(TEXT("Target ID %d"), Index), Events[Index].TargetId, FName(TEXT("Player")));
	}
	TestEqual(TEXT("Event IDs are unique"), EventIds.Num(), Events.Num());
	if (Events.Num() >= 2)
	{
		TestTrue(TEXT("Critical metadata propagated"), Events[0].bCritical);
		TestTrue(TEXT("DamageTags propagated"), Events[0].DamageTags.HasTagExact(UPRTagLibrary::GetStateStunnedTag()));
		TestEqual(TEXT("First shield absorbed"), Events[0].ShieldAbsorbed, 20.0f);
		TestEqual(TEXT("Spill-over health damage"), Events[1].HealthDamage, 10.0f);
		TestTrue(TEXT("Shield break response"), Events[1].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag()));
	}

	Fixture.Subsystem->OnCombatEvent().Remove(EventHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatValidationAndReplacementTest,
	"ProjectR.Combat.Runtime.ValidationAndPawnReplacement",
	PRCombatAutomation::TestFlags)

bool FPRCombatValidationAndReplacementTest::RunTest(const FString& Parameters)
{
	PRCombatAutomation::FCombatFixture Fixture(*this);
	if (!Fixture.bValid)
	{
		return false;
	}

	AddExpectedError(TEXT("invalid damage request"), EAutomationExpectedErrorFlags::Contains, 5);
	FPRDamageRequest Invalid = Fixture.MakeDamage(0.0f);
	TestEqual(TEXT("Zero is invalid"), Fixture.Subsystem->ApplyDamage(Invalid), EPRCombatRequestStatus::Invalid);
	Invalid.RawDamage = -1.0f;
	TestEqual(TEXT("Negative is invalid"), Fixture.Subsystem->ApplyDamage(Invalid), EPRCombatRequestStatus::Invalid);
	Invalid.RawDamage = std::numeric_limits<float>::quiet_NaN();
	TestEqual(TEXT("NaN is invalid"), Fixture.Subsystem->ApplyDamage(Invalid), EPRCombatRequestStatus::Invalid);
	Invalid.RawDamage = std::numeric_limits<float>::infinity();
	TestEqual(TEXT("Infinity is invalid"), Fixture.Subsystem->ApplyDamage(Invalid), EPRCombatRequestStatus::Invalid);
	Invalid = Fixture.MakeDamage(10.0f);
	Invalid.SourceId = NAME_None;
	TestEqual(TEXT("Empty source is invalid"), Fixture.Subsystem->ApplyDamage(Invalid), EPRCombatRequestStatus::Invalid);

	TestEqual(TEXT("Oversized finite damage clamps and applies"),
		Fixture.Subsystem->ApplyDamage(Fixture.MakeDamage(500000.0f)),
		EPRCombatRequestStatus::Applied);
	TestTrue(TEXT("Oversized damage kills"), PRCombatAutomation::HasTag(Fixture.ASC, UPRTagLibrary::GetStateDeadTag()));

	APRPlayerCharacter* Replacement = PRCombatAutomation::SpawnBlueprintActor<APRPlayerCharacter>(
		Fixture.World.Get(),
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerCharacter.BP_PlayerCharacter_C"));
	if (!TestNotNull(TEXT("Replacement Character spawned"), Replacement))
	{
		return false;
	}
	Fixture.Controller->Possess(Replacement);
	TestEqual(TEXT("ASC survives replacement"), Fixture.PlayerState->GetProjectRAbilitySystemComponent(), Fixture.ASC);
	TestEqual(TEXT("Replacement becomes Avatar"), Fixture.ASC->GetAvatarActor(), static_cast<AActor*>(Replacement));
	TestTrue(TEXT("Dead persists across replacement"), PRCombatAutomation::HasTag(Fixture.ASC, UPRTagLibrary::GetStateDeadTag()));
	TestEqual(TEXT("Replacement starts death locked"), Replacement->GetCharacterMovement()->MovementMode, MOVE_None);

	FPRReviveRequest Revive = Fixture.MakeRevive();
	Revive.Target = Replacement;
	Revive.HealthFraction = 0.25f;
	Revive.ShieldFraction = 0.5f;
	TestEqual(TEXT("Replacement revive applies"), Fixture.Subsystem->Revive(Revive), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Fractional revive Health"), Fixture.Attributes->GetHealth(), 25.0f);
	TestEqual(TEXT("Fractional revive Shield"), Fixture.Attributes->GetShield(), 25.0f);
	Fixture.PlayerState->ClearAbilityAvatar(Fixture.Character);
	TestEqual(TEXT("Old pawn cannot clear new Avatar"), Fixture.ASC->GetAvatarActor(), static_cast<AActor*>(Replacement));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCombatDamageEffectAssetTest,
	"ProjectR.Combat.Assets.DamageEffect",
	PRCombatAutomation::TestFlags)

bool FPRCombatDamageEffectAssetTest::RunTest(const FString& Parameters)
{
	UClass* EffectClass = LoadClass<UGameplayEffect>(
		nullptr,
		TEXT("/Game/ProjectR/Effects/GE_Damage.GE_Damage_C"));
	if (!TestNotNull(TEXT("Damage effect class exists"), EffectClass))
	{
		return false;
	}

	const UGameplayEffect* Effect = EffectClass->GetDefaultObject<UGameplayEffect>();
	TestEqual(TEXT("Damage effect is Instant"), Effect->DurationPolicy, EGameplayEffectDurationType::Instant);
	TestEqual(TEXT("Damage effect has no modifiers"), Effect->Modifiers.Num(), 0);
	TestEqual(TEXT("Damage effect has one execution"), Effect->Executions.Num(), 1);
	if (Effect->Executions.Num() == 1)
	{
		TestEqual(TEXT("Damage execution class"),
			Effect->Executions[0].CalculationClass.Get(),
			UPRDamageExecutionCalculation::StaticClass());
		TestEqual(TEXT("No scoped modifiers"), Effect->Executions[0].CalculationModifiers.Num(), 0);
		TestEqual(TEXT("No conditional effects"), Effect->Executions[0].ConditionalGameplayEffects.Num(), 0);
	}
	TestEqual(TEXT("No stacking"), Effect->GetStackingType(), EGameplayEffectStackingType::None);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
