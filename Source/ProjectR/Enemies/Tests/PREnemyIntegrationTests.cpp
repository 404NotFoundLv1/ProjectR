// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Enemies/PREnemyAIController.h"
#include "Enemies/PREnemyAttackGameplayAbility.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyCharacter.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Enemies/PREnemyStateTreeNodes.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRTagLibrary.h"

#include "Components/CapsuleComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"

namespace PREnemyIntegrationAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

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

	void Tick(const float DeltaSeconds = 1.0f / 120.0f) const
	{
		++GFrameCounter;
		World->Tick(ELevelTick::LEVELTICK_All, DeltaSeconds);
	}

private:
	TObjectPtr<UWorld> World;
};

APREnemyCharacter* SpawnDormantEnemyFixture(UWorld* World)
{
	UClass* MeleeClass = LoadClass<APREnemyCharacter>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Blueprints/BP_Enemy_MeleeMinion.BP_Enemy_MeleeMinion_C"));
	if (!World || !MeleeClass)
	{
		return nullptr;
	}
	return World->SpawnActor<APREnemyCharacter>(MeleeClass, FVector::ZeroVector, FRotator::ZeroRotator);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyAttackPhasesDamageEventsAndCleanupTest,
	"ProjectR.Enemy.Combat.AttackPhasesDamageEventsAndCleanup",
	PREnemyIntegrationAutomation::TestFlags)

bool FPREnemyAttackPhasesDamageEventsAndCleanupTest::RunTest(const FString& Parameters)
{
	const UPRGameplayAbility* AttackCDO = UPREnemyAttackGameplayAbility::StaticClass()
		->GetDefaultObject<UPREnemyAttackGameplayAbility>();
	TestEqual(TEXT("Enemy attack is ServerTriggered"), AttackCDO->GetActivationPolicy(),
		EPRAbilityActivationPolicy::ServerTriggered);
	TestEqual(TEXT("Enemy attack has no input tag contract"), AttackCDO->GetProjectRAbilityTag().IsValid(), false);
	TestTrue(TEXT("Enemy AI controller exists"), APREnemyAIController::StaticClass()->IsChildOf(AAIController::StaticClass()));
	TestTrue(TEXT("Acquire target StateTree task is a native task"),
		FPRSTTaskAcquireTarget::StaticStruct()->IsChildOf(FStateTreeTaskCommonBase::StaticStruct()));
	TestTrue(TEXT("Enemy state condition is a native condition"),
		FPRSTConditionEnemyState::StaticStruct()->IsChildOf(FStateTreeConditionCommonBase::StaticStruct()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyShieldGuardingLifecycleTest,
	"ProjectR.Enemy.Combat.ShieldGuardingLifecycle",
	PREnemyIntegrationAutomation::TestFlags)

bool FPREnemyShieldGuardingLifecycleTest::RunTest(const FString& Parameters)
{
	PREnemyIntegrationAutomation::FScopedWorld World;
	APREnemyCharacter* Enemy = PREnemyIntegrationAutomation::SpawnDormantEnemyFixture(World.Get());
	TestNotNull(TEXT("Dormant enemy fixture spawns"), Enemy);
	if (!Enemy)
	{
		return false;
	}
	UPRAbilitySystemComponent* ASC = Enemy->GetProjectRAbilitySystemComponent();
	const UPRAttributeSet* Attributes = Enemy->GetAttributeSet();
	UPRCombatSubsystem* Combat = World.Get()->GetSubsystem<UPRCombatSubsystem>();
	TestNotNull(TEXT("Shield enemy ASC exists"), ASC);
	TestNotNull(TEXT("Shield enemy attributes exist"), Attributes);
	TestNotNull(TEXT("Combat subsystem exists"), Combat);
	if (!ASC || !Attributes || !Combat)
	{
		return false;
	}
	ASC->InitAbilityActorInfo(Enemy, Enemy);
	ASC->SetLooseGameplayTagCount(UPRTagLibrary::GetStateAliveTag(), 1);
	ASC->SetLooseGameplayTagCount(UPRTagLibrary::GetStateDeadTag(), 0);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 140.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 140.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxShieldAttribute(), 80.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 80.0f);
	Enemy->BindShieldGuardingLifecycle();

	const FGameplayTag GuardingTag = UPRTagLibrary::GetStateGuardingTag();
	TestEqual(TEXT("Shield begins at 80"), Attributes->GetShield(), 80.0f);
	TestFalse(TEXT("Dormant Shield enemy does not claim Guarding before initialization"),
		ASC->HasMatchingGameplayTag(GuardingTag));
	Enemy->bInitialized = true;
	Enemy->SynchronizeShieldGuardingState();
	TestTrue(TEXT("Guarding is present while Shield is positive"), ASC->HasMatchingGameplayTag(GuardingTag));
	TestEqual(TEXT("Guarding count is exactly one"), ASC->GetTagCount(GuardingTag), 1);
	TestTrue(TEXT("Shield lifecycle delegate is bound exactly to the enemy"),
		ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetShieldAttribute()).IsBoundToObject(Enemy));

	TArray<FPRCombatEvent> Events;
	const FDelegateHandle EventHandle = Combat->OnCombatEvent().AddLambda(
		[&Events, Enemy](const FPRCombatEvent& Event)
		{
			if (Event.Target.Get() == Enemy)
			{
				Events.Add(Event);
			}
		});
	auto ApplyDamage = [Combat, Enemy](const float Damage)
	{
		FPRDamageRequest Request;
		Request.SourceId = TEXT("Automation.ShieldGuarding");
		Request.DamageSource = Enemy;
		Request.Instigator = Enemy;
		Request.Target = Enemy;
		Request.RawDamage = Damage;
		Request.ImpactOrigin = Enemy->GetActorLocation();
		Request.IncomingDirection = FVector::ForwardVector;
		return Combat->ApplyDamage(Request);
	};

	TestEqual(TEXT("Shield-only damage applies"), ApplyDamage(30.0f), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Shield-only damage preserves Health"), Attributes->GetHealth(), 140.0f);
	TestEqual(TEXT("Shield-only damage leaves 50 Shield"), Attributes->GetShield(), 50.0f);
	TestTrue(TEXT("Guarding remains while Shield is positive"), ASC->HasMatchingGameplayTag(GuardingTag));

	TestEqual(TEXT("Spill-over damage applies"), ApplyDamage(70.0f), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Spill-over empties Shield"), Attributes->GetShield(), 0.0f);
	TestEqual(TEXT("Spill-over applies 20 Health damage"), Attributes->GetHealth(), 120.0f);
	TestFalse(TEXT("Guarding clears on Shield break"), ASC->HasMatchingGameplayTag(GuardingTag));
	TestFalse(TEXT("Shield break does not apply Stunned"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()));
	TestEqual(TEXT("Exactly two damage events were published"), Events.Num(), 2);
	if (Events.Num() == 2)
	{
		TestTrue(TEXT("Shield-only event reports absorption"),
			Events[0].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldAbsorbedTag()));
		TestFalse(TEXT("Shield-only event does not report break"),
			Events[0].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag()));
		TestTrue(TEXT("Spill-over event reports absorption"),
			Events[1].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldAbsorbedTag()));
		TestTrue(TEXT("Spill-over event reports one Shield break fact"),
			Events[1].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag()));
		TestTrue(TEXT("Spill-over event reports Health damage"),
			Events[1].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseHealthDamagedTag()));
	}

	TestEqual(TEXT("Post-break Health damage applies"), ApplyDamage(10.0f), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Post-break damage publishes one additional event"), Events.Num(), 3);
	if (Events.Num() == 3)
	{
		TestFalse(TEXT("Post-break damage does not repeat ShieldBroken"),
			Events[2].ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag()));
	}

	ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 25.0f);
	TestTrue(TEXT("Formal Shield restoration re-adds Guarding"), ASC->HasMatchingGameplayTag(GuardingTag));
	TestEqual(TEXT("Restored Guarding count remains exactly one"), ASC->GetTagCount(GuardingTag), 1);
	ASC->SetLooseGameplayTagCount(UPRTagLibrary::GetStateDeadTag(), 1);
	Enemy->HandleCombatLifeStateChanged(true);
	TestFalse(TEXT("Death clears Guarding"), ASC->HasMatchingGameplayTag(GuardingTag));
	TestFalse(TEXT("Death unbinds the Shield lifecycle delegate"),
		ASC->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetShieldAttribute()).IsBoundToObject(Enemy));

	Combat->OnCombatEvent().Remove(EventHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyEliteShieldBreakLifecycleTest,
	"ProjectR.Enemy.Combat.EliteShieldBreakLifecycle",
	PREnemyIntegrationAutomation::TestFlags)

bool FPREnemyEliteShieldBreakLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace PREnemyIntegrationAutomation;
	FScopedWorld World;
	APREnemyCharacter* Enemy = SpawnDormantEnemyFixture(World.Get());
	TestNotNull(TEXT("Elite shield-break fixture spawns"), Enemy);
	if (!Enemy)
	{
		return false;
	}

	UPRAbilitySystemComponent* ASC = Enemy->GetProjectRAbilitySystemComponent();
	const UPRAttributeSet* Attributes = Enemy->GetAttributeSet();
	UPRCombatSubsystem* Combat = World.Get()->GetSubsystem<UPRCombatSubsystem>();
	UClass* StunnedEffectClass = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Stunned.GE_State_Stunned_C"));
	TestNotNull(TEXT("Elite fixture ASC exists"), ASC);
	TestNotNull(TEXT("Elite fixture attributes exist"), Attributes);
	TestNotNull(TEXT("Elite fixture Combat subsystem exists"), Combat);
	TestNotNull(TEXT("Frozen Stunned effect class loads"), StunnedEffectClass);
	if (!ASC || !Attributes || !Combat || !StunnedEffectClass)
	{
		return false;
	}

	UPREnemyPrototypeDataAsset* Prototype = NewObject<UPREnemyPrototypeDataAsset>(GetTransientPackage());
	Prototype->ShieldBreakEffect = StunnedEffectClass;
	Enemy->Prototype = Prototype;
	Enemy->bInitialized = true;
	ASC->InitAbilityActorInfo(Enemy, Enemy);
	ASC->SetLooseGameplayTagCount(UPRTagLibrary::GetStateAliveTag(), 1);
	ASC->SetLooseGameplayTagCount(UPRTagLibrary::GetStateDeadTag(), 0);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 300.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 300.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxShieldAttribute(), 150.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 150.0f);
	Enemy->BindShieldGuardingLifecycle();
	TestTrue(TEXT("Elite begins with one Guarding tag"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	TestEqual(TEXT("Elite Guarding tag count is one"),
		ASC->GetTagCount(UPRTagLibrary::GetStateGuardingTag()), 1);
	TestTrue(TEXT("Elite Stunned lifecycle binds"), Enemy->BindEnemyStunnedLifecycle());
	TestTrue(TEXT("Elite ShieldBreak response lifecycle binds"), Enemy->BindShieldBreakResponseLifecycle());
	TestNotNull(TEXT("Elite fixture Brain exists"), Enemy->EnemyBrain.Get());
	if (!Enemy->EnemyBrain)
	{
		return false;
	}
	Enemy->EnemyBrain->InitializeBrain(Enemy);

	auto ApplyDamage = [Combat, Enemy](const float Damage)
	{
		FPRDamageRequest Request;
		Request.SourceId = TEXT("Automation.EliteShieldBreak");
		Request.DamageSource = Enemy;
		Request.Instigator = Enemy;
		Request.Target = Enemy;
		Request.RawDamage = Damage;
		Request.ImpactOrigin = Enemy->GetActorLocation();
		Request.IncomingDirection = FVector::ForwardVector;
		return Combat->ApplyDamage(Request);
	};

	TestEqual(TEXT("Initial non-fatal ShieldBreak applies damage"), ApplyDamage(150.0f), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Initial ShieldBreak consumes all Shield"), Attributes->GetShield(), 0.0f);
	TestEqual(TEXT("Initial ShieldBreak preserves Health"), Attributes->GetHealth(), 300.0f);
	TestFalse(TEXT("Initial ShieldBreak removes Guarding"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	TestTrue(TEXT("Initial ShieldBreak response is consumed"), Enemy->bShieldBreakResponseConsumed);
	TestTrue(TEXT("Initial non-fatal ShieldBreak applies frozen Stunned"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()));
	TestEqual(TEXT("Initial Stunned tag count is one"),
		ASC->GetTagCount(UPRTagLibrary::GetStateStunnedTag()), 1);
	TestEqual(TEXT("Initial ShieldBreak forces Staggered Brain state"),
		Enemy->EnemyBrain->GetRuntimeState().BrainState, EPREnemyBrainState::Staggered);
	TestEqual(TEXT("Initial ShieldBreak cancels active attack phase"),
		Enemy->EnemyBrain->GetRuntimeState().AttackPhase, EPREnemyAttackPhase::None);

	ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 150.0f);
	TestTrue(TEXT("Formal Shield restoration re-adds Guarding after first break"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateGuardingTag()));
	TestEqual(TEXT("Second ShieldBreak applies damage"), ApplyDamage(150.0f), EPRCombatRequestStatus::Applied);
	TestEqual(TEXT("Second ShieldBreak does not stack Stunned"),
		ASC->GetTagCount(UPRTagLibrary::GetStateStunnedTag()), 1);
	for (int32 TickIndex = 0; TickIndex < 100; ++TickIndex)
	{
		World.Tick(0.01f);
	}
	TestFalse(TEXT("Frozen Stunned expires before fatal-break fixture"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()));
	TestNotEqual(TEXT("Frozen Stunned recovers through one controlled reevaluation"),
		Enemy->EnemyBrain->GetRuntimeState().BrainState, EPREnemyBrainState::Staggered);

	Enemy->ClearShieldBreakResponseLifecycle();
	Enemy->ClearEnemyStunnedLifecycle();
	Enemy->ClearShieldGuardingLifecycle();
	Enemy->bInitialized = false;
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), 150.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 20.0f);
	Enemy->bShieldBreakResponseConsumed = false;
	Enemy->bInitialized = true;
	Enemy->BindShieldGuardingLifecycle();
	Enemy->BindEnemyStunnedLifecycle();
	Enemy->BindShieldBreakResponseLifecycle();
	TestEqual(TEXT("Fatal ShieldBreak applies damage"), ApplyDamage(170.0f), EPRCombatRequestStatus::Applied);
	TestTrue(TEXT("Fatal ShieldBreak consumes response without applying Stunned"), Enemy->bShieldBreakResponseConsumed);
	TestFalse(TEXT("Fatal ShieldBreak never applies Stunned"),
		ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()));

	Enemy->ClearShieldBreakResponseLifecycle();
	Enemy->ClearEnemyStunnedLifecycle();
	Enemy->ClearShieldGuardingLifecycle();
	Enemy->EnemyBrain->StopBrain();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPREnemyDeadPawnCollisionLifecycleTest,
	"ProjectR.Enemy.Lifecycle.DeadPawnCollisionLifecycle",
	PREnemyIntegrationAutomation::TestFlags)

bool FPREnemyDeadPawnCollisionLifecycleTest::RunTest(const FString& Parameters)
{
	PREnemyIntegrationAutomation::FScopedWorld World;
	APREnemyCharacter* Enemy = PREnemyIntegrationAutomation::SpawnDormantEnemyFixture(World.Get());
	if (!TestNotNull(TEXT("Enemy fixture spawns for dead-collision lifecycle"), Enemy))
	{
		return false;
	}

	UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
	if (!TestNotNull(TEXT("Enemy has a collision capsule"), Capsule))
	{
		return false;
	}
	const ECollisionResponse InitialPawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
	TestNotEqual(TEXT("Living enemy initially participates in Pawn collision"),
		InitialPawnResponse, ECR_Ignore);

	Enemy->HandleCombatLifeStateChanged(true);
	TestEqual(TEXT("Dead enemy no longer blocks Pawn movement"),
		Capsule->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);

	Enemy->HandleCombatLifeStateChanged(false);
	TestEqual(TEXT("Revived enemy restores its authored Pawn collision response"),
		Capsule->GetCollisionResponseToChannel(ECC_Pawn), InitialPawnResponse);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
