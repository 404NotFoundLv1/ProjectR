// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRAbilityAutomationToolset.h"

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/PRGameplayAbility.h"
#include "Characters/PRPlayerCharacter.h"
#include "Combat/PRCombatSubsystem.h"
#include "Containers/Ticker.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "Core/PRTagLibrary.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectTypes.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/UnrealType.h"

namespace PRAbilityAutomationToolset
{
namespace ValidationAuthoring
{
constexpr TCHAR DefaultSetPath[] =
	TEXT("/Game/ProjectR/Abilities/DA_DefaultAbilitySet.DA_DefaultAbilitySet");
constexpr TCHAR CooldownEffectPath[] =
	TEXT("/Game/ProjectR/MCPTest/Abilities/GE_AbilityValidation_Cooldown.GE_AbilityValidation_Cooldown");
constexpr TCHAR CostEffectPath[] =
	TEXT("/Game/ProjectR/MCPTest/Abilities/GE_AbilityValidation_EnergyCost.GE_AbilityValidation_EnergyCost");
constexpr TCHAR TriggeredAbilityPath[] =
	TEXT("/Game/ProjectR/MCPTest/Abilities/GA_AbilityValidation_Triggered.GA_AbilityValidation_Triggered");
constexpr TCHAR HeldAbilityPath[] =
	TEXT("/Game/ProjectR/MCPTest/Abilities/GA_AbilityValidation_Held.GA_AbilityValidation_Held");
constexpr TCHAR PassiveAbilityPath[] =
	TEXT("/Game/ProjectR/MCPTest/Abilities/GA_AbilityValidation_Passive.GA_AbilityValidation_Passive");
constexpr TCHAR ValidationSetPath[] =
	TEXT("/Game/ProjectR/MCPTest/Abilities/DA_AbilitySet_Validation.DA_AbilitySet_Validation");

UBlueprint* LoadRequiredBlueprint(const TCHAR* Path, FString& Error)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Path);
	if (!IsValid(Blueprint) || !IsValid(Blueprint->GeneratedClass))
	{
		Error = FString::Printf(TEXT("Required Blueprint is missing or uncompiled: %s"), Path);
		return nullptr;
	}
	return Blueprint;
}

template <typename TObjectType>
TObjectType* GetRequiredCDO(UBlueprint* Blueprint, FString& Error)
{
	TObjectType* CDO = Blueprint && Blueprint->GeneratedClass
		? Cast<TObjectType>(Blueprint->GeneratedClass->GetDefaultObject())
		: nullptr;
	if (!IsValid(CDO))
	{
		Error = FString::Printf(
			TEXT("Blueprint %s does not generate the required CDO type."),
			*GetNameSafe(Blueprint));
	}
	return CDO;
}

bool SetGameplayTagProperty(UObject* Object, const FName PropertyName, const FGameplayTag& Value)
{
	FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName);
	if (!Property || Property->Struct != FGameplayTag::StaticStruct())
	{
		return false;
	}
	*Property->ContainerPtrToValuePtr<FGameplayTag>(Object) = Value;
	return true;
}

bool SetActivationPolicyProperty(
	UObject* Object,
	const EPRAbilityActivationPolicy Value)
{
	FEnumProperty* Property = FindFProperty<FEnumProperty>(
		Object->GetClass(), TEXT("ActivationPolicy"));
	if (!Property)
	{
		return false;
	}
	void* ValueAddress = Property->ContainerPtrToValuePtr<void>(Object);
	Property->GetUnderlyingProperty()->SetIntPropertyValue(
		ValueAddress, static_cast<int64>(Value));
	return true;
}

bool SetBoolProperty(UObject* Object, const FName PropertyName, const bool Value)
{
	FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName);
	if (!Property)
	{
		return false;
	}
	Property->SetPropertyValue_InContainer(Object, Value);
	return true;
}

bool SetClassProperty(UObject* Object, const FName PropertyName, UClass* Value)
{
	FClassProperty* Property = FindFProperty<FClassProperty>(Object->GetClass(), PropertyName);
	if (!Property)
	{
		return false;
	}
	Property->SetPropertyValue_InContainer(Object, Value);
	return true;
}

bool ConfigureAbilityCDO(
	UBlueprint* Blueprint,
	const FGameplayTag& AbilityTag,
	const EPRAbilityActivationPolicy Policy,
	UClass* CostEffectClass,
	UClass* CooldownEffectClass,
	FString& Error)
{
	UPRGameplayAbility* Ability = GetRequiredCDO<UPRGameplayAbility>(Blueprint, Error);
	if (!Ability)
	{
		return false;
	}

	Blueprint->Modify();
	Ability->Modify();
	const bool bConfigured =
		SetGameplayTagProperty(Ability, TEXT("AbilityTag"), AbilityTag)
		&& SetActivationPolicyProperty(Ability, Policy)
		&& SetBoolProperty(Ability, TEXT("bCanActivateWhileDead"), false)
		&& SetBoolProperty(Ability, TEXT("bCancelOnDeath"), true)
		&& SetClassProperty(Ability, TEXT("CostGameplayEffectClass"), CostEffectClass)
		&& SetClassProperty(Ability, TEXT("CooldownGameplayEffectClass"), CooldownEffectClass);
	if (!bConfigured)
	{
		Error = FString::Printf(
			TEXT("The fixed ability properties could not be resolved on %s."),
			*GetNameSafe(Blueprint));
		return false;
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	return true;
}

void MarkBlueprintConfigured(UBlueprint* Blueprint)
{
	Blueprint->Modify();
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
}

UToolCallAsyncResultString* Configure()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	FString Error;

	UPRAbilitySetDataAsset* DefaultSet = LoadObject<UPRAbilitySetDataAsset>(nullptr, DefaultSetPath);
	UPRAbilitySetDataAsset* ValidationSet = LoadObject<UPRAbilitySetDataAsset>(nullptr, ValidationSetPath);
	UBlueprint* CooldownBlueprint = LoadRequiredBlueprint(CooldownEffectPath, Error);
	UBlueprint* CostBlueprint = LoadRequiredBlueprint(CostEffectPath, Error);
	UBlueprint* TriggeredBlueprint = LoadRequiredBlueprint(TriggeredAbilityPath, Error);
	UBlueprint* HeldBlueprint = LoadRequiredBlueprint(HeldAbilityPath, Error);
	UBlueprint* PassiveBlueprint = LoadRequiredBlueprint(PassiveAbilityPath, Error);
	if (!IsValid(DefaultSet) || !IsValid(ValidationSet)
		|| !CooldownBlueprint || !CostBlueprint || !TriggeredBlueprint
		|| !HeldBlueprint || !PassiveBlueprint)
	{
		if (Error.IsEmpty())
		{
			Error = TEXT("One or both fixed AbilitySet DataAssets are missing.");
		}
		Result->SetError(Error);
		return Result;
	}

	UGameplayEffect* CooldownEffect = GetRequiredCDO<UGameplayEffect>(CooldownBlueprint, Error);
	UGameplayEffect* CostEffect = GetRequiredCDO<UGameplayEffect>(CostBlueprint, Error);
	if (!CooldownEffect || !CostEffect)
	{
		Result->SetError(Error);
		return Result;
	}

	DefaultSet->Modify();
	DefaultSet->AbilityEntries.Reset();
	DefaultSet->MarkPackageDirty();

	CooldownEffect->Modify();
	CooldownEffect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	CooldownEffect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
	CooldownEffect->Period = FScalableFloat(0.0f);
	CooldownEffect->Modifiers.Reset();
	CooldownEffect->Executions.Reset();
	CooldownEffect->GameplayCues.Reset();
	UTargetTagsGameplayEffectComponent& TargetTags =
		CooldownEffect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer CooldownTags;
	CooldownTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Validation")));
	TargetTags.SetAndApplyTargetTagChanges(CooldownTags);
	MarkBlueprintConfigured(CooldownBlueprint);

	CostEffect->Modify();
	CostEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
	CostEffect->Period = FScalableFloat(0.0f);
	CostEffect->Modifiers.Reset();
	CostEffect->Executions.Reset();
	CostEffect->GameplayCues.Reset();
	FGameplayModifierInfo EnergyModifier;
	EnergyModifier.Attribute = UPRAttributeSet::GetEnergyAttribute();
	EnergyModifier.ModifierOp = EGameplayModOp::Additive;
	EnergyModifier.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(FScalableFloat(-10.0f));
	CostEffect->Modifiers.Add(EnergyModifier);
	MarkBlueprintConfigured(CostBlueprint);

	if (!ConfigureAbilityCDO(
		TriggeredBlueprint,
		FGameplayTag::RequestGameplayTag(TEXT("Ability.Validation.OnInputTriggered")),
		EPRAbilityActivationPolicy::OnInputTriggered,
		CostBlueprint->GeneratedClass,
		CooldownBlueprint->GeneratedClass,
		Error)
		|| !ConfigureAbilityCDO(
			HeldBlueprint,
			FGameplayTag::RequestGameplayTag(TEXT("Ability.Validation.WhileInputActive")),
			EPRAbilityActivationPolicy::WhileInputActive,
			nullptr,
			nullptr,
			Error)
		|| !ConfigureAbilityCDO(
			PassiveBlueprint,
			FGameplayTag::RequestGameplayTag(TEXT("Ability.Validation.Passive")),
			EPRAbilityActivationPolicy::Passive,
			nullptr,
			nullptr,
			Error))
	{
		Result->SetError(Error);
		return Result;
	}

	ValidationSet->Modify();
	ValidationSet->AbilityEntries.Reset();
	auto AddEntry = [ValidationSet](
		UClass* AbilityClass,
		const FGameplayTag& InputTag)
	{
		FPRAbilitySetEntry& Entry = ValidationSet->AbilityEntries.AddDefaulted_GetRef();
		Entry.AbilityClass = AbilityClass;
		Entry.AbilityLevel = 1;
		Entry.InputTag = InputTag;
		Entry.bGrantOnInitialization = true;
		Entry.GrantedSpecTags.Reset();
		Entry.AbilityData = nullptr;
	};
	AddEntry(
		TriggeredBlueprint->GeneratedClass,
		UPRTagLibrary::GetInputAttackTag());
	AddEntry(HeldBlueprint->GeneratedClass, UPRTagLibrary::GetInputDodgeTag());
	AddEntry(PassiveBlueprint->GeneratedClass, FGameplayTag());
	ValidationSet->MarkPackageDirty();

	float CooldownMagnitude = 0.0f;
	const bool bHasStaticCooldown =
		CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(
			1.0f, CooldownMagnitude);
	const bool bConfigured =
		DefaultSet->AbilityEntries.IsEmpty()
		&& ValidationSet->AbilityEntries.Num() == 3
		&& CooldownEffect->DurationPolicy == EGameplayEffectDurationType::HasDuration
		&& bHasStaticCooldown
		&& FMath::IsNearlyEqual(CooldownMagnitude, 0.5f)
		&& CooldownEffect->GetGrantedTags().HasTagExact(
			FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Validation")))
		&& CostEffect->Modifiers.Num() == 1
		&& CostEffect->Modifiers[0].Attribute == UPRAttributeSet::GetEnergyAttribute();
	if (!bConfigured)
	{
		Result->SetError(TEXT("Fixed ability validation asset readback failed."));
		return Result;
	}

	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("status"), TEXT("PASS"));
	Json->SetNumberField(TEXT("configuredPackages"), 7);
	Json->SetNumberField(TEXT("validationEntries"), ValidationSet->AbilityEntries.Num());
	Json->SetNumberField(TEXT("cooldownSeconds"), 0.5);
	Json->SetNumberField(TEXT("energyCost"), 10.0);
	Json->SetBoolField(TEXT("saved"), false);
	FString JsonString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(Json, Writer);
	Result->SetValue(JsonString);
	return Result;
}
} // namespace ValidationAuthoring

enum class ESmokePhase : uint8
{
	Grant,
	FirstAttack,
	CooldownRejected,
	WaitForCooldown,
	SecondAttack,
	WaitForSecondCooldown,
	CostRejected,
	HeldPress,
	HeldRelease,
	HeldBeforeDeath,
	FatalDamage,
	DeadAttack,
	Revive,
	Remove,
	Complete
};

class FPIEAbilitySmokeRunner : public TSharedFromThis<FPIEAbilitySmokeRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FPIEAbilitySmokeRunner> Runner = MakeShared<FPIEAbilitySmokeRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}

		Runner->PhaseStartTime = Runner->GetPlayWorldTimeSeconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](float DeltaSeconds) mutable
			{
				const bool bContinue = RunnerPtr->Tick(DeltaSeconds);
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

	~FPIEAbilitySmokeRunner()
	{
		Cleanup();
	}

private:
	bool Initialize()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Fail(TEXT("RunPIEAbilitySmoke requires an active authoritative in-process PIE world."));
			return false;
		}

		Controller = Cast<APRPlayerController>(PlayWorld->GetFirstPlayerController());
		Character = Controller.IsValid() ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		PlayerState = Character.IsValid() ? Character->GetPlayerState<APRPlayerState>() : nullptr;
		ASC = PlayerState.IsValid() ? PlayerState->GetProjectRAbilitySystemComponent() : nullptr;
		CombatSubsystem = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		ValidationSet = LoadObject<UPRAbilitySetDataAsset>(
			nullptr,
			TEXT("/Game/ProjectR/MCPTest/Abilities/DA_AbilitySet_Validation.DA_AbilitySet_Validation"));
		if (!Controller.IsValid() || !Character.IsValid() || !PlayerState.IsValid()
			|| !ASC.IsValid() || !CombatSubsystem.IsValid() || !ValidationSet.IsValid())
		{
			Fail(TEXT("PIE must contain the formal ProjectR player and the fixed validation AbilitySet."));
			return false;
		}
		if (ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
		{
			Fail(TEXT("Ability smoke requires an initially alive player."));
			return false;
		}

		const UPRAttributeSet* Attributes = PlayerState->GetAttributeSet();
		InitialEnergy = Attributes->GetEnergy();
		InitialHealth = Attributes->GetHealth();
		InitialShield = Attributes->GetShield();
		ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), 100.0f);

		AttackInputTag = UPRTagLibrary::GetInputAttackTag();
		DodgeInputTag = UPRTagLibrary::GetInputDodgeTag();
		TriggeredAbilityTag = FGameplayTag::RequestGameplayTag(
			TEXT("Ability.Validation.OnInputTriggered"));
		HeldAbilityTag = FGameplayTag::RequestGameplayTag(
			TEXT("Ability.Validation.WhileInputActive"));
		PassiveAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Validation.Passive"));
		CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Validation"));

		LifecycleHandle = ASC->OnAbilityLifecycleEvent().AddLambda(
			[WeakThis = TWeakPtr<FPIEAbilitySmokeRunner>(AsShared())](const FPRAbilityLifecycleEvent& Event)
			{
				if (TSharedPtr<FPIEAbilitySmokeRunner> This = WeakThis.Pin())
				{
					This->LifecycleEvents.Add(Event);
				}
			});
		return true;
	}

	bool Tick(float DeltaSeconds)
	{
		if (!Character.IsValid() || !PlayerState.IsValid() || !ASC.IsValid()
			|| !CombatSubsystem.IsValid() || !GEditor || GEditor->PlayWorld != Character->GetWorld())
		{
			Fail(TEXT("PIE ended or ProjectR ability objects became invalid during smoke."));
			return false;
		}

		const double Elapsed = GetPlayWorldTimeSeconds() - PhaseStartTime;
		switch (Phase)
		{
		case ESmokePhase::Grant:
		{
			const EPRAbilitySetOperationStatus Status = ASC->GrantAbilitySet(
				ValidationSet.Get(), EPRAbilitySetGrantMode::AllEntries, GrantHandle);
			if (Status != EPRAbilitySetOperationStatus::Applied
				|| GrantHandle.AbilitySpecHandles.Num() != 3
				|| !IsAbilityActive(PassiveAbilityTag))
			{
				Fail(TEXT("Validation AbilitySet did not grant three Specs and activate Passive."));
				return false;
			}
			Advance(ESmokePhase::FirstAttack);
			break;
		}

		case ESmokePhase::FirstAttack:
			PressAndRelease(AttackInputTag);
			Advance(ESmokePhase::CooldownRejected);
			break;

		case ESmokePhase::CooldownRejected:
			if (Elapsed < 0.05)
			{
				break;
			}
			if (!CheckEnergy(90.0f) || ASC->GetGameplayTagCount(CooldownTag) != 1)
			{
				Fail(TEXT("First Attack did not commit Energy 100->90 and Cooldown.Validation."));
				return false;
			}
			PressAndRelease(AttackInputTag);
			if (!CheckEnergy(90.0f))
			{
				Fail(TEXT("Cooldown-rejected Attack changed Energy."));
				return false;
			}
			Advance(ESmokePhase::WaitForCooldown);
			break;

		case ESmokePhase::WaitForCooldown:
			if (Elapsed >= 0.60)
			{
				Advance(ESmokePhase::SecondAttack);
			}
			break;

		case ESmokePhase::SecondAttack:
			if (ASC->GetGameplayTagCount(CooldownTag) != 0)
			{
				Fail(TEXT("Validation cooldown did not expire after 0.6 seconds."));
				return false;
			}
			PressAndRelease(AttackInputTag);
			Advance(ESmokePhase::WaitForSecondCooldown);
			break;

		case ESmokePhase::WaitForSecondCooldown:
			if (Elapsed < 0.60)
			{
				break;
			}
			if (!CheckEnergy(80.0f) || ASC->GetGameplayTagCount(CooldownTag) != 0)
			{
				Fail(TEXT("Second Attack or its cooldown expiration was incorrect."));
				return false;
			}
			Advance(ESmokePhase::CostRejected);
			break;

		case ESmokePhase::CostRejected:
			ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), 0.0f);
			PressAndRelease(AttackInputTag);
			if (!CheckEnergy(0.0f) || ASC->GetGameplayTagCount(CooldownTag) != 0)
			{
				Fail(TEXT("Cost-rejected Attack changed Energy or created Cooldown."));
				return false;
			}
			Advance(ESmokePhase::HeldPress);
			break;

		case ESmokePhase::HeldPress:
			ASC->AbilityInputTagPressed(DodgeInputTag);
			ASC->AbilityInputTagPressed(DodgeInputTag);
			if (!IsAbilityActive(HeldAbilityTag) || !IsAbilityHeld(HeldAbilityTag))
			{
				Fail(TEXT("Held ability did not activate and remain held exactly once."));
				return false;
			}
			Advance(ESmokePhase::HeldRelease);
			break;

		case ESmokePhase::HeldRelease:
			ASC->AbilityInputTagReleased(DodgeInputTag);
			ASC->AbilityInputTagReleased(DodgeInputTag);
			if (IsAbilityActive(HeldAbilityTag) || IsAbilityHeld(HeldAbilityTag))
			{
				Fail(TEXT("Held ability did not cancel and clear Held on release."));
				return false;
			}
			Advance(ESmokePhase::HeldBeforeDeath);
			break;

		case ESmokePhase::HeldBeforeDeath:
			ASC->AbilityInputTagPressed(DodgeInputTag);
			if (!IsAbilityActive(HeldAbilityTag))
			{
				Fail(TEXT("Held ability could not reactivate before death."));
				return false;
			}
			Advance(ESmokePhase::FatalDamage);
			break;

		case ESmokePhase::FatalDamage:
		{
			FPRDamageRequest Request;
			Request.SourceId = TEXT("MCP.AbilitySmoke");
			Request.DamageSource = Character.Get();
			Request.Instigator = Character.Get();
			Request.Target = Character.Get();
			Request.RawDamage = 200000.0f;
			if (CombatSubsystem->ApplyDamage(Request) != EPRCombatRequestStatus::Applied
				|| !ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag())
				|| IsAbilityActive(HeldAbilityTag) || IsAbilityActive(PassiveAbilityTag))
			{
				Fail(TEXT("Fatal combat did not cancel Held and Passive abilities."));
				return false;
			}
			Advance(ESmokePhase::DeadAttack);
			break;
		}

		case ESmokePhase::DeadAttack:
			PressAndRelease(AttackInputTag);
			if (!HasFailureTags(TriggeredAbilityTag,
				UPRTagLibrary::GetAbilityActivateFailTagsBlockedTag(),
				UPRTagLibrary::GetStateDeadTag()))
			{
				Fail(TEXT("Dead Attack did not publish TagsBlocked and State.Dead."));
				return false;
			}
			Advance(ESmokePhase::Revive);
			break;

		case ESmokePhase::Revive:
		{
			FPRReviveRequest Request;
			Request.SourceId = TEXT("MCP.AbilitySmoke");
			Request.DamageSource = Character.Get();
			Request.Instigator = Character.Get();
			Request.Target = Character.Get();
			Request.HealthFraction = 1.0f;
			Request.ShieldFraction = 1.0f;
			if (CombatSubsystem->Revive(Request) != EPRCombatRequestStatus::Applied
				|| !IsAbilityActive(PassiveAbilityTag))
			{
				Fail(TEXT("Revive did not restore Passive activation eligibility."));
				return false;
			}
			Advance(ESmokePhase::Remove);
			break;
		}

		case ESmokePhase::Remove:
			if (ASC->RemoveAbilitySet(GrantHandle) != EPRAbilitySetOperationStatus::Removed
				|| ASC->RemoveAbilitySet(GrantHandle) != EPRAbilitySetOperationStatus::NotFound)
			{
				Fail(TEXT("Validation AbilitySet exact removal or repeated removal failed."));
				return false;
			}
			bSetRemoved = true;
			Advance(ESmokePhase::Complete);
			break;

		case ESmokePhase::Complete:
			return Finish();
		}
		return true;
	}

	void PressAndRelease(const FGameplayTag& InputTag) const
	{
		ASC->AbilityInputTagPressed(InputTag);
		ASC->AbilityInputTagReleased(InputTag);
	}

	bool CheckEnergy(float Expected) const
	{
		return FMath::IsNearlyEqual(PlayerState->GetAttributeSet()->GetEnergy(), Expected);
	}

	bool IsAbilityActive(const FGameplayTag& AbilityTag) const
	{
		FPRAbilityRuntimeState State;
		return ASC->GetAbilityRuntimeStateByAbilityTag(AbilityTag, State) && State.bActive;
	}

	bool IsAbilityHeld(const FGameplayTag& AbilityTag) const
	{
		FPRAbilityRuntimeState State;
		return ASC->GetAbilityRuntimeStateByAbilityTag(AbilityTag, State) && State.bInputHeld;
	}

	bool HasFailureTags(
		const FGameplayTag& AbilityTag,
		const FGameplayTag& First,
		const FGameplayTag& Second) const
	{
		for (int32 Index = LifecycleEvents.Num() - 1; Index >= 0; --Index)
		{
			const FPRAbilityLifecycleEvent& Event = LifecycleEvents[Index];
			if (Event.EventType == EPRAbilityLifecycleEventType::ActivationFailed
				&& Event.AbilityState.AbilityTag == AbilityTag)
			{
				return Event.FailureTags.HasTagExact(First)
					&& Event.FailureTags.HasTagExact(Second);
			}
		}
		return false;
	}

	bool Finish()
	{
		const bool bNoValidationSpecs =
			!HasAbility(TriggeredAbilityTag) && !HasAbility(HeldAbilityTag) && !HasAbility(PassiveAbilityTag);
		const bool bCleanRuntime = bNoValidationSpecs
			&& ASC->GetGameplayTagCount(CooldownTag) == 0
			&& ASC->GetActiveEffects(FGameplayEffectQuery()).IsEmpty();

		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("status"), bCleanRuntime ? TEXT("PASS") : TEXT("FAIL"));
		Json->SetNumberField(TEXT("lifecycleEventCount"), LifecycleEvents.Num());
		Json->SetBoolField(TEXT("validationSpecsRemoved"), bNoValidationSpecs);
		Json->SetBoolField(TEXT("cooldownCleared"), ASC->GetGameplayTagCount(CooldownTag) == 0);
		Json->SetNumberField(TEXT("energy"), InitialEnergy);
		Json->SetNumberField(TEXT("health"), InitialHealth);
		Json->SetNumberField(TEXT("shield"), InitialShield);

		FString JsonString;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
		FJsonSerializer::Serialize(Json, Writer);
		Cleanup();
		if (bCleanRuntime)
		{
			Result->SetValue(JsonString);
		}
		else
		{
			Result->SetError(JsonString);
		}
		return false;
	}

	bool HasAbility(const FGameplayTag& AbilityTag) const
	{
		FPRAbilityRuntimeState State;
		return ASC->GetAbilityRuntimeStateByAbilityTag(AbilityTag, State);
	}

	void Advance(ESmokePhase NextPhase)
	{
		Phase = NextPhase;
		PhaseStartTime = GetPlayWorldTimeSeconds();
	}

	double GetPlayWorldTimeSeconds() const
	{
		const UWorld* PlayWorld = Character.IsValid() ? Character->GetWorld() : nullptr;
		return IsValid(PlayWorld) ? PlayWorld->GetTimeSeconds() : 0.0;
	}

	void Cleanup()
	{
		if (ASC.IsValid())
		{
			if (LifecycleHandle.IsValid())
			{
				ASC->OnAbilityLifecycleEvent().Remove(LifecycleHandle);
				LifecycleHandle.Reset();
			}
			if (!bSetRemoved && GrantHandle.GrantId.IsValid())
			{
				ASC->RemoveAbilitySet(GrantHandle);
				bSetRemoved = true;
			}
			if (CooldownTag.IsValid())
			{
				FGameplayTagContainer CooldownTags;
				CooldownTags.AddTag(CooldownTag);
				ASC->RemoveActiveEffectsWithGrantedTags(CooldownTags);
			}
			if (CombatSubsystem.IsValid() && Character.IsValid()
				&& ASC->HasMatchingGameplayTag(UPRTagLibrary::GetStateDeadTag()))
			{
				FPRReviveRequest Request;
				Request.SourceId = TEXT("MCP.AbilitySmoke.Cleanup");
				Request.DamageSource = Character.Get();
				Request.Instigator = Character.Get();
				Request.Target = Character.Get();
				Request.HealthFraction = 1.0f;
				Request.ShieldFraction = 1.0f;
				CombatSubsystem->Revive(Request);
			}
			ASC->SetNumericAttributeBase(UPRAttributeSet::GetEnergyAttribute(), InitialEnergy);
			ASC->SetNumericAttributeBase(UPRAttributeSet::GetHealthAttribute(), InitialHealth);
			ASC->SetNumericAttributeBase(UPRAttributeSet::GetShieldAttribute(), InitialShield);
		}
	}

	void Fail(const FString& Message)
	{
		Cleanup();
		if (Result.IsValid())
		{
			Result->SetError(Message);
		}
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<APRPlayerController> Controller;
	TWeakObjectPtr<APRPlayerCharacter> Character;
	TWeakObjectPtr<APRPlayerState> PlayerState;
	TWeakObjectPtr<UPRAbilitySystemComponent> ASC;
	TWeakObjectPtr<UPRCombatSubsystem> CombatSubsystem;
	TWeakObjectPtr<UPRAbilitySetDataAsset> ValidationSet;
	FPRAbilitySetGrantHandle GrantHandle;
	FDelegateHandle LifecycleHandle;
	TArray<FPRAbilityLifecycleEvent> LifecycleEvents;
	FGameplayTag AttackInputTag;
	FGameplayTag DodgeInputTag;
	FGameplayTag TriggeredAbilityTag;
	FGameplayTag HeldAbilityTag;
	FGameplayTag PassiveAbilityTag;
	FGameplayTag CooldownTag;
	ESmokePhase Phase = ESmokePhase::Grant;
	double PhaseStartTime = 0.0;
	float InitialEnergy = 0.0f;
	float InitialHealth = 0.0f;
	float InitialShield = 0.0f;
	bool bSetRemoved = false;
};
} // namespace PRAbilityAutomationToolset

UToolCallAsyncResultString* UPRAbilityAutomationToolset::ConfigureAbilityValidationAssets()
{
	return PRAbilityAutomationToolset::ValidationAuthoring::Configure();
}

UToolCallAsyncResultString* UPRAbilityAutomationToolset::RunPIEAbilitySmoke()
{
	return PRAbilityAutomationToolset::FPIEAbilitySmokeRunner::Start();
}
