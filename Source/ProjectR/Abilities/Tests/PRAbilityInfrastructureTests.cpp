// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAbilityTypes.h"
#include "Abilities/PRGameplayAbility.h"
#include "Core/PRTagLibrary.h"

#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Misc/DataValidation.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace PRAbilityAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

template <typename TProperty>
const TProperty* FindRequiredProperty(FAutomationTestBase& Test, const UClass* Class, const FName Name)
{
	const TProperty* Property = FindFProperty<TProperty>(Class, Name);
	Test.TestNotNull(*FString::Printf(TEXT("%s.%s exists"), *Class->GetName(), *Name.ToString()), Property);
	return Property;
}
} // namespace PRAbilityAutomation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAbilitySchemaTest,
	"ProjectR.Ability.Schema.TypesTagsAndDataValidation",
	PRAbilityAutomation::TestFlags)

bool FPRAbilitySchemaTest::RunTest(const FString& Parameters)
{
	const FPRAbilitySetEntry Entry;
	TestNull(TEXT("Entry AbilityClass defaults null"), Entry.AbilityClass.Get());
	TestEqual(TEXT("Entry AbilityLevel defaults to one"), Entry.AbilityLevel, 1);
	TestFalse(TEXT("Entry InputTag defaults invalid"), Entry.InputTag.IsValid());
	TestTrue(TEXT("Entry grants on initialization by default"), Entry.bGrantOnInitialization);
	TestTrue(TEXT("Entry GrantedSpecTags defaults empty"), Entry.GrantedSpecTags.IsEmpty());
	TestNull(TEXT("Entry AbilityData defaults null"), Entry.AbilityData.Get());

	const FPRAbilitySetGrantHandle GrantHandle;
	TestFalse(TEXT("GrantId defaults invalid"), GrantHandle.GrantId.IsValid());
	TestFalse(TEXT("Grant handle set defaults invalid"), GrantHandle.AbilitySet.IsValid());
	TestTrue(TEXT("Grant handle specs default empty"), GrantHandle.AbilitySpecHandles.IsEmpty());

	const FPRAbilityRuntimeState RuntimeState;
	TestFalse(TEXT("Runtime SpecHandle defaults invalid"), RuntimeState.SpecHandle.IsValid());
	TestEqual(TEXT("Runtime policy defaults triggered"), RuntimeState.ActivationPolicy,
		EPRAbilityActivationPolicy::OnInputTriggered);
	TestEqual(TEXT("Runtime level defaults one"), RuntimeState.AbilityLevel, 1);
	TestFalse(TEXT("Runtime active defaults false"), RuntimeState.bActive);
	TestFalse(TEXT("Runtime held defaults false"), RuntimeState.bInputHeld);
	TestFalse(TEXT("Runtime can-activate defaults false"), RuntimeState.bCanActivate);

	UPRAbilitySetDataAsset* EmptySet = NewObject<UPRAbilitySetDataAsset>();
	TestEqual(TEXT("AbilitySet primary type"), EmptySet->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("ProjectRAbilitySet")));
	TestEqual(TEXT("Transient AbilitySet primary name"), EmptySet->GetPrimaryAssetId().PrimaryAssetName,
		EmptySet->GetFName());
#if WITH_EDITOR
	FDataValidationContext ValidationContext;
	TestEqual(TEXT("Empty formal AbilitySet is valid"),
		EmptySet->IsDataValid(ValidationContext), EDataValidationResult::Valid);
#endif

	TestTrue(TEXT("Failure CanActivate tag exists"),
		UPRTagLibrary::GetAbilityActivateFailCanActivateTag().IsValid());
	TestTrue(TEXT("Failure Cooldown tag exists"),
		UPRTagLibrary::GetAbilityActivateFailCooldownTag().IsValid());
	TestTrue(TEXT("Failure Cost tag exists"),
		UPRTagLibrary::GetAbilityActivateFailCostTag().IsValid());
	TestTrue(TEXT("Failure Networking tag exists"),
		UPRTagLibrary::GetAbilityActivateFailNetworkingTag().IsValid());
	TestTrue(TEXT("Failure TagsBlocked tag exists"),
		UPRTagLibrary::GetAbilityActivateFailTagsBlockedTag().IsValid());
	TestTrue(TEXT("Failure TagsMissing tag exists"),
		UPRTagLibrary::GetAbilityActivateFailTagsMissingTag().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAbilityGrantRemoveInputTest,
	"ProjectR.Ability.Runtime.GrantRemoveAndInputRouting",
	PRAbilityAutomation::TestFlags)

bool FPRAbilityGrantRemoveInputTest::RunTest(const FString& Parameters)
{
	UPRAbilitySystemComponent* ASC = NewObject<UPRAbilitySystemComponent>();
	FPRAbilitySetGrantHandle OutHandle;
	TestEqual(TEXT("Null set is rejected"),
		ASC->GrantAbilitySet(nullptr, EPRAbilitySetGrantMode::AllEntries, OutHandle),
		EPRAbilitySetOperationStatus::Invalid);
	TestFalse(TEXT("Rejected grant leaves invalid handle"), OutHandle.GrantId.IsValid());
	TestEqual(TEXT("Empty handle cannot remove a set"), ASC->RemoveAbilitySet(OutHandle),
		EPRAbilitySetOperationStatus::NotFound);

	FPRAbilityRuntimeState State;
	TestFalse(TEXT("Invalid AbilityTag query is rejected"),
		ASC->GetAbilityRuntimeStateByAbilityTag(FGameplayTag(), State));
	TestFalse(TEXT("Invalid InputTag query is rejected"),
		ASC->GetAbilityRuntimeStateByInputTag(FGameplayTag(), State));
	ASC->AbilityInputTagPressed(FGameplayTag());
	ASC->AbilityInputTagReleased(FGameplayTag());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAbilityCostCooldownEventTest,
	"ProjectR.Ability.Runtime.CostCooldownFailuresAndEvents",
	PRAbilityAutomation::TestFlags)

bool FPRAbilityCostCooldownEventTest::RunTest(const FString& Parameters)
{
	const UClass* AbilityClass = UPRGameplayAbility::StaticClass();
	TestTrue(TEXT("ProjectR ability is abstract"), AbilityClass->HasAnyClassFlags(CLASS_Abstract));
	const UPRGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UPRGameplayAbility>();
	TestEqual(TEXT("Ability is InstancedPerActor"), AbilityCDO->GetInstancingPolicy(),
		EGameplayAbilityInstancingPolicy::InstancedPerActor);

	const FStructProperty* AbilityTagProperty =
		PRAbilityAutomation::FindRequiredProperty<FStructProperty>(*this, AbilityClass, TEXT("AbilityTag"));
	const FEnumProperty* ActivationPolicyProperty =
		PRAbilityAutomation::FindRequiredProperty<FEnumProperty>(*this, AbilityClass, TEXT("ActivationPolicy"));
	const FBoolProperty* CanActivateWhileDeadProperty =
		PRAbilityAutomation::FindRequiredProperty<FBoolProperty>(*this, AbilityClass, TEXT("bCanActivateWhileDead"));
	const FBoolProperty* CancelOnDeathProperty =
		PRAbilityAutomation::FindRequiredProperty<FBoolProperty>(*this, AbilityClass, TEXT("bCancelOnDeath"));
	if (AbilityTagProperty && ActivationPolicyProperty
		&& CanActivateWhileDeadProperty && CancelOnDeathProperty)
	{
		const FGameplayTag* AbilityTag =
			AbilityTagProperty->ContainerPtrToValuePtr<FGameplayTag>(AbilityCDO);
		TestFalse(TEXT("Base AbilityTag defaults invalid"), AbilityTag->IsValid());
		TestEqual(TEXT("Default activation policy"),
			static_cast<EPRAbilityActivationPolicy>(
				ActivationPolicyProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(
					ActivationPolicyProperty->ContainerPtrToValuePtr<void>(AbilityCDO))),
			EPRAbilityActivationPolicy::OnInputTriggered);
		TestFalse(TEXT("Dead activation disabled by default"),
			CanActivateWhileDeadProperty->GetPropertyValue_InContainer(AbilityCDO));
		TestTrue(TEXT("Cancel on death enabled by default"),
			CancelOnDeathProperty->GetPropertyValue_InContainer(AbilityCDO));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAbilityDeathReviveReplacementTest,
	"ProjectR.Ability.Runtime.DeathReviveAndAvatarReplacement",
	PRAbilityAutomation::TestFlags)

bool FPRAbilityDeathReviveReplacementTest::RunTest(const FString& Parameters)
{
	UPRAbilitySystemComponent* ASC = NewObject<UPRAbilitySystemComponent>();
	int32 EventCount = 0;
	const FDelegateHandle Handle = ASC->OnAbilityLifecycleEvent().AddLambda(
		[&EventCount](const FPRAbilityLifecycleEvent& Event)
		{
			++EventCount;
		});
	ASC->HandleProjectRLifeStateChanged(UPRTagLibrary::GetStateDeadTag(), 1);
	ASC->HandleProjectRLifeStateChanged(UPRTagLibrary::GetStateDeadTag(), 0);
	TestEqual(TEXT("Life-state changes with no granted abilities emit no events"), EventCount, 0);
	ASC->OnAbilityLifecycleEvent().Remove(Handle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRAbilityAssetTest,
	"ProjectR.Ability.Assets.DefaultAndValidationAssets",
	PRAbilityAutomation::TestFlags)

bool FPRAbilityAssetTest::RunTest(const FString& Parameters)
{
	const TCHAR* RequiredAssets[] = {
		TEXT("/Game/ProjectR/Abilities/DA_DefaultAbilitySet.DA_DefaultAbilitySet"),
		TEXT("/Game/ProjectR/MCPTest/Abilities/GE_AbilityValidation_Cooldown.GE_AbilityValidation_Cooldown_C"),
		TEXT("/Game/ProjectR/MCPTest/Abilities/GE_AbilityValidation_EnergyCost.GE_AbilityValidation_EnergyCost_C"),
		TEXT("/Game/ProjectR/MCPTest/Abilities/GA_AbilityValidation_Triggered.GA_AbilityValidation_Triggered_C"),
		TEXT("/Game/ProjectR/MCPTest/Abilities/GA_AbilityValidation_Held.GA_AbilityValidation_Held_C"),
		TEXT("/Game/ProjectR/MCPTest/Abilities/GA_AbilityValidation_Passive.GA_AbilityValidation_Passive_C"),
		TEXT("/Game/ProjectR/MCPTest/Abilities/DA_AbilitySet_Validation.DA_AbilitySet_Validation")};
	for (const TCHAR* Path : RequiredAssets)
	{
		TestNotNull(*FString::Printf(TEXT("Required asset %s"), Path),
			StaticLoadObject(UObject::StaticClass(), nullptr, Path));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
