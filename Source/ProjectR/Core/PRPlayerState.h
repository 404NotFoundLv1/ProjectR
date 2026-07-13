// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/PRAttributeTypes.h"
#include "AbilitySystemInterface.h"
#include "Core/PRCombatantInterface.h"
#include "GameFramework/PlayerState.h"

#include "PRPlayerState.generated.h"

class APRPlayerCharacter;
class UGameplayEffect;
class UPRAbilitySystemComponent;
class UPRAttributeSet;
struct FOnAttributeChangeData;
struct FGameplayEffectSpecHandle;

/** Player-owned persistent GAS state shared by successive player pawns. */
UCLASS(Abstract)
class PROJECTR_API APRPlayerState
	: public APlayerState
	, public IAbilitySystemInterface
	, public IPRCombatantInterface
{
	GENERATED_BODY()

public:
	APRPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FName GetCombatantId() const override;
	virtual TSubclassOf<UGameplayEffect> GetDamageEffectClass() const override;
	UPRAbilitySystemComponent* GetProjectRAbilitySystemComponent() const;
	const UPRAttributeSet* GetAttributeSet() const;
	bool InitializeAbilitySystemForAvatar(APRPlayerCharacter* Avatar);
	void ClearAbilityAvatar(APRPlayerCharacter* Avatar);
	FPRAttributeChangedNative& OnAttributeChanged();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	UPROPERTY(EditDefaultsOnly, Category="ProjectR|Combat")
	TSubclassOf<UGameplayEffect> DamageEffect;

private:
	void BindAttributeDelegates();
	void UnbindAttributeDelegates();
	void HandleAttributeChanged(const FOnAttributeChangeData& ChangeData);
	bool TryApplyDefaultAttributes();
	bool ApplyDefaultAttributesSpec(const FGameplayEffectSpecHandle& SpecHandle);
	bool EnsureLifeStateForInitializedAvatar();

	UPROPERTY(VisibleAnywhere, Category="ProjectR|Abilities", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPRAbilitySystemComponent> ProjectRAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category="ProjectR|Abilities", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPRAttributeSet> ProjectRAttributeSet;

	TArray<TPair<FGameplayAttribute, FDelegateHandle>> AttributeDelegateHandles;
	FPRAttributeChangedNative AttributeChanged;
	bool bDefaultAttributesApplied = false;

	friend class FPRGASLifecycleTest;
};
