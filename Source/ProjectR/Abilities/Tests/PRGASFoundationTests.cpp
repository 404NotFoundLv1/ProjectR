// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Characters/PRPlayerCharacter.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

namespace PRGASAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

struct FExpectedAttribute
{
	FGameplayAttribute Attribute;
	const TCHAR* Name;
	float DefaultValue;
};

TArray<FExpectedAttribute> GetExpectedAttributes()
{
	return {
		{UPRAttributeSet::GetMaxHealthAttribute(), TEXT("MaxHealth"), 100.0f},
		{UPRAttributeSet::GetHealthAttribute(), TEXT("Health"), 100.0f},
		{UPRAttributeSet::GetMaxShieldAttribute(), TEXT("MaxShield"), 50.0f},
		{UPRAttributeSet::GetShieldAttribute(), TEXT("Shield"), 50.0f},
		{UPRAttributeSet::GetMaxEnergyAttribute(), TEXT("MaxEnergy"), 100.0f},
		{UPRAttributeSet::GetEnergyAttribute(), TEXT("Energy"), 100.0f},
		{UPRAttributeSet::GetAttackPowerAttribute(), TEXT("AttackPower"), 10.0f},
		{UPRAttributeSet::GetMoveSpeedAttribute(), TEXT("MoveSpeed"), 600.0f},
		{UPRAttributeSet::GetCritChanceAttribute(), TEXT("CritChance"), 0.05f},
		{UPRAttributeSet::GetPermissionAttribute(), TEXT("Permission"), 0.0f},
		{UPRAttributeSet::GetResonanceAttribute(), TEXT("Resonance"), 0.0f}};
}

int32 GetReflectedArrayNum(const UObject* Object, const FName PropertyName)
{
	const FArrayProperty* Property = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
	if (!Property)
	{
		return INDEX_NONE;
	}
	const void* ArrayAddress = Property->ContainerPtrToValuePtr<void>(Object);
	return FScriptArrayHelper(Property, ArrayAddress).Num();
}

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
} // namespace PRGASAutomation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRGASAttributeSchemaTest,
	"ProjectR.GAS.Attributes.SchemaAndReplication",
	PRGASAutomation::TestFlags)

bool FPRGASAttributeSchemaTest::RunTest(const FString& Parameters)
{
	UPRAttributeSet* AttributeSet = NewObject<UPRAttributeSet>();
	const TArray<PRGASAutomation::FExpectedAttribute> Expected =
		PRGASAutomation::GetExpectedAttributes();
	TestEqual(TEXT("Attribute count"), Expected.Num(), 11);

	TArray<FLifetimeProperty> LifetimeProperties;
	AttributeSet->GetLifetimeReplicatedProps(LifetimeProperties);
	for (const PRGASAutomation::FExpectedAttribute& Entry : Expected)
	{
		TestEqual(*FString::Printf(TEXT("Default %s"), Entry.Name),
			Entry.Attribute.GetNumericValue(AttributeSet), Entry.DefaultValue);

		FProperty* Property = Entry.Attribute.GetUProperty();
		if (!TestNotNull(*FString::Printf(TEXT("Property %s"), Entry.Name), Property))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s is replicated"), Entry.Name),
			Property->HasAnyPropertyFlags(CPF_Net));
		TestEqual(*FString::Printf(TEXT("%s RepNotify"), Entry.Name),
			Property->RepNotifyFunc,
			FName(*FString::Printf(TEXT("OnRep_%s"), Entry.Name)));

		const FLifetimeProperty* Lifetime = LifetimeProperties.FindByPredicate(
			[Property](const FLifetimeProperty& Candidate)
			{
				return Candidate.RepIndex == Property->RepIndex;
			});
		if (TestNotNull(*FString::Printf(TEXT("Lifetime %s"), Entry.Name), Lifetime))
		{
			TestEqual(TEXT("Replication condition"), Lifetime->Condition, COND_None);
			TestEqual(TEXT("RepNotify condition"),
				Lifetime->RepNotifyCondition, REPNOTIFY_Always);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRGASAttributeClampTest,
	"ProjectR.GAS.Attributes.ClampAndRatio",
	PRGASAutomation::TestFlags)

bool FPRGASAttributeClampTest::RunTest(const FString& Parameters)
{
	AddExpectedError(
		TEXT("No GameplayCueNotifyPaths were specified"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	PRGASAutomation::FScopedTestWorld TestWorld;
	APRPlayerState* PlayerState = PRGASAutomation::SpawnBlueprintActor<APRPlayerState>(
		TestWorld.Get(),
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerState.BP_PlayerState_C"));
	if (!TestNotNull(TEXT("PlayerState spawned for clamp test"), PlayerState))
	{
		return false;
	}
	UPRAbilitySystemComponent* ASC = PlayerState->GetProjectRAbilitySystemComponent();
	const UPRAttributeSet* AttributeSet = PlayerState->GetAttributeSet();

	ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 50.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 200.0f);
	TestEqual(TEXT("Max increase preserves Health ratio"), AttributeSet->GetHealth(), 100.0f);

	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMaxHealthAttribute(), 50.0f);
	TestEqual(TEXT("Max decrease preserves Health ratio"), AttributeSet->GetHealth(), 25.0f);

	ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 500.0f);
	TestEqual(TEXT("Health clamps to MaxHealth"), AttributeSet->GetHealth(), 50.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMoveSpeedAttribute(), 5000.0f);
	TestEqual(TEXT("MoveSpeed clamps high"), AttributeSet->GetMoveSpeed(), 2000.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMoveSpeedAttribute(), -1.0f);
	TestEqual(TEXT("MoveSpeed clamps low"), AttributeSet->GetMoveSpeed(), 0.0f);
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetCritChanceAttribute(), 2.0f);
	TestEqual(TEXT("CritChance clamps"), AttributeSet->GetCritChance(), 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRGASDefaultEffectAssetTest,
	"ProjectR.GAS.Assets.DefaultEffect",
	PRGASAutomation::TestFlags)

bool FPRGASDefaultEffectAssetTest::RunTest(const FString& Parameters)
{
	UClass* EffectClass = LoadClass<UGameplayEffect>(
		nullptr,
		TEXT("/Game/ProjectR/Effects/GE_DefaultAttributes.GE_DefaultAttributes_C"));
	if (!TestNotNull(TEXT("Default attributes effect class exists"), EffectClass))
	{
		return false;
	}

	const UGameplayEffect* Effect = EffectClass->GetDefaultObject<UGameplayEffect>();
	TestEqual(TEXT("Duration is Instant"),
		Effect->DurationPolicy, EGameplayEffectDurationType::Instant);
	TestEqual(TEXT("Modifier count"), Effect->Modifiers.Num(), 11);

	const TArray<PRGASAutomation::FExpectedAttribute> Expected = {
		{UPRAttributeSet::GetMaxHealthAttribute(), TEXT("MaxHealth"), 100.0f},
		{UPRAttributeSet::GetMaxShieldAttribute(), TEXT("MaxShield"), 50.0f},
		{UPRAttributeSet::GetMaxEnergyAttribute(), TEXT("MaxEnergy"), 100.0f},
		{UPRAttributeSet::GetHealthAttribute(), TEXT("Health"), 100.0f},
		{UPRAttributeSet::GetShieldAttribute(), TEXT("Shield"), 50.0f},
		{UPRAttributeSet::GetEnergyAttribute(), TEXT("Energy"), 100.0f},
		{UPRAttributeSet::GetAttackPowerAttribute(), TEXT("AttackPower"), 10.0f},
		{UPRAttributeSet::GetMoveSpeedAttribute(), TEXT("MoveSpeed"), 600.0f},
		{UPRAttributeSet::GetCritChanceAttribute(), TEXT("CritChance"), 0.05f},
		{UPRAttributeSet::GetPermissionAttribute(), TEXT("Permission"), 0.0f},
		{UPRAttributeSet::GetResonanceAttribute(), TEXT("Resonance"), 0.0f}};

	for (int32 Index = 0; Index < FMath::Min(Effect->Modifiers.Num(), Expected.Num()); ++Index)
	{
		const FGameplayModifierInfo& Modifier = Effect->Modifiers[Index];
		TestEqual(*FString::Printf(TEXT("Modifier %d attribute"), Index),
			Modifier.Attribute, Expected[Index].Attribute);
		TestEqual(*FString::Printf(TEXT("Modifier %d operation"), Index),
			Modifier.ModifierOp, EGameplayModOp::Override);
		float Magnitude = 0.0f;
		TestTrue(*FString::Printf(TEXT("Modifier %d static magnitude"), Index),
			Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, Magnitude));
		TestEqual(*FString::Printf(TEXT("Modifier %d value"), Index),
			Magnitude, Expected[Index].DefaultValue);
	}

	TestEqual(TEXT("No executions"), Effect->Executions.Num(), 0);
	TestEqual(TEXT("No granted abilities"),
		PRGASAutomation::GetReflectedArrayNum(Effect, TEXT("GrantedAbilities")), 0);
	TestEqual(TEXT("No GE components"),
		PRGASAutomation::GetReflectedArrayNum(Effect, TEXT("GEComponents")), 0);
	TestEqual(TEXT("No stacking"),
		Effect->GetStackingType(), EGameplayEffectStackingType::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRGASLifecycleTest,
	"ProjectR.GAS.Lifecycle.OwnerAvatarDefaultsAndReplacement",
	PRGASAutomation::TestFlags)

bool FPRGASLifecycleTest::RunTest(const FString& Parameters)
{
	PRGASAutomation::FScopedTestWorld TestWorld;
	APRPlayerState* PlayerState = PRGASAutomation::SpawnBlueprintActor<APRPlayerState>(
		TestWorld.Get(),
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerState.BP_PlayerState_C"));
	APRPlayerCharacter* FirstCharacter = PRGASAutomation::SpawnBlueprintActor<APRPlayerCharacter>(
		TestWorld.Get(),
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerCharacter.BP_PlayerCharacter_C"));
	APRPlayerController* PlayerController = PRGASAutomation::SpawnBlueprintActor<APRPlayerController>(
		TestWorld.Get(),
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerController.BP_PlayerController_C"));
	if (!TestNotNull(TEXT("PlayerState spawned"), PlayerState)
		|| !TestNotNull(TEXT("First Character spawned"), FirstCharacter)
		|| !TestNotNull(TEXT("PlayerController spawned"), PlayerController))
	{
		return false;
	}
	PlayerController->SetPlayerState(PlayerState);

	UPRAbilitySystemComponent* ASC = PlayerState->GetProjectRAbilitySystemComponent();
	TestNotNull(TEXT("ProjectR ASC exists"), ASC);
	TestEqual(TEXT("ASC replication mode"),
		ASC->ReplicationMode, EGameplayEffectReplicationMode::Mixed);
	TestTrue(TEXT("AttributeSet is registered"),
		ASC->GetSpawnedAttributes().Contains(PlayerState->GetAttributeSet()));
	ASC->InitAbilityActorInfo(PlayerState, FirstCharacter);
	AddExpectedError(TEXT("invalid default GameplayEffect Spec"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Invalid Spec is rejected"),
		PlayerState->ApplyDefaultAttributesSpec(FGameplayEffectSpecHandle()));
	TestFalse(TEXT("Invalid Spec does not consume retry"),
		PlayerState->bDefaultAttributesApplied);
	PlayerController->Possess(FirstCharacter);
	TestEqual(TEXT("Controller possesses first Character"),
		PlayerController->GetPawn().Get(), static_cast<APawn*>(FirstCharacter));
	TestTrue(TEXT("First possession applies defaults"), PlayerState->bDefaultAttributesApplied);
	TestEqual(TEXT("ASC Owner is PlayerState"),
		ASC->GetOwnerActor(), static_cast<AActor*>(PlayerState));
	TestEqual(TEXT("ASC Avatar is first Character"),
		ASC->GetAvatarActor(), static_cast<AActor*>(FirstCharacter));
	TestEqual(TEXT("Initial movement speed"),
		FirstCharacter->GetCharacterMovement()->MaxWalkSpeed, 600.0f);

	ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), 25.0f);
	TestTrue(TEXT("Repeated initialization succeeds"),
		PlayerState->InitializeAbilitySystemForAvatar(FirstCharacter));
	TestEqual(TEXT("Repeated initialization does not reset Health"),
		PlayerState->GetAttributeSet()->GetHealth(), 25.0f);

	int32 ChangeCount = 0;
	FPRAttributeChange LastChange;
	const FDelegateHandle ChangeHandle = PlayerState->OnAttributeChanged().AddLambda(
		[&ChangeCount, &LastChange](const FPRAttributeChange& Change)
		{
			++ChangeCount;
			LastChange = Change;
		});
	ASC->SetNumericAttributeBase(UPRAttributeSet::GetMoveSpeedAttribute(), 900.0f);
	TestEqual(TEXT("Unified delegate fires once"), ChangeCount, 1);
	TestEqual(TEXT("Unified delegate attribute"),
		LastChange.Attribute, UPRAttributeSet::GetMoveSpeedAttribute());
	TestEqual(TEXT("Unified delegate old value"), LastChange.OldValue, 600.0f);
	TestEqual(TEXT("Unified delegate new value"), LastChange.NewValue, 900.0f);
	TestEqual(TEXT("Character movement follows MoveSpeed"),
		FirstCharacter->GetCharacterMovement()->MaxWalkSpeed, 900.0f);
	PlayerState->OnAttributeChanged().Remove(ChangeHandle);

	APRPlayerCharacter* SecondCharacter = PRGASAutomation::SpawnBlueprintActor<APRPlayerCharacter>(
		TestWorld.Get(),
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerCharacter.BP_PlayerCharacter_C"));
	if (!TestNotNull(TEXT("Second Character spawned"), SecondCharacter))
	{
		return false;
	}
	PlayerController->Possess(SecondCharacter);
	TestEqual(TEXT("Controller possesses replacement"),
		PlayerController->GetPawn().Get(), static_cast<APawn*>(SecondCharacter));
	TestEqual(TEXT("ASC survives replacement"),
		PlayerState->GetProjectRAbilitySystemComponent(), ASC);
	TestEqual(TEXT("Avatar changes to replacement"),
		ASC->GetAvatarActor(), static_cast<AActor*>(SecondCharacter));
	TestEqual(TEXT("Attributes survive replacement"),
		PlayerState->GetAttributeSet()->GetHealth(), 25.0f);
	TestEqual(TEXT("Replacement immediately receives MoveSpeed"),
		SecondCharacter->GetCharacterMovement()->MaxWalkSpeed, 900.0f);

	PlayerState->ClearAbilityAvatar(FirstCharacter);
	TestEqual(TEXT("Old pawn cannot clear replacement ActorInfo"),
		ASC->GetAvatarActor(), static_cast<AActor*>(SecondCharacter));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
