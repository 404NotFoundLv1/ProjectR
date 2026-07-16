// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAttributeSet.h"
#include "Abilities/Player/PRPlayerSkillDataAsset.h"
#include "Abilities/Player/PRPlayerSkillFragment.h"
#include "Abilities/Player/PRPlayerSkillGameplayAbility.h"
#include "Input/PRInputConfigDataAsset.h"

#include "GameplayEffect.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/UnrealType.h"

namespace PRPlayerSkillAssetAutomation
{
struct FSkillExpectation
{
	const TCHAR* Name;
	const TCHAR* InputTag;
	EPRAbilityActivationPolicy ActivationPolicy;
	EPRPlayerSkillTargetPolicy TargetPolicy;
	float BaseDamage;
	float AttackPowerScale;
	float EnergyCost;
	float CooldownDuration;
	float StartupDuration;
	float ActiveDuration;
	float RecoveryDuration;
	float Range;
	float Radius;
	float HalfAngleDegrees;
	float DisplacementDistance;
	float StatusDuration;
	UClass* FragmentClass;
};

const FSkillExpectation SkillExpectations[] = {
	{TEXT("ShadowThrust"), TEXT("Input.Skill.ShadowThrust"), EPRAbilityActivationPolicy::OnInputTriggered,
		EPRPlayerSkillTargetPolicy::ForwardSweep, 25.0f, 1.5f, 20.0f, 4.0f,
		0.08f, 0.18f, 0.12f, 450.0f, 75.0f, 0.0f, 450.0f, 0.0f, nullptr},
	{TEXT("FireSlash"), TEXT("Input.Skill.FireSlash"), EPRAbilityActivationPolicy::OnInputTriggered,
		EPRPlayerSkillTargetPolicy::ForwardArea, 15.0f, 1.0f, 25.0f, 5.0f,
		0.15f, 0.0f, 0.20f, 360.0f, 0.0f, 60.0f, 0.0f, 1.5f,
		UPRFireSlashSkillFragment::StaticClass()},
	{TEXT("ThunderDrop"), TEXT("Input.Skill.ThunderDrop"), EPRAbilityActivationPolicy::OnInputTriggered,
		EPRPlayerSkillTargetPolicy::GroundArea, 25.0f, 2.0f, 30.0f, 7.0f,
		0.0f, 0.60f, 0.0f, 700.0f, 220.0f, 0.0f, 0.0f, 0.75f,
		UPRThunderDropSkillFragment::StaticClass()},
	{TEXT("AfterimageDodge"), TEXT("Input.Dodge"), EPRAbilityActivationPolicy::OnInputTriggered,
		EPRPlayerSkillTargetPolicy::Self, 0.0f, 0.0f, 15.0f, 2.5f,
		0.0f, 0.18f, 0.04f, 0.0f, 0.0f, 0.0f, 300.0f, 0.22f,
		UPRAfterimageDodgeSkillFragment::StaticClass()},
	{TEXT("VectorHook"), TEXT("Input.Skill.VectorHook"), EPRAbilityActivationPolicy::OnInputTriggered,
		EPRPlayerSkillTargetPolicy::SingleTarget, 0.0f, 0.0f, 15.0f, 4.0f,
		0.0f, 0.30f, 0.0f, 800.0f, 0.0f, 45.0f, 0.0f, 0.0f,
		UPRVectorHookSkillFragment::StaticClass()},
	{TEXT("CounterProofWall"), TEXT("Input.Skill.CounterProofWall"), EPRAbilityActivationPolicy::WhileInputActive,
		EPRPlayerSkillTargetPolicy::Self, 0.0f, 0.0f, 20.0f, 6.0f,
		0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 60.0f, 0.0f, 1.0f,
		UPRCounterProofWallSkillFragment::StaticClass()}};

const UInputAction* LoadInputAction(const TCHAR* Name)
{
	return LoadObject<UInputAction>(nullptr, *FString::Printf(
		TEXT("/Game/ProjectR/Input/Actions/%s.%s"), Name, Name));
}
} // namespace PRPlayerSkillAssetAutomation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPlayerSkillAssetTest,
	"ProjectR.PlayerSkill.Assets.InputAbilitySetAndEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRPlayerSkillAssetTest::RunTest(const FString& Parameters)
{
	using namespace PRPlayerSkillAssetAutomation;
	const UPRInputConfigDataAsset* InputConfig = LoadObject<UPRInputConfigDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Input/DA_PlayerInputConfig.DA_PlayerInputConfig"));
	const UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
		nullptr, TEXT("/Game/ProjectR/Input/IMC_Player.IMC_Player"));
	const UPRAbilitySetDataAsset* DefaultSet = LoadObject<UPRAbilitySetDataAsset>(
		nullptr, TEXT("/Game/ProjectR/Abilities/DA_DefaultAbilitySet.DA_DefaultAbilitySet"));
	if (!TestNotNull(TEXT("Input config exists"), InputConfig)
		|| !TestNotNull(TEXT("Mapping context exists"), MappingContext)
		|| !TestNotNull(TEXT("Default ability set exists"), DefaultSet))
	{
		return false;
	}

	const TCHAR* ExpectedInputTags[] = {
		TEXT("Input.Move"), TEXT("Input.Jump"), TEXT("Input.Attack"), TEXT("Input.Dodge"),
		TEXT("Input.Interact"), TEXT("Input.Execute"), TEXT("Input.Skill.ShadowThrust"),
		TEXT("Input.Skill.FireSlash"), TEXT("Input.Skill.ThunderDrop"),
		TEXT("Input.Skill.VectorHook"), TEXT("Input.Skill.CounterProofWall")};
	const TCHAR* ExpectedInputActions[] = {
		TEXT("IA_Move"), TEXT("IA_Jump"), TEXT("IA_Attack"), TEXT("IA_Dodge"),
		TEXT("IA_Interact"), TEXT("IA_Execute"), TEXT("IA_Skill_ShadowThrust"),
		TEXT("IA_Skill_FireSlash"), TEXT("IA_Skill_ThunderDrop"),
		TEXT("IA_Skill_VectorHook"), TEXT("IA_Skill_CounterProofWall")};
	TestEqual(TEXT("Input config contains eleven semantic actions"), InputConfig->TaggedInputActions.Num(), 11);
	for (int32 Index = 0; Index < FMath::Min(InputConfig->TaggedInputActions.Num(), 11); ++Index)
	{
		const FPRTaggedInputAction& Entry = InputConfig->TaggedInputActions[Index];
		TestEqual(*FString::Printf(TEXT("Input tag %d"), Index), Entry.InputTag.ToString(), FString(ExpectedInputTags[Index]));
		TestEqual(*FString::Printf(TEXT("Input action %d"), Index),
			static_cast<const UInputAction*>(Entry.InputAction.Get()), LoadInputAction(ExpectedInputActions[Index]));
	}
	for (int32 Index = 6; Index < 11; ++Index)
	{
		const UInputAction* Action = InputConfig->TaggedInputActions[Index].InputAction;
		if (TestNotNull(*FString::Printf(TEXT("Formal skill IA %d"), Index), Action))
		{
			TestEqual(*FString::Printf(TEXT("Formal skill IA %d is Boolean"), Index),
				Action->ValueType, EInputActionValueType::Boolean);
		}
	}

	const TArray<FEnhancedActionKeyMapping>& Mappings = MappingContext->GetMappings();
	const FKey ExpectedKeys[] = {
		EKeys::A, EKeys::D, EKeys::Gamepad_LeftX, EKeys::SpaceBar,
		EKeys::Gamepad_FaceButton_Bottom, EKeys::LeftMouseButton,
		EKeys::Gamepad_FaceButton_Left, EKeys::LeftShift,
		EKeys::Gamepad_FaceButton_Right, EKeys::E, EKeys::Gamepad_FaceButton_Top,
		EKeys::F, EKeys::Gamepad_RightShoulder, EKeys::J, EKeys::K, EKeys::L,
		EKeys::U, EKeys::Gamepad_LeftShoulder, EKeys::I, EKeys::Gamepad_RightTrigger,
		EKeys::O, EKeys::Gamepad_LeftTrigger, EKeys::H, EKeys::Gamepad_DPad_Up,
		EKeys::Semicolon, EKeys::Gamepad_DPad_Down};
	const TCHAR* ExpectedMappingActions[] = {
		TEXT("IA_Move"), TEXT("IA_Move"), TEXT("IA_Move"), TEXT("IA_Jump"), TEXT("IA_Jump"),
		TEXT("IA_Attack"), TEXT("IA_Attack"), TEXT("IA_Dodge"), TEXT("IA_Dodge"),
		TEXT("IA_Interact"), TEXT("IA_Interact"), TEXT("IA_Execute"), TEXT("IA_Execute"),
		TEXT("IA_Attack"), TEXT("IA_Jump"), TEXT("IA_Dodge"),
		TEXT("IA_Skill_ShadowThrust"), TEXT("IA_Skill_ShadowThrust"),
		TEXT("IA_Skill_FireSlash"), TEXT("IA_Skill_FireSlash"),
		TEXT("IA_Skill_ThunderDrop"), TEXT("IA_Skill_ThunderDrop"),
		TEXT("IA_Skill_VectorHook"), TEXT("IA_Skill_VectorHook"),
		TEXT("IA_Skill_CounterProofWall"), TEXT("IA_Skill_CounterProofWall")};
	TestEqual(TEXT("IMC contains twenty-six mappings"), Mappings.Num(), 26);
	for (int32 Index = 0; Index < FMath::Min(Mappings.Num(), 26); ++Index)
	{
		TestEqual(*FString::Printf(TEXT("IMC key %d"), Index), Mappings[Index].Key, ExpectedKeys[Index]);
		TestEqual(*FString::Printf(TEXT("IMC action %d"), Index), Mappings[Index].Action.Get(), LoadInputAction(ExpectedMappingActions[Index]));
		TestEqual(*FString::Printf(TEXT("IMC trigger count %d"), Index), Mappings[Index].Triggers.Num(), 0);
		TestEqual(*FString::Printf(TEXT("IMC modifier count %d"), Index), Mappings[Index].Modifiers.Num(), Index == 0 ? 1 : 0);
	}
	if (Mappings.Num() > 0 && Mappings[0].Modifiers.Num() == 1)
	{
		TestTrue(TEXT("A mapping retains Negate"), Mappings[0].Modifiers[0]->IsA<UInputModifierNegate>());
	}

	const UClass* BurningEffectClass = LoadClass<UGameplayEffect>(
		nullptr,
		TEXT("/Game/ProjectR/Abilities/Effects/GE_State_Burning.GE_State_Burning_C"));
	const UGameplayEffect* BurningEffect = BurningEffectClass
		? BurningEffectClass->GetDefaultObject<UGameplayEffect>()
		: nullptr;
	if (TestNotNull(TEXT("Checkpoint B Burning effect exists"), BurningEffect))
	{
		float BurningDuration = 0.0f;
		TestEqual(TEXT("Burning has duration"), BurningEffect->DurationPolicy,
			EGameplayEffectDurationType::HasDuration);
		TestTrue(TEXT("Burning duration is static"),
			BurningEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(1.0f, BurningDuration));
		TestEqual(TEXT("Burning duration is 1.5 seconds"), BurningDuration, 1.5f);
		TestEqual(TEXT("Burning has no period"), BurningEffect->Period.GetValueAtLevel(1.0f), 0.0f);
		TestEqual(TEXT("Burning has no modifiers"), BurningEffect->Modifiers.Num(), 0);
		TestEqual(TEXT("Burning has no executions"), BurningEffect->Executions.Num(), 0);
		TestEqual(TEXT("Burning grants exactly one tag"), BurningEffect->GetGrantedTags().Num(), 1);
		TestTrue(TEXT("Burning grants State.Burning"), BurningEffect->GetGrantedTags().HasTagExact(
			FGameplayTag::RequestGameplayTag(TEXT("State.Burning"))));
		TestEqual(TEXT("Burning aggregates by target"), BurningEffect->GetStackingType(),
			EGameplayEffectStackingType::AggregateByTarget);
		TestEqual(TEXT("Burning stack limit is one"), BurningEffect->StackLimitCount, 1);
		TestEqual(TEXT("Burning refreshes duration"), BurningEffect->StackDurationRefreshPolicy,
			EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication);
		TestEqual(TEXT("Burning never resets a period"), BurningEffect->StackPeriodResetPolicy,
			EGameplayEffectStackingPeriodPolicy::NeverReset);
		TestEqual(TEXT("Burning clears its entire stack"), BurningEffect->StackExpirationPolicy,
			EGameplayEffectStackingExpirationPolicy::ClearEntireStack);
	}

	struct FStateEffectExpectation
	{
		const TCHAR* Name;
		float Duration;
		const TCHAR* Tag;
	};
	const FStateEffectExpectation StateEffects[] = {
		{TEXT("Stunned"), 0.75f, TEXT("State.Stunned")},
		{TEXT("Invulnerable"), 0.22f, TEXT("State.Invulnerable")},
		{TEXT("Guarding"), 1.0f, TEXT("State.Guarding")}};
	const UClass* StateEffectClasses[UE_ARRAY_COUNT(StateEffects)] = {};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(StateEffects); ++Index)
	{
		const FStateEffectExpectation& Expected = StateEffects[Index];
		StateEffectClasses[Index] = LoadClass<UGameplayEffect>(
			nullptr,
			*FString::Printf(TEXT("/Game/ProjectR/Abilities/Effects/GE_State_%s.GE_State_%s_C"),
				Expected.Name, Expected.Name));
		const UGameplayEffect* StateEffect = StateEffectClasses[Index]
			? StateEffectClasses[Index]->GetDefaultObject<UGameplayEffect>() : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("C State %s effect exists"), Expected.Name), StateEffect))
		{
			continue;
		}
		float Duration = 0.0f;
		TestEqual(*FString::Printf(TEXT("C State %s has duration"), Expected.Name),
			StateEffect->DurationPolicy, EGameplayEffectDurationType::HasDuration);
		TestTrue(*FString::Printf(TEXT("C State %s duration is static"), Expected.Name),
			StateEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(1.0f, Duration));
		TestEqual(*FString::Printf(TEXT("C State %s duration"), Expected.Name), Duration, Expected.Duration);
		TestEqual(*FString::Printf(TEXT("C State %s has no period"), Expected.Name),
			StateEffect->Period.GetValueAtLevel(1.0f), 0.0f);
		TestEqual(*FString::Printf(TEXT("C State %s has no modifiers"), Expected.Name), StateEffect->Modifiers.Num(), 0);
		TestEqual(*FString::Printf(TEXT("C State %s has no executions"), Expected.Name), StateEffect->Executions.Num(), 0);
		TestEqual(*FString::Printf(TEXT("C State %s has no cues"), Expected.Name), StateEffect->GameplayCues.Num(), 0);
		TestEqual(*FString::Printf(TEXT("C State %s grants one tag"), Expected.Name), StateEffect->GetGrantedTags().Num(), 1);
		TestTrue(*FString::Printf(TEXT("C State %s grants expected tag"), Expected.Name),
			StateEffect->GetGrantedTags().HasTagExact(FGameplayTag::RequestGameplayTag(Expected.Tag)));
	}

	const TArray<FPRAbilitySetEntry>& StartupEntries = DefaultSet->GetAbilityEntries();
	TestEqual(TEXT("Checkpoint D default ability set contains six entries"), StartupEntries.Num(), 6);
	const TCHAR* ExpectedStartupSkills[] = {
		TEXT("ShadowThrust"), TEXT("FireSlash"), TEXT("ThunderDrop"), TEXT("AfterimageDodge"),
		TEXT("VectorHook"), TEXT("CounterProofWall")};
	const TCHAR* ExpectedStartupInputs[] = {
		TEXT("Input.Skill.ShadowThrust"), TEXT("Input.Skill.FireSlash"),
		TEXT("Input.Skill.ThunderDrop"), TEXT("Input.Dodge"),
		TEXT("Input.Skill.VectorHook"), TEXT("Input.Skill.CounterProofWall")};
	for (int32 Index = 0; Index < FMath::Min(StartupEntries.Num(), 6); ++Index)
	{
		const FPRAbilitySetEntry& Entry = StartupEntries[Index];
		const FString SkillName(ExpectedStartupSkills[Index]);
		const UClass* ExpectedAbilityClass = LoadClass<UPRPlayerSkillGameplayAbility>(
			nullptr,
			*FString::Printf(TEXT("/Game/ProjectR/Abilities/Skills/GA_Skill_%s.GA_Skill_%s_C"),
				*SkillName, *SkillName));
		const UPRPlayerSkillDataAsset* ExpectedData = LoadObject<UPRPlayerSkillDataAsset>(
			nullptr,
			*FString::Printf(TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_%s.DA_Skill_%s"),
				*SkillName, *SkillName));
		TestTrue(*FString::Printf(TEXT("Startup entry %d ability"), Index),
			Entry.AbilityClass.Get() == ExpectedAbilityClass);
		TestEqual(*FString::Printf(TEXT("Startup entry %d level"), Index), Entry.AbilityLevel, 1);
		TestEqual(*FString::Printf(TEXT("Startup entry %d input"), Index),
			Entry.InputTag.ToString(), FString(ExpectedStartupInputs[Index]));
		TestTrue(*FString::Printf(TEXT("Startup entry %d grants on initialization"), Index),
			Entry.bGrantOnInitialization);
		TestTrue(*FString::Printf(TEXT("Startup entry %d has no extra spec tags"), Index),
			Entry.GrantedSpecTags.IsEmpty());
		TestTrue(*FString::Printf(TEXT("Startup entry %d data"), Index),
			Entry.AbilityData.Get() == static_cast<const UPrimaryDataAsset*>(ExpectedData));
	}
	for (const FSkillExpectation& Expected : SkillExpectations)
	{
		const FString DataPath = FString::Printf(
			TEXT("/Game/ProjectR/Abilities/Skills/DA_Skill_%s.DA_Skill_%s"), Expected.Name, Expected.Name);
		const UPRPlayerSkillDataAsset* Skill = LoadObject<UPRPlayerSkillDataAsset>(nullptr, *DataPath);
		if (!TestNotNull(*DataPath, Skill))
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s primary type"), Expected.Name),
			Skill->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("ProjectRPlayerSkill")));
		TestEqual(*FString::Printf(TEXT("%s primary name"), Expected.Name),
			Skill->GetPrimaryAssetId().PrimaryAssetName, Skill->GetFName());
		TestEqual(*FString::Printf(TEXT("%s ability tag"), Expected.Name),
			Skill->AbilityTag.ToString(), FString::Printf(TEXT("Skill.%s"), Expected.Name));
		TestEqual(*FString::Printf(TEXT("%s input tag"), Expected.Name), Skill->InputTag.ToString(), FString(Expected.InputTag));
		TestEqual(*FString::Printf(TEXT("%s activation"), Expected.Name), Skill->ActivationPolicy, Expected.ActivationPolicy);
		TestEqual(*FString::Printf(TEXT("%s target policy"), Expected.Name), Skill->TargetPolicy, Expected.TargetPolicy);
		TestEqual(*FString::Printf(TEXT("%s base damage"), Expected.Name), Skill->BaseDamage, Expected.BaseDamage);
		TestEqual(*FString::Printf(TEXT("%s AP scale"), Expected.Name), Skill->AttackPowerScale, Expected.AttackPowerScale);
		TestEqual(*FString::Printf(TEXT("%s energy"), Expected.Name), Skill->EnergyCost, Expected.EnergyCost);
		TestEqual(*FString::Printf(TEXT("%s cooldown"), Expected.Name), Skill->CooldownDuration, Expected.CooldownDuration);
		TestEqual(*FString::Printf(TEXT("%s startup"), Expected.Name), Skill->StartupDuration, Expected.StartupDuration);
		TestEqual(*FString::Printf(TEXT("%s active"), Expected.Name), Skill->ActiveDuration, Expected.ActiveDuration);
		TestEqual(*FString::Printf(TEXT("%s recovery"), Expected.Name), Skill->RecoveryDuration, Expected.RecoveryDuration);
		TestEqual(*FString::Printf(TEXT("%s range"), Expected.Name), Skill->Range, Expected.Range);
		TestEqual(*FString::Printf(TEXT("%s radius"), Expected.Name), Skill->Radius, Expected.Radius);
		TestEqual(*FString::Printf(TEXT("%s half angle"), Expected.Name), Skill->HalfAngleDegrees, Expected.HalfAngleDegrees);
		TestEqual(*FString::Printf(TEXT("%s displacement"), Expected.Name), Skill->DisplacementDistance, Expected.DisplacementDistance);
		TestEqual(*FString::Printf(TEXT("%s status"), Expected.Name), Skill->StatusDuration, Expected.StatusDuration);
		TestEqual(*FString::Printf(TEXT("%s fragment class"), Expected.Name),
			Skill->SkillFragment ? Skill->SkillFragment->GetClass() : nullptr, Expected.FragmentClass);
		TestTrue(*FString::Printf(TEXT("%s Montage remains intentionally empty"), Expected.Name), Skill->Montage.IsNull());
		const FString ExpectedVFXPath = FString::Printf(
			TEXT("/Game/ProjectR/VFX/Skills/VFX_%s.VFX_%s"), Expected.Name, Expected.Name);
		const FString ExpectedSFXPath = FString::Printf(
			TEXT("/Game/ProjectR/Audio/Skills/SFX_%s.SFX_%s"), Expected.Name, Expected.Name);
		TestFalse(*FString::Printf(TEXT("%s VFX reference is configured"), Expected.Name), Skill->VFX.IsNull());
		TestEqual(*FString::Printf(TEXT("%s VFX soft path"), Expected.Name),
			Skill->VFX.ToSoftObjectPath().ToString(), ExpectedVFXPath);
		TestTrue(*FString::Printf(TEXT("%s VFX class"), Expected.Name),
			Skill->VFX.LoadSynchronous() && Skill->VFX.Get()->IsA<UNiagaraSystem>());
		TestFalse(*FString::Printf(TEXT("%s SFX reference is configured"), Expected.Name), Skill->SFX.IsNull());
		TestEqual(*FString::Printf(TEXT("%s SFX soft path"), Expected.Name),
			Skill->SFX.ToSoftObjectPath().ToString(), ExpectedSFXPath);
		TestTrue(*FString::Printf(TEXT("%s SFX class"), Expected.Name),
			Skill->SFX.LoadSynchronous() && Skill->SFX.Get()->IsA<USoundBase>());

		const UPRPlayerSkillGameplayAbility* AbilityCDO = Skill->AbilityClass
			? Skill->AbilityClass->GetDefaultObject<UPRPlayerSkillGameplayAbility>() : nullptr;
		if (TestNotNull(*FString::Printf(TEXT("%s GA CDO"), Expected.Name), AbilityCDO))
		{
			TestEqual(*FString::Printf(TEXT("%s DA to GA tag"), Expected.Name), AbilityCDO->GetProjectRAbilityTag(), Skill->AbilityTag);
			TestEqual(*FString::Printf(TEXT("%s DA to GA policy"), Expected.Name), AbilityCDO->GetActivationPolicy(), Skill->ActivationPolicy);
			TestEqual(*FString::Printf(TEXT("%s DA to GA Cost"), Expected.Name),
				AbilityCDO->GetCostGameplayEffect()->GetClass(), Skill->CostEffectClass.Get());
			TestEqual(*FString::Printf(TEXT("%s DA to GA Cooldown"), Expected.Name),
				AbilityCDO->GetCooldownGameplayEffect()->GetClass(), Skill->CooldownEffectClass.Get());
			if (FString(Expected.Name) == TEXT("FireSlash"))
			{
				const FClassProperty* BurningProperty = FindFProperty<FClassProperty>(
					AbilityCDO->GetClass(), TEXT("BurningEffectClass"));
				if (TestNotNull(TEXT("FireSlash exposes its private Burning asset binding"), BurningProperty))
				{
					const UClass* ConfiguredBurningClass = Cast<UClass>(
						BurningProperty->GetObjectPropertyValue_InContainer(AbilityCDO));
					TestTrue(TEXT("FireSlash references formal Burning GE"),
						ConfiguredBurningClass == BurningEffectClass);
				}
			}
			else if (FString(Expected.Name) == TEXT("ThunderDrop")
				|| FString(Expected.Name) == TEXT("AfterimageDodge")
				|| FString(Expected.Name) == TEXT("CounterProofWall"))
			{
				const bool bThunder = FString(Expected.Name) == TEXT("ThunderDrop");
				const bool bDodge = FString(Expected.Name) == TEXT("AfterimageDodge");
				const FClassProperty* StateProperty = FindFProperty<FClassProperty>(
					AbilityCDO->GetClass(), bThunder ? TEXT("StunnedEffectClass")
						: bDodge ? TEXT("InvulnerableEffectClass") : TEXT("GuardingEffectClass"));
				if (TestNotNull(*FString::Printf(TEXT("%s exposes its private State GE binding"), Expected.Name), StateProperty))
				{
					const UClass* ConfiguredStateClass = Cast<UClass>(
						StateProperty->GetObjectPropertyValue_InContainer(AbilityCDO));
					TestTrue(*FString::Printf(TEXT("%s references its formal State GE"), Expected.Name),
						ConfiguredStateClass == StateEffectClasses[bThunder ? 0 : bDodge ? 1 : 2]);
				}
			}
		}

		const UGameplayEffect* Cost = Skill->CostEffectClass->GetDefaultObject<UGameplayEffect>();
		float CostMagnitude = 0.0f;
		TestEqual(*FString::Printf(TEXT("%s Cost instant"), Expected.Name), Cost->DurationPolicy, EGameplayEffectDurationType::Instant);
		TestEqual(*FString::Printf(TEXT("%s Cost modifier count"), Expected.Name), Cost->Modifiers.Num(), 1);
		if (Cost->Modifiers.Num() == 1)
		{
			TestEqual(*FString::Printf(TEXT("%s Cost Energy"), Expected.Name), Cost->Modifiers[0].Attribute, UPRAttributeSet::GetEnergyAttribute());
			TestEqual(*FString::Printf(TEXT("%s Cost additive"), Expected.Name), Cost->Modifiers[0].ModifierOp, EGameplayModOp::Additive);
			TestTrue(*FString::Printf(TEXT("%s Cost static"), Expected.Name),
				Cost->Modifiers[0].ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, CostMagnitude));
			TestEqual(*FString::Printf(TEXT("%s Cost magnitude"), Expected.Name), CostMagnitude, -Expected.EnergyCost);
		}
		const UGameplayEffect* Cooldown = Skill->CooldownEffectClass->GetDefaultObject<UGameplayEffect>();
		float CooldownMagnitude = 0.0f;
		TestEqual(*FString::Printf(TEXT("%s Cooldown duration policy"), Expected.Name), Cooldown->DurationPolicy, EGameplayEffectDurationType::HasDuration);
		TestTrue(*FString::Printf(TEXT("%s Cooldown static"), Expected.Name),
			Cooldown->DurationMagnitude.GetStaticMagnitudeIfPossible(1.0f, CooldownMagnitude));
		TestEqual(*FString::Printf(TEXT("%s Cooldown magnitude"), Expected.Name), CooldownMagnitude, Expected.CooldownDuration);
		TestEqual(*FString::Printf(TEXT("%s Cooldown modifier count"), Expected.Name), Cooldown->Modifiers.Num(), 0);
		TestEqual(*FString::Printf(TEXT("%s Cooldown tag count"), Expected.Name), Cooldown->GetGrantedTags().Num(), 1);
		TestTrue(*FString::Printf(TEXT("%s Cooldown tag"), Expected.Name), Cooldown->GetGrantedTags().HasTagExact(
			FGameplayTag::RequestGameplayTag(*FString::Printf(TEXT("Cooldown.Skill.%s"), Expected.Name))));

		FDataValidationContext ValidationContext;
		TestEqual(*FString::Printf(TEXT("%s Data Validation"), Expected.Name),
			Skill->IsDataValid(ValidationContext), EDataValidationResult::Valid);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
