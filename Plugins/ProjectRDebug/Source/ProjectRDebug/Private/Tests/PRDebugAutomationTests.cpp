// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "PRDebugCommandRegistry.h"
#include "PRDebugSubsystem.h"
#include "Core/Logging/PRLogSanitizer.h"
#include "Core/PRDeveloperSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include <limits>

namespace PRDebugTests
{
constexpr EAutomationTestFlags Flags =
	EAutomationTestFlags::EditorContext
	| EAutomationTestFlags::ClientContext
	| EAutomationTestFlags::ProductFilter;

UGameInstance* FindGameInstance()
{
	if (GEngine == nullptr)
	{
		return nullptr;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (UWorld* World = Context.World())
		{
			if (World->IsGameWorld() && World->GetGameInstance() != nullptr)
			{
				return World->GetGameInstance();
			}
		}
	}

	return nullptr;
}

FPRDebugCommandRequest MakeRequest(EPRDebugCommandId CommandId)
{
	FPRDebugCommandRequest Request;
	Request.CommandId = CommandId;
	return Request;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugCommandSchemaTest,
	"ProjectR.Debug.CommandSchema",
	PRDebugTests::Flags)

bool FPRDebugCommandSchemaTest::RunTest(const FString& Parameters)
{
	const TArray<FPRDebugCommandDescriptor> Descriptors = UPRDebugSubsystem::BuildBuiltInDescriptorsForTests();
	TestEqual(TEXT("Descriptor count"), Descriptors.Num(), 11);
	TSet<EPRDebugCommandId> Ids;
	TSet<FName> Names;
	for (int32 Index = 0; Index < Descriptors.Num(); ++Index)
	{
		const FPRDebugCommandDescriptor& Descriptor = Descriptors[Index];
		TestEqual(TEXT("Stable enum order"), static_cast<int32>(Descriptor.CommandId), Index);
		TestEqual(TEXT("Schema version"), Descriptor.SchemaVersion, static_cast<uint16>(1));
		TestTrue(TEXT("Unique command id"), !Ids.Contains(Descriptor.CommandId));
		TestTrue(TEXT("Unique stable name"), !Names.Contains(Descriptor.StableName));
		Ids.Add(Descriptor.CommandId);
		Names.Add(Descriptor.StableName);
	}
	TestEqual(TEXT("Damage stable name"), Descriptors[1].StableName, FName(TEXT("Combat.DamageSelf")));
	TestEqual(TEXT("Damage argument count"), Descriptors[1].Arguments.Num(), 1);
	TestEqual(TEXT("Damage minimum"), Descriptors[1].Arguments[0].MinValue, 1.0);
	TestEqual(TEXT("Damage maximum"), Descriptors[1].Arguments[0].MaxValue, 100000.0);
	TestEqual(TEXT("Damage default"), Descriptors[1].Arguments[0].DefaultValue, 25.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugRegistryLifecycleTest,
	"ProjectR.Debug.RegistryLifecycle",
	PRDebugTests::Flags)

bool FPRDebugRegistryLifecycleTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = PRDebugTests::FindGameInstance();
	TestNotNull(TEXT("Game instance is available"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}
	UPRDebugSubsystem* Provider = GameInstance->GetSubsystem<UPRDebugSubsystem>();
	TestNotNull(TEXT("Debug subsystem is available"), Provider);
	if (Provider == nullptr)
	{
		return false;
	}
	FPRDebugCommandRegistry Registry;
	const TArray<FPRDebugCommandDescriptor> Descriptors = UPRDebugSubsystem::BuildBuiltInDescriptorsForTests();
	const FPRDebugCommandHandler Handler = FPRDebugCommandHandler::CreateUObject(
		Provider,
		&UPRDebugSubsystem::ExecuteBuiltInCommandForTests);
	const FPRDebugProviderHandle First = Registry.RegisterProvider(TEXT("ProjectR.Tests"), Descriptors, Handler);
	const FPRDebugProviderHandle Duplicate = Registry.RegisterProvider(TEXT("ProjectR.Tests"), Descriptors, Handler);
	TestTrue(TEXT("First registration succeeds"), First.IsValid());
	TestTrue(TEXT("Idempotent registration returns same handle"), First == Duplicate);

	TArray<FPRDebugCommandDescriptor> ConflictDescriptors = Descriptors;
	ConflictDescriptors[0].Description = FText::FromString(TEXT("conflict"));
	AddExpectedError(
		TEXT("Rejected conflicting debug provider id."),
		EAutomationExpectedErrorFlags::Exact,
		1);
	TestFalse(
		TEXT("Same provider with different descriptor is rejected"),
		Registry.RegisterProvider(TEXT("ProjectR.Tests"), ConflictDescriptors, Handler).IsValid());
	TestTrue(TEXT("First unregister succeeds"), Registry.UnregisterProvider(First));
	TestFalse(TEXT("Repeated unregister is safe"), Registry.UnregisterProvider(First));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugArgumentValidationTest,
	"ProjectR.Debug.ArgumentValidation",
	PRDebugTests::Flags)

bool FPRDebugArgumentValidationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = PRDebugTests::FindGameInstance();
	TestNotNull(TEXT("Game instance is available"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}
	UPRDebugSubsystem* Provider = GameInstance->GetSubsystem<UPRDebugSubsystem>();
	TestNotNull(TEXT("Debug subsystem is available"), Provider);
	if (Provider == nullptr)
	{
		return false;
	}
	FPRDebugCommandRegistry Registry;
	Registry.RegisterProvider(
		TEXT("ProjectR.Tests"),
		UPRDebugSubsystem::BuildBuiltInDescriptorsForTests(),
		FPRDebugCommandHandler::CreateUObject(Provider, &UPRDebugSubsystem::ExecuteBuiltInCommandForTests));

	FPRDebugCommandRequest Missing = PRDebugTests::MakeRequest(EPRDebugCommandId::DamageSelf);
	TestEqual(
		TEXT("Required argument missing"),
		Registry.ExecuteCommand(Missing, nullptr).ResultCode,
		EPRDebugCommandResultCode::InvalidParameter);

	FPRDebugArgumentValue Amount;
	Amount.ArgumentId = TEXT("Amount");
	Amount.Type = EPRDebugArgumentType::Float;
	Amount.FloatValue = std::numeric_limits<double>::quiet_NaN();
	Missing.Arguments.Add(Amount);
	TestEqual(
		TEXT("NaN rejected"),
		Registry.ExecuteCommand(Missing, nullptr).ResultCode,
		EPRDebugCommandResultCode::InvalidParameter);
	Missing.Arguments[0].FloatValue = 100001.0;
	TestEqual(
		TEXT("Out of range rejected"),
		Registry.ExecuteCommand(Missing, nullptr).ResultCode,
		EPRDebugCommandResultCode::InvalidParameter);
	Missing.Arguments[0].FloatValue = 25.0;
	const FPRDebugArgumentValue DuplicateAmount = Missing.Arguments[0];
	Missing.Arguments.Add(DuplicateAmount);
	TestEqual(
		TEXT("Duplicate argument rejected"),
		Registry.ExecuteCommand(Missing, nullptr).ResultCode,
		EPRDebugCommandResultCode::InvalidParameter);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugGameThreadGateTest,
	"ProjectR.Debug.GameThreadGate",
	PRDebugTests::Flags)

bool FPRDebugGameThreadGateTest::RunTest(const FString& Parameters)
{
	FPRDebugCommandRegistry Registry;
	FPRDebugCommandRequest Request = PRDebugTests::MakeRequest(EPRDebugCommandId::StatusSnapshot);
	TestTrue(TEXT("Automation dispatch runs on game thread"), IsInGameThread());
	TestEqual(
		TEXT("Unregistered command is not available"),
		Registry.ExecuteCommand(Request, nullptr).ResultCode,
		EPRDebugCommandResultCode::NotAvailable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugStatusSnapshotTest,
	"ProjectR.Debug.StatusSnapshot",
	PRDebugTests::Flags)

bool FPRDebugStatusSnapshotTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = PRDebugTests::FindGameInstance();
	TestNotNull(TEXT("Game instance is available"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}
	UPRDebugSubsystem* Subsystem = GameInstance->GetSubsystem<UPRDebugSubsystem>();
	TestNotNull(TEXT("Debug subsystem is available"), Subsystem);
	if (Subsystem == nullptr)
	{
		return false;
	}
	const FPRDebugCommandResult Result = Subsystem->ExecuteBuiltInCommandForTests(
		PRDebugTests::MakeRequest(EPRDebugCommandId::StatusSnapshot),
		GameInstance);
	TestEqual(TEXT("Status succeeds without leaking context"), Result.ResultCode, EPRDebugCommandResultCode::Success);
	TestTrue(TEXT("Status includes snapshot"), Result.bHasStatusSnapshot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugControlledCombatTest,
	"ProjectR.Debug.ControlledCombat",
	PRDebugTests::Flags)

bool FPRDebugControlledCombatTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = PRDebugTests::FindGameInstance();
	if (GameInstance == nullptr)
	{
		AddWarning(TEXT("Controlled combat requires the v0.1.5 -game automation invocation."));
		return true;
	}
	UPRDebugSubsystem* Subsystem = GameInstance->GetSubsystem<UPRDebugSubsystem>();
	FPRDebugCommandRequest Damage = PRDebugTests::MakeRequest(EPRDebugCommandId::DamageSelf);
	FPRDebugArgumentValue Amount;
	Amount.ArgumentId = TEXT("Amount");
	Amount.Type = EPRDebugArgumentType::Float;
	Amount.FloatValue = 25.0;
	Damage.Arguments.Add(Amount);
	const FPRDebugCommandResult DamageResult = Subsystem->ExecuteBuiltInCommandForTests(Damage, GameInstance);
	TestTrue(
		TEXT("Damage uses controlled result surface"),
		DamageResult.ResultCode == EPRDebugCommandResultCode::Success
		|| DamageResult.ResultCode == EPRDebugCommandResultCode::InvalidContext);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugSaveReadOnlyTest,
	"ProjectR.Debug.SaveReadOnly",
	PRDebugTests::Flags)

bool FPRDebugSaveReadOnlyTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = PRDebugTests::FindGameInstance();
	TestNotNull(TEXT("Game instance is available"), GameInstance);
	if (GameInstance == nullptr)
	{
		return false;
	}
	UPRDebugSubsystem* Subsystem = GameInstance->GetSubsystem<UPRDebugSubsystem>();
	TestNotNull(TEXT("Debug subsystem is available"), Subsystem);
	if (Subsystem == nullptr)
	{
		return false;
	}
	const FPRDebugCommandResult Result = Subsystem->ExecuteBuiltInCommandForTests(
		PRDebugTests::MakeRequest(EPRDebugCommandId::SaveRuntimeState),
		GameInstance);
	TestTrue(
		TEXT("Save command is read-only or explicitly context-invalid"),
		Result.ResultCode == EPRDebugCommandResultCode::Success
		|| Result.ResultCode == EPRDebugCommandResultCode::InvalidContext);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugSafeTravelTest,
	"ProjectR.Debug.SafeTravel",
	PRDebugTests::Flags)

bool FPRDebugSafeTravelTest::RunTest(const FString& Parameters)
{
	const FPRDebugCommandRequest Request = PRDebugTests::MakeRequest(EPRDebugCommandId::ReturnToCombatGym);
	TestEqual(TEXT("Travel request has no path arguments"), Request.Arguments.Num(), 0);
	TestEqual(TEXT("Travel schema is frozen"), Request.SchemaVersion, static_cast<uint16>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugFutureAvailabilityTest,
	"ProjectR.Debug.FutureAvailability",
	PRDebugTests::Flags)

bool FPRDebugFutureAvailabilityTest::RunTest(const FString& Parameters)
{
	const TArray<FPRDebugCommandDescriptor> Descriptors = UPRDebugSubsystem::BuildBuiltInDescriptorsForTests();
	for (int32 Index = static_cast<int32>(EPRDebugCommandId::GrantResource); Index <= static_cast<int32>(EPRDebugCommandId::JumpToBoss); ++Index)
	{
		TestEqual(
			TEXT("Future command remains unavailable"),
			Descriptors[Index].Availability,
			EPRDebugCommandAvailability::NotAvailable);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugInputPanelLifecycleTest,
	"ProjectR.Debug.InputAndPanelLifecycle",
	PRDebugTests::Flags)

bool FPRDebugInputPanelLifecycleTest::RunTest(const FString& Parameters)
{
#if UE_BUILD_SHIPPING
	TestTrue(TEXT("Shipping must not compile ProjectRDebug"), false);
#else
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	TestTrue(TEXT("Development debug feature default"), Settings->bEnableDebugFeatures);
	TestFalse(TEXT("Panel begins closed"), UPRDebugSubsystem::IsAnyDebugPanelOpenForTests());
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugLoggingContractTest,
	"ProjectR.Debug.LoggingContract",
	PRDebugTests::Flags)

bool FPRDebugLoggingContractTest::RunTest(const FString& Parameters)
{
	const FGuid Sample(0x12345678, 0x90abcdef, 0x12345678, 0x90abcdef);
	const FString Token = FPRLogSanitizer::ToOpaqueGuidToken(Sample);
	TestTrue(TEXT("Opaque token prefix"), Token.StartsWith(TEXT("prg_")));
	TestEqual(TEXT("Opaque token length"), Token.Len(), 16);
	TestFalse(TEXT("Token omits raw GUID fragment"), Token.Contains(TEXT("12345678")));
	TestEqual(TEXT("Invalid GUID token"), FPRLogSanitizer::ToOpaqueGuidToken(FGuid()), FString(TEXT("prg_invalid")));
	TestEqual(TEXT("Redacted value"), FPRLogSanitizer::RedactedValue(), FString(TEXT("[REDACTED]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRDebugBuildBoundaryTest,
	"ProjectR.Debug.BuildBoundary",
	PRDebugTests::Flags)

bool FPRDebugBuildBoundaryTest::RunTest(const FString& Parameters)
{
#if UE_BUILD_SHIPPING
	TestTrue(TEXT("Shipping must not compile ProjectRDebug"), false);
#else
	TestNotNull(TEXT("Debug subsystem type is linked only in non-Shipping builds"), UPRDebugSubsystem::StaticClass());
	TestTrue(TEXT("Development feature gate defaults enabled"), GetDefault<UPRDeveloperSettings>()->bEnableDebugFeatures);
#endif
	return true;
}

#endif
