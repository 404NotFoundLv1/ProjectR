// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyCharacter.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAttributeSet.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Core/PRTagLibrary.h"
#include "Enemies/PREnemyAIController.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyAttackGameplayAbility.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Enemies/PREnemyPlaneMovementComponent.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "ProjectR.h"

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
	EnemyIdentityLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("EnemyIdentityLabel"));
	EnemyIdentityLabel->SetupAttachment(GetRootComponent());
	EnemyIdentityLabel->SetHorizontalAlignment(EHTA_Center);
	EnemyIdentityLabel->SetWorldSize(22.0f);
	EnemyIdentityLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	EnemyStatusLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("EnemyStatusLabel"));
	EnemyStatusLabel->SetupAttachment(GetRootComponent());
	EnemyStatusLabel->SetHorizontalAlignment(EHTA_Center);
	EnemyStatusLabel->SetWorldSize(18.0f);
	EnemyStatusLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
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
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			if (!bDeadPawnCollisionSuppressed)
			{
				AuthoredPawnCollisionResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
				bDeadPawnCollisionSuppressed = true;
			}
			Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
		ClearShieldBreakResponseLifecycle();
		ClearEnemyStunnedLifecycle();
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
		return;
	}

	if (bDeadPawnCollisionSuppressed)
	{
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCollisionResponseToChannel(ECC_Pawn, AuthoredPawnCollisionResponse);
		}
		bDeadPawnCollisionSuppressed = false;
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
	RefreshPresentationLabels();
}

void APREnemyCharacter::BeginAttackPresentation(const UPREnemyAttackDataAsset* Attack)
{
	if (!Attack || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	EndAttackPresentation();
	bPresentationAttackActive = true;
	if (UNiagaraSystem* VFX = Attack->VFX.Get())
	{
		ActiveAttackVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(VFX, GetRootComponent(), NAME_None, FVector::ZeroVector,
			FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, true, ENCPoolMethod::None, true);
	}
	if (USoundBase* SFX = Attack->SFX.Get(); SFX && SFX->GetDuration() > KINDA_SMALL_NUMBER)
	{
		ActiveAttackSFX = UGameplayStatics::SpawnSoundAttached(SFX, GetRootComponent(), NAME_None, FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset, true, 0.35f, 1.0f, 0.0f, nullptr, nullptr, true);
	}
	RefreshPresentationLabels();
}

void APREnemyCharacter::EndAttackPresentation()
{
	bPresentationAttackActive = false;
	if (UNiagaraComponent* VFX = ActiveAttackVFX.Get())
	{
		VFX->DeactivateImmediate();
		VFX->DestroyComponent();
	}
	if (UAudioComponent* SFX = ActiveAttackSFX.Get())
	{
		SFX->Stop();
		SFX->DestroyComponent();
	}
	ActiveAttackVFX.Reset();
	ActiveAttackSFX.Reset();
	RefreshPresentationLabels();
}

void APREnemyCharacter::ShowShieldBreakPresentation()
{
	if (GetNetMode() == NM_DedicatedServer || !GetWorld())
	{
		return;
	}
	bPresentationBreakActive = true;
	RefreshPresentationLabels();
	GetWorld()->GetTimerManager().SetTimer(PresentationBreakTimer, this, &APREnemyCharacter::ClearShieldBreakPresentation, 0.25f, false);
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
	if (!BindEnemyStunnedLifecycle() || !BindShieldBreakResponseLifecycle())
	{
		ClearShieldBreakResponseLifecycle();
		ClearEnemyStunnedLifecycle();
		ClearShieldGuardingLifecycle();
		bInitialized = false;
		return;
	}
	EnemyBrain->InitializeBrain(this);
	APREnemyAIController* EnemyController = Cast<APREnemyAIController>(GetController());
	if (!EnemyController || !EnemyController->ConfigureAndStartStateTree(EnemyStateTree))
	{
		EnemyBrain->StopBrain();
		ClearShieldBreakResponseLifecycle();
		ClearEnemyStunnedLifecycle();
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

bool APREnemyCharacter::BindEnemyStunnedLifecycle()
{
	if (!ProjectRAbilitySystemComponent)
	{
		return false;
	}
	if (!StunnedTagDelegateHandle.IsValid())
	{
		StunnedTagDelegateHandle = ProjectRAbilitySystemComponent
			->RegisterGameplayTagEvent(UPRTagLibrary::GetStateStunnedTag(), EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &APREnemyCharacter::HandleEnemyStunnedTagChanged);
	}
	HandleEnemyStunnedTagChanged(UPRTagLibrary::GetStateStunnedTag(),
		ProjectRAbilitySystemComponent->GetTagCount(UPRTagLibrary::GetStateStunnedTag()));
	return StunnedTagDelegateHandle.IsValid();
}

void APREnemyCharacter::ClearEnemyStunnedLifecycle()
{
	if (ProjectRAbilitySystemComponent && StunnedTagDelegateHandle.IsValid())
	{
		ProjectRAbilitySystemComponent
			->RegisterGameplayTagEvent(UPRTagLibrary::GetStateStunnedTag(), EGameplayTagEventType::NewOrRemoved)
			.Remove(StunnedTagDelegateHandle);
	}
	StunnedTagDelegateHandle.Reset();
	bEnemyStunned = false;
}

bool APREnemyCharacter::BindShieldBreakResponseLifecycle()
{
	if (!Prototype || !Prototype->ShieldBreakEffect)
	{
		return true;
	}
	UPRCombatSubsystem* Combat = GetWorld() ? GetWorld()->GetSubsystem<UPRCombatSubsystem>() : nullptr;
	if (!Combat)
	{
		return false;
	}
	if (!ShieldBreakCombatEventDelegateHandle.IsValid())
	{
		ShieldBreakCombatEventDelegateHandle = Combat->OnCombatEvent().AddUObject(this, &APREnemyCharacter::HandleShieldBreakCombatEvent);
		BoundCombatSubsystem = Combat;
	}
	return ShieldBreakCombatEventDelegateHandle.IsValid();
}

void APREnemyCharacter::ClearShieldBreakResponseLifecycle()
{
	if (UPRCombatSubsystem* Combat = BoundCombatSubsystem.Get(); Combat && ShieldBreakCombatEventDelegateHandle.IsValid())
	{
		Combat->OnCombatEvent().Remove(ShieldBreakCombatEventDelegateHandle);
	}
	ShieldBreakCombatEventDelegateHandle.Reset();
	BoundCombatSubsystem.Reset();
}

void APREnemyCharacter::HandleEnemyStunnedTagChanged(const FGameplayTag Tag, const int32 NewCount)
{
	if (Tag != UPRTagLibrary::GetStateStunnedTag())
	{
		return;
	}
	const bool bNowStunned = NewCount > 0;
	if (bNowStunned == bEnemyStunned)
	{
		return;
	}
	bEnemyStunned = bNowStunned;
	RefreshPresentationLabels();
	if (!bInitialized || IsEnemyDead() || !EnemyBrain)
	{
		return;
	}
	if (bEnemyStunned)
	{
		EnemyBrain->EnterStunnedState();
		CancelActiveEnemyAttackAbilities();
		return;
	}
	EnemyBrain->ExitStunnedState();
}

void APREnemyCharacter::HandleShieldBreakCombatEvent(const FPRCombatEvent& Event)
{
	if (bShieldBreakResponseConsumed || !HasAuthority() || !bInitialized || IsEnemyDead() || !Prototype
		|| !Prototype->ShieldBreakEffect || Event.Target.Get() != this || Event.TargetId != GetCombatantId()
		|| !Event.ResponseTags.HasTagExact(UPRTagLibrary::GetCombatResponseShieldBrokenTag()))
	{
		return;
	}
	bShieldBreakResponseConsumed = true;
	ShowShieldBreakPresentation();
	if (Event.bFatal || ProjectRAbilitySystemComponent->HasMatchingGameplayTag(UPRTagLibrary::GetStateStunnedTag()))
	{
		return;
	}
	FGameplayEffectContextHandle EffectContext = ProjectRAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(Prototype);
	const FGameplayEffectSpecHandle Spec = ProjectRAbilitySystemComponent->MakeOutgoingSpec(Prototype->ShieldBreakEffect, 1.0f, EffectContext);
	if (!Spec.IsValid() || !Spec.Data.IsValid()
		|| !ProjectRAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()).WasSuccessfullyApplied())
	{
		UE_LOG(LogProjectR, Error, TEXT("Enemy ShieldBreak response effect could not be applied for %s."), *GetName());
	}
}

void APREnemyCharacter::CancelActiveEnemyAttackAbilities()
{
	if (!ProjectRAbilitySystemComponent)
	{
		return;
	}
	TArray<FGameplayAbilitySpecHandle> ActiveEnemyAttackHandles;
	{
		FScopedAbilityListLock AbilityLock(*ProjectRAbilitySystemComponent);
		for (const FGameplayAbilitySpec& Spec : ProjectRAbilitySystemComponent->GetActivatableAbilities())
		{
			if (Spec.IsActive() && Spec.Ability && Spec.Ability->IsA<UPREnemyAttackGameplayAbility>())
			{
				ActiveEnemyAttackHandles.Add(Spec.Handle);
			}
		}
	}
	for (const FGameplayAbilitySpecHandle Handle : ActiveEnemyAttackHandles)
	{
		ProjectRAbilitySystemComponent->CancelAbilityHandle(Handle);
	}
}

void APREnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndAttackPresentation();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PresentationBreakTimer);
	}
	bPresentationBreakActive = false;
	ClearShieldBreakResponseLifecycle();
	ClearEnemyStunnedLifecycle();
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

void APREnemyCharacter::RefreshPresentationLabels()
{
	if (EnemyIdentityLabel)
	{
		EnemyIdentityLabel->SetText(FText::FromString(Prototype ? Prototype->EnemyId.ToString() : TEXT("Enemy")));
	}
	if (EnemyStatusLabel)
	{
		const TCHAR* Status = bEnemyStunned ? TEXT("STUN") : bPresentationBreakActive ? TEXT("BREAK") : bPresentationAttackActive ? TEXT("!") : TEXT("");
		EnemyStatusLabel->SetText(FText::FromString(Status));
	}
}

void APREnemyCharacter::ClearShieldBreakPresentation()
{
	bPresentationBreakActive = false;
	RefreshPresentationLabels();
}
