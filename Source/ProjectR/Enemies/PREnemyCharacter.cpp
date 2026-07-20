// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyCharacter.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAttributeSet.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/PREnemyAIController.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyPlaneMovementComponent.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "GameplayEffect.h"

APREnemyCharacter::APREnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPREnemyPlaneMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	ProjectRAbilitySystemComponent = CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ProjectRAbilitySystemComponent->SetIsReplicated(true);
	ProjectRAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	ProjectRAttributeSet = CreateDefaultSubobject<UPRAttributeSet>(TEXT("AttributeSet"));
	ProjectRAbilitySystemComponent->AddAttributeSetSubobject(ProjectRAttributeSet.Get());
	EnemyBrain = CreateDefaultSubobject<UPREnemyBrainComponent>(TEXT("EnemyBrain"));
	AIControllerClass = APREnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAbilitySystemComponent* APREnemyCharacter::GetAbilitySystemComponent() const
{
	return ProjectRAbilitySystemComponent;
}

FName APREnemyCharacter::GetCombatantId() const
{
	FString PrototypeName = TEXT("Dormant");
	if (Prototype)
	{
		PrototypeName = Prototype->EnemyId.IsNone() ? Prototype->PrototypeTag.ToString() : Prototype->EnemyId.ToString();
	}
	return FName(FString::Printf(TEXT("Enemy_%s_%s"), *PrototypeName, *SpawnId.ToString(EGuidFormats::Digits)));
}

TSubclassOf<UGameplayEffect> APREnemyCharacter::GetDamageEffectClass() const
{
	return DamageEffect;
}

void APREnemyCharacter::HandleCombatHitReaction()
{
	if (bInitialized && EnemyBrain)
	{
		EnemyBrain->SetBrainState(EPREnemyBrainState::Staggered);
	}
}

void APREnemyCharacter::HandleCombatLifeStateChanged(const bool bIsDead)
{
	if (bIsDead)
	{
		ClearShieldGuardingLifecycle();
		if (UPREnemyPlaneMovementComponent* Movement = GetEnemyPlaneMovement())
		{
			Movement->StopEnemyMovement();
		}
		if (EnemyBrain)
		{
			EnemyBrain->StopBrain();
		}
		if (APREnemyAIController* EnemyController = Cast<APREnemyAIController>(GetController()))
		{
			EnemyController->StopEnemyStateTree();
		}
	}
}

FName APREnemyCharacter::GetAbilityTargetId() const
{
	return GetCombatantId();
}

FVector APREnemyCharacter::GetAbilityTargetPoint() const
{
	return GetActorLocation();
}

EPRAbilityTargetMobility APREnemyCharacter::GetAbilityTargetMobility() const
{
	return Prototype ? Prototype->Mobility : EPRAbilityTargetMobility::Light;
}

bool APREnemyCharacter::CanBeTargetedByAbility(const FGameplayTag AbilityTag) const
{
	return bInitialized && AbilityTag.IsValid() && AbilityTag.ToString().StartsWith(TEXT("Skill."))
		&& ProjectRAbilitySystemComponent
		&& !ProjectRAbilitySystemComponent->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag());
}

UPRAbilitySystemComponent* APREnemyCharacter::GetProjectRAbilitySystemComponent() const
{
	return ProjectRAbilitySystemComponent;
}

const UPRAttributeSet* APREnemyCharacter::GetAttributeSet() const
{
	return ProjectRAttributeSet;
}

UPREnemyPlaneMovementComponent* APREnemyCharacter::GetEnemyPlaneMovement() const
{
	return Cast<UPREnemyPlaneMovementComponent>(GetCharacterMovement());
}

UPREnemyBrainComponent* APREnemyCharacter::GetEnemyBrain() const
{
	return EnemyBrain;
}

const UPREnemyPrototypeDataAsset* APREnemyCharacter::GetEnemyPrototype() const
{
	return Prototype;
}

const TArray<TObjectPtr<UPREnemyAttackDataAsset>>& APREnemyCharacter::GetAttackDefinitions() const
{
	static const TArray<TObjectPtr<UPREnemyAttackDataAsset>> Empty;
	return Prototype ? Prototype->AttackDefinitions : Empty;
}

UStateTree* APREnemyCharacter::GetEnemyStateTree() const
{
	return EnemyStateTree;
}

bool APREnemyCharacter::IsEnemyDead() const
{
	return ProjectRAbilitySystemComponent
		&& ProjectRAbilitySystemComponent->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag());
}

bool APREnemyCharacter::IsEnemyInitialized() const
{
	return bInitialized;
}

FGuid APREnemyCharacter::GetSpawnId() const
{
	return SpawnId;
}

void APREnemyCharacter::ConfigureSpawn(
	UPREnemyPrototypeDataAsset* InPrototype,
	const FGuid InSpawnId,
	const float InPlaneY)
{
	Prototype = InPrototype;
	SpawnId = InSpawnId;
	PlaneY = InPlaneY;
}

void APREnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	TryInitializeEnemy();
}

void APREnemyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	TryInitializeEnemy();
}

void APREnemyCharacter::TryInitializeEnemy()
{
	if (!HasAuthority() || bInitialized || !Prototype || !SpawnId.IsValid() || !ProjectRAbilitySystemComponent || !EnemyBrain
		|| !DefaultAttributesEffect || !Prototype->InitialAbilitySet || !EnemyStateTree)
	{
		return;
	}
	ProjectRAbilitySystemComponent->InitAbilityActorInfo(this, this);
	if (ProjectRAbilitySystemComponent->GetOwnerActor() != this || ProjectRAbilitySystemComponent->GetAvatarActor() != this)
	{
		return;
	}
	if (UPREnemyPlaneMovementComponent* Movement = GetEnemyPlaneMovement())
	{
		Movement->SetPlaneY(PlaneY);
	}
	const FPREnemyAttributeDefaults& Attributes = Prototype->Attributes;
	if (!bDefaultAttributesApplied)
	{
		FGameplayEffectContextHandle EffectContext = ProjectRAbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle DefaultSpec = ProjectRAbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1.0f, EffectContext);
		if (!DefaultSpec.IsValid() || !DefaultSpec.Data.IsValid())
		{
			return;
		}
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataHealthTag(), Attributes.Health);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataMaxHealthTag(), Attributes.MaxHealth);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataShieldTag(), Attributes.Shield);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataMaxShieldTag(), Attributes.MaxShield);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataEnergyTag(), Attributes.Energy);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataMaxEnergyTag(), Attributes.MaxEnergy);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataAttackPowerTag(), Attributes.AttackPower);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataMoveSpeedTag(), Attributes.MoveSpeed);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataCritChanceTag(), Attributes.CritChance);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataPermissionTag(), Attributes.Permission);
		DefaultSpec.Data->SetSetByCallerMagnitude(UPRTagLibrary::GetEnemyDataResonanceTag(), Attributes.Resonance);
		if (!ProjectRAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*DefaultSpec.Data.Get()).WasSuccessfullyApplied())
		{
			return;
		}
		bDefaultAttributesApplied = true;
	}
	if (UPREnemyPlaneMovementComponent* Movement = GetEnemyPlaneMovement())
	{
		Movement->MaxWalkSpeed = Attributes.MoveSpeed;
	}
	ProjectRAbilitySystemComponent->SetLooseGameplayTagCount(UPRTagLibrary::GetStateAliveTag(), 1);
	ProjectRAbilitySystemComponent->SetLooseGameplayTagCount(UPRTagLibrary::GetStateDeadTag(), 0);
	if (!InitialAbilityGrant.AbilitySet.IsValid()
		&& ProjectRAbilitySystemComponent->GrantAbilitySet(Prototype->InitialAbilitySet, EPRAbilitySetGrantMode::AllEntries, InitialAbilityGrant) != EPRAbilitySetOperationStatus::Applied)
	{
		return;
	}
	// The StateTree can immediately enter an attack task.  Mark the actor ready
	// only after all preceding lifecycle steps have succeeded, but before that
	// task has any chance to activate the ServerTriggered ability.
	bInitialized = true;
	BindShieldGuardingLifecycle();
	EnemyBrain->InitializeBrain(this);
	APREnemyAIController* EnemyController = Cast<APREnemyAIController>(GetController());
	if (!EnemyController || !EnemyController->ConfigureAndStartStateTree(EnemyStateTree))
	{
		EnemyBrain->StopBrain();
		ClearShieldGuardingLifecycle();
		bInitialized = false;
		return;
	}
}

void APREnemyCharacter::BindShieldGuardingLifecycle()
{
	if (!ProjectRAbilitySystemComponent || !ProjectRAttributeSet)
	{
		return;
	}
	if (!ShieldAttributeDelegateHandle.IsValid())
	{
		ShieldAttributeDelegateHandle = ProjectRAbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetShieldAttribute())
			.AddUObject(this, &APREnemyCharacter::HandleShieldAttributeChanged);
	}
	SynchronizeShieldGuardingState();
}

void APREnemyCharacter::ClearShieldGuardingLifecycle()
{
	if (ProjectRAbilitySystemComponent)
	{
		if (ShieldAttributeDelegateHandle.IsValid())
		{
			ProjectRAbilitySystemComponent
				->GetGameplayAttributeValueChangeDelegate(UPRAttributeSet::GetShieldAttribute())
				.Remove(ShieldAttributeDelegateHandle);
		}
		ProjectRAbilitySystemComponent->SetLooseGameplayTagCount(UPRTagLibrary::GetStateGuardingTag(), 0);
	}
	ShieldAttributeDelegateHandle.Reset();
}

void APREnemyCharacter::SynchronizeShieldGuardingState()
{
	if (!ProjectRAbilitySystemComponent || !ProjectRAttributeSet || !ShieldAttributeDelegateHandle.IsValid())
	{
		return;
	}
	const bool bAlive = bInitialized
		&& ProjectRAbilitySystemComponent->HasMatchingGameplayTag(UPRTagLibrary::GetStateAliveTag())
		&& !ProjectRAbilitySystemComponent->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag());
	const int32 GuardingCount = bAlive && ProjectRAttributeSet->GetShield() > UE_SMALL_NUMBER ? 1 : 0;
	ProjectRAbilitySystemComponent->SetLooseGameplayTagCount(UPRTagLibrary::GetStateGuardingTag(), GuardingCount);
}

void APREnemyCharacter::HandleShieldAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.Attribute == UPRAttributeSet::GetShieldAttribute())
	{
		SynchronizeShieldGuardingState();
	}
}

void APREnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearShieldGuardingLifecycle();
	if (EnemyBrain)
	{
		EnemyBrain->StopBrain();
	}
	if (ProjectRAbilitySystemComponent && InitialAbilityGrant.AbilitySet.IsValid())
	{
		ProjectRAbilitySystemComponent->RemoveAbilitySet(InitialAbilityGrant);
	}
	Super::EndPlay(EndPlayReason);
}
