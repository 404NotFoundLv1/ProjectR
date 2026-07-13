// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/PRPlayerState.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Characters/PRPlayerCharacter.h"
#include "Core/PRTagLibrary.h"
#include "GameplayEffect.h"
#include "ProjectR.h"

APRPlayerState::APRPlayerState()
{
	SetNetUpdateFrequency(100.0f);

	ProjectRAbilitySystemComponent =
		CreateDefaultSubobject<UPRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ProjectRAbilitySystemComponent->SetIsReplicated(true);
	ProjectRAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	ProjectRAttributeSet = CreateDefaultSubobject<UPRAttributeSet>(TEXT("AttributeSet"));
	ProjectRAbilitySystemComponent->AddAttributeSetSubobject(ProjectRAttributeSet.Get());
}

UAbilitySystemComponent* APRPlayerState::GetAbilitySystemComponent() const
{
	return ProjectRAbilitySystemComponent;
}

FName APRPlayerState::GetCombatantId() const
{
	return TEXT("Player");
}

TSubclassOf<UGameplayEffect> APRPlayerState::GetDamageEffectClass() const
{
	return DamageEffect;
}

UPRAbilitySystemComponent* APRPlayerState::GetProjectRAbilitySystemComponent() const
{
	return ProjectRAbilitySystemComponent;
}

const UPRAttributeSet* APRPlayerState::GetAttributeSet() const
{
	return ProjectRAttributeSet;
}

bool APRPlayerState::InitializeAbilitySystemForAvatar(APRPlayerCharacter* Avatar)
{
	if (!IsValid(Avatar) || !IsValid(ProjectRAbilitySystemComponent))
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerState %s cannot initialize GAS for an invalid Avatar."), *GetPathName());
		return false;
	}

	ProjectRAbilitySystemComponent->InitAbilityActorInfo(this, Avatar);
	if (ProjectRAbilitySystemComponent->GetOwnerActor() != this
		|| ProjectRAbilitySystemComponent->GetAvatarActor() != Avatar)
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerState %s failed to initialize GAS ActorInfo for %s."),
			*GetPathName(), *Avatar->GetPathName());
		return false;
	}

	BindAttributeDelegates();
	if (HasAuthority() && !bDefaultAttributesApplied && !TryApplyDefaultAttributes())
	{
		return false;
	}
	if (HasAuthority() && !EnsureLifeStateForInitializedAvatar())
	{
		return false;
	}

	UE_LOG(LogProjectR, Display, TEXT("ProjectR GAS initialized: Owner=%s Avatar=%s DefaultApplied=%s"),
		*GetPathName(), *Avatar->GetPathName(), bDefaultAttributesApplied ? TEXT("true") : TEXT("client"));
	return true;
}

void APRPlayerState::ClearAbilityAvatar(APRPlayerCharacter* Avatar)
{
	if (!IsValid(ProjectRAbilitySystemComponent) || !IsValid(Avatar))
	{
		return;
	}
	if (ProjectRAbilitySystemComponent->GetAvatarActor() == Avatar)
	{
		ProjectRAbilitySystemComponent->ClearActorInfo();
	}
}

FPRAttributeChangedNative& APRPlayerState::OnAttributeChanged()
{
	return AttributeChanged;
}

void APRPlayerState::BeginPlay()
{
	Super::BeginPlay();
	BindAttributeDelegates();
}

void APRPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAttributeDelegates();
	Super::EndPlay(EndPlayReason);
}

void APRPlayerState::BindAttributeDelegates()
{
	if (!IsValid(ProjectRAbilitySystemComponent) || !AttributeDelegateHandles.IsEmpty())
	{
		return;
	}

	const FGameplayAttribute Attributes[] = {
		UPRAttributeSet::GetMaxHealthAttribute(),
		UPRAttributeSet::GetHealthAttribute(),
		UPRAttributeSet::GetMaxShieldAttribute(),
		UPRAttributeSet::GetShieldAttribute(),
		UPRAttributeSet::GetMaxEnergyAttribute(),
		UPRAttributeSet::GetEnergyAttribute(),
		UPRAttributeSet::GetAttackPowerAttribute(),
		UPRAttributeSet::GetMoveSpeedAttribute(),
		UPRAttributeSet::GetCritChanceAttribute(),
		UPRAttributeSet::GetPermissionAttribute(),
		UPRAttributeSet::GetResonanceAttribute()};
	for (const FGameplayAttribute& Attribute : Attributes)
	{
		FDelegateHandle Handle = ProjectRAbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(Attribute)
			.AddUObject(this, &APRPlayerState::HandleAttributeChanged);
		AttributeDelegateHandles.Emplace(Attribute, Handle);
	}
}

void APRPlayerState::UnbindAttributeDelegates()
{
	if (!IsValid(ProjectRAbilitySystemComponent))
	{
		AttributeDelegateHandles.Reset();
		return;
	}
	for (const TPair<FGameplayAttribute, FDelegateHandle>& Entry : AttributeDelegateHandles)
	{
		ProjectRAbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(Entry.Key)
			.Remove(Entry.Value);
	}
	AttributeDelegateHandles.Reset();
}

void APRPlayerState::HandleAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	if (FMath::IsNearlyEqual(ChangeData.OldValue, ChangeData.NewValue))
	{
		return;
	}
	AttributeChanged.Broadcast({ChangeData.Attribute, ChangeData.OldValue, ChangeData.NewValue});
}

bool APRPlayerState::TryApplyDefaultAttributes()
{
	if (!DefaultAttributesEffect)
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerState %s has no DefaultAttributesEffect."), *GetPathName());
		return false;
	}

	FGameplayEffectContextHandle Context = ProjectRAbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = ProjectRAbilitySystemComponent->MakeOutgoingSpec(
		DefaultAttributesEffect, 1.0f, Context);
	return ApplyDefaultAttributesSpec(SpecHandle);
}

bool APRPlayerState::ApplyDefaultAttributesSpec(const FGameplayEffectSpecHandle& SpecHandle)
{
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerState %s received an invalid default GameplayEffect Spec."), *GetPathName());
		return false;
	}
	if (bDefaultAttributesApplied)
	{
		return true;
	}

	const FActiveGameplayEffectHandle AppliedHandle =
		ProjectRAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (!AppliedHandle.WasSuccessfullyApplied())
	{
		UE_LOG(LogProjectR, Error, TEXT("PlayerState %s failed to apply DefaultAttributesEffect."), *GetPathName());
		return false;
	}

	bDefaultAttributesApplied = true;
	UE_LOG(LogProjectR, Display, TEXT("ProjectR default attributes applied once to %s."), *GetPathName());
	return true;
}

bool APRPlayerState::EnsureLifeStateForInitializedAvatar()
{
	if (!IsValid(ProjectRAbilitySystemComponent) || !IsValid(ProjectRAttributeSet))
	{
		return false;
	}

	const FGameplayTag& AliveTag = UPRTagLibrary::GetStateAliveTag();
	const FGameplayTag& DeadTag = UPRTagLibrary::GetStateDeadTag();
	if (ProjectRAbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		ProjectRAbilitySystemComponent->SetLooseGameplayTagCount(
			AliveTag, 0, EGameplayTagReplicationState::TagOnly);
		return ProjectRAbilitySystemComponent->GetGameplayTagCount(AliveTag) == 0
			&& ProjectRAbilitySystemComponent->GetGameplayTagCount(DeadTag) > 0;
	}

	if (ProjectRAttributeSet->GetHealth() <= 0.0f)
	{
		UE_LOG(LogProjectR, Error,
			TEXT("PlayerState %s has zero Health without State.Dead during GAS initialization."),
			*GetPathName());
		return false;
	}

	ProjectRAbilitySystemComponent->SetLooseGameplayTagCount(
		DeadTag, 0, EGameplayTagReplicationState::TagOnly);
	ProjectRAbilitySystemComponent->SetLooseGameplayTagCount(
		AliveTag, 1, EGameplayTagReplicationState::TagOnly);
	return ProjectRAbilitySystemComponent->GetGameplayTagCount(AliveTag) == 1
		&& ProjectRAbilitySystemComponent->GetGameplayTagCount(DeadTag) == 0;
}
