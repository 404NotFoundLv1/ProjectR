// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Input/PRInputConfigDataAsset.h"

#include "Core/PRTagLibrary.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "UObject/SoftObjectPath.h"

namespace PRInputAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

UInputAction* MakeAction(EInputActionValueType ValueType)
{
	UInputAction* Action = NewObject<UInputAction>();
	Action->ValueType = ValueType;
	return Action;
}

FPRTaggedInputAction MakeTaggedAction(const FGameplayTag& InputTag, UInputAction* InputAction)
{
	FPRTaggedInputAction Result;
	Result.InputTag = InputTag;
	Result.InputAction = InputAction;
	return Result;
}

bool HasMapping(
	FAutomationTestBase& Test,
	const UInputMappingContext* MappingContext,
	const UInputAction* Action,
	const FKey& Key,
	bool bExpectNegate)
{
	int32 MatchCount = 0;
	for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
	{
		if (Mapping.Action != Action || Mapping.Key != Key)
		{
			continue;
		}

		++MatchCount;
		const bool bHasNegate = Mapping.Modifiers.ContainsByPredicate(
			[](const UInputModifier* Modifier)
			{
				return IsValid(Modifier) && Modifier->IsA<UInputModifierNegate>();
			});
		Test.TestEqual(TEXT("Mapping negate modifier"), bHasNegate, bExpectNegate);
	}

	return Test.TestEqual(TEXT("Required mapping count"), MatchCount, 1);
}
} // namespace PRInputAutomation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRInputConfigLookupTest,
	"ProjectR.Input.Config.Lookup",
	PRInputAutomation::TestFlags)

bool FPRInputConfigLookupTest::RunTest(const FString& Parameters)
{
	UPRInputConfigDataAsset* Config = NewObject<UPRInputConfigDataAsset>();
	UInputAction* MoveAction = PRInputAutomation::MakeAction(EInputActionValueType::Axis1D);
	Config->TaggedInputActions.Add(
		PRInputAutomation::MakeTaggedAction(UPRTagLibrary::GetInputMoveTag(), MoveAction));

	TestEqual(
		TEXT("Configured tag resolves its action"),
		Config->FindInputActionForTag(UPRTagLibrary::GetInputMoveTag()),
		static_cast<const UInputAction*>(MoveAction));

	AddExpectedError(TEXT("invalid InputTag"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("Invalid tag returns null"), Config->FindInputActionForTag(FGameplayTag()));

	AddExpectedError(TEXT("has no configured InputAction"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(
		TEXT("Missing tag returns null"),
		Config->FindInputActionForTag(UPRTagLibrary::GetInputLookTag()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRInputConfigValidationTest,
	"ProjectR.Input.Config.Validation",
	PRInputAutomation::TestFlags)

bool FPRInputConfigValidationTest::RunTest(const FString& Parameters)
{
	UPRInputConfigDataAsset* Config = NewObject<UPRInputConfigDataAsset>();
	{
		FDataValidationContext Context;
		TestEqual(
			TEXT("Missing mapping context is invalid"),
			Config->IsDataValid(Context),
			EDataValidationResult::Invalid);
	}

	Config->DefaultMappingContext = NewObject<UInputMappingContext>();
	UInputAction* MoveAction = PRInputAutomation::MakeAction(EInputActionValueType::Axis1D);
	UInputAction* JumpAction = PRInputAutomation::MakeAction(EInputActionValueType::Boolean);
	Config->TaggedInputActions = {
		PRInputAutomation::MakeTaggedAction(UPRTagLibrary::GetInputMoveTag(), MoveAction),
		PRInputAutomation::MakeTaggedAction(UPRTagLibrary::GetInputJumpTag(), JumpAction)};
	{
		FDataValidationContext Context;
		TestEqual(
			TEXT("Valid baseline passes"),
			Config->IsDataValid(Context),
			EDataValidationResult::Valid);
	}

	Config->TaggedInputActions.Add(
		PRInputAutomation::MakeTaggedAction(UPRTagLibrary::GetInputMoveTag(), JumpAction));
	{
		FDataValidationContext Context;
		TestEqual(
			TEXT("Duplicate tag is invalid"),
			Config->IsDataValid(Context),
			EDataValidationResult::Invalid);
	}

	Config->TaggedInputActions.SetNum(2);
	Config->TaggedInputActions[1].InputAction = MoveAction;
	{
		FDataValidationContext Context;
		TestEqual(
			TEXT("Duplicate action is invalid"),
			Config->IsDataValid(Context),
			EDataValidationResult::Invalid);
	}

	Config->TaggedInputActions[1].InputAction = JumpAction;
	MoveAction->ValueType = EInputActionValueType::Boolean;
	{
		FDataValidationContext Context;
		TestEqual(
			TEXT("Move must be Axis1D"),
			Config->IsDataValid(Context),
			EDataValidationResult::Invalid);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRInputRequiredAssetsTest,
	"ProjectR.Input.Assets.RequiredBaseline",
	PRInputAutomation::TestFlags)

bool FPRInputRequiredAssetsTest::RunTest(const FString& Parameters)
{
	const TCHAR* ConfigPath =
		TEXT("/Game/ProjectR/Input/DA_PlayerInputConfig.DA_PlayerInputConfig");
	UPRInputConfigDataAsset* Config = LoadObject<UPRInputConfigDataAsset>(nullptr, ConfigPath);
	if (!TestNotNull(TEXT("Player input config exists"), Config))
	{
		return false;
	}

	TestNotNull(TEXT("Default mapping context exists"), Config->DefaultMappingContext.Get());
	TestEqual(TEXT("Mapping priority"), Config->MappingPriority, 0);

	struct FRequiredAction
	{
		FGameplayTag Tag;
		const TCHAR* Path;
		EInputActionValueType ValueType;
	};

	const FRequiredAction RequiredActions[] = {
		{UPRTagLibrary::GetInputMoveTag(), TEXT("/Game/ProjectR/Input/Actions/IA_Move.IA_Move"), EInputActionValueType::Axis1D},
		{UPRTagLibrary::GetInputJumpTag(), TEXT("/Game/ProjectR/Input/Actions/IA_Jump.IA_Jump"), EInputActionValueType::Boolean},
		{UPRTagLibrary::GetInputAttackTag(), TEXT("/Game/ProjectR/Input/Actions/IA_Attack.IA_Attack"), EInputActionValueType::Boolean},
		{UPRTagLibrary::GetInputDodgeTag(), TEXT("/Game/ProjectR/Input/Actions/IA_Dodge.IA_Dodge"), EInputActionValueType::Boolean},
		{UPRTagLibrary::GetInputInteractTag(), TEXT("/Game/ProjectR/Input/Actions/IA_Interact.IA_Interact"), EInputActionValueType::Boolean},
		{UPRTagLibrary::GetInputExecuteTag(), TEXT("/Game/ProjectR/Input/Actions/IA_Execute.IA_Execute"), EInputActionValueType::Boolean}};

	TMap<FString, UInputAction*> Actions;
	for (const FRequiredAction& Required : RequiredActions)
	{
		UInputAction* Action = LoadObject<UInputAction>(nullptr, Required.Path);
		if (!TestNotNull(*FString::Printf(TEXT("Action exists: %s"), Required.Path), Action))
		{
			continue;
		}
		TestEqual(TEXT("Action value type"), Action->ValueType, Required.ValueType);
		TestEqual(
			TEXT("Config lookup"),
			Config->FindInputActionForTag(Required.Tag),
			static_cast<const UInputAction*>(Action));
		Actions.Add(FPackageName::ObjectPathToPackageName(FString(Required.Path)), Action);
	}

	if (Config->DefaultMappingContext)
	{
		using namespace PRInputAutomation;
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Move")), EKeys::A, true);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Move")), EKeys::D, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Move")), EKeys::Gamepad_LeftX, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Jump")), EKeys::SpaceBar, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Jump")), EKeys::Gamepad_FaceButton_Bottom, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Attack")), EKeys::LeftMouseButton, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Attack")), EKeys::Gamepad_FaceButton_Left, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Dodge")), EKeys::LeftShift, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Dodge")), EKeys::Gamepad_FaceButton_Right, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Interact")), EKeys::E, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Interact")), EKeys::Gamepad_FaceButton_Top, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Execute")), EKeys::F, false);
		HasMapping(*this, Config->DefaultMappingContext, Actions.FindRef(TEXT("/Game/ProjectR/Input/Actions/IA_Execute")), EKeys::Gamepad_RightShoulder, false);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
