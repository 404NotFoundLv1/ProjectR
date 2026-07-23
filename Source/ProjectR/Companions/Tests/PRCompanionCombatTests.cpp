// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Companions/PRCompanionRuntimeDataAsset.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Companions/PRCompanionTypes.h"
#include "GameplayEffect.h"

#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"

namespace PRCompanionCombatAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionCombatContractTest,
	"ProjectR.Companion.Combat.SupportContractAndOverload",
	PRCompanionCombatAutomation::TestFlags)

bool FPRCompanionCombatContractTest::RunTest(const FString& Parameters)
{
	UPRCompanionRuntimeDataAsset* RuntimeData = NewObject<UPRCompanionRuntimeDataAsset>();
	RuntimeData->BaseSupportInterval = 4.0f;
	RuntimeData->SupportRange = 800.0f;
	RuntimeData->SupportMagnitude = 12.0f;
	RuntimeData->SupportType = EPRCompanionSupportType::LightDamage;

	TestEqual(TEXT("Zero overload retains the base interval"),
		UPRCompanionRuntimeSubsystem::CalculateEffectiveSupportInterval(*RuntimeData, 0), 4.0f);
	TestEqual(TEXT("Full overload doubles the base interval"),
		UPRCompanionRuntimeSubsystem::CalculateEffectiveSupportInterval(*RuntimeData, 100), 8.0f);
	TestEqual(TEXT("Overload is clamped before calculating the interval"),
		UPRCompanionRuntimeSubsystem::CalculateEffectiveSupportInterval(*RuntimeData, 500), 8.0f);

	FPRCompanionSupportEvent Event;
	Event.CompanionId = FPRCompanionContract::KindleTag();
	Event.SupportType = EPRCompanionSupportType::LightDamage;
	Event.Result = EPRCompanionSupportResult::Applied;
	Event.RequestedMagnitude = 12.0f;
	TestTrue(TEXT("Support event has a generated identity"), Event.EventId.IsValid());
	TestTrue(TEXT("Support event stores the support identity"), Event.CompanionId.MatchesTagExact(FPRCompanionContract::KindleTag()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRCompanionRuntimeAssetTest,
	"ProjectR.Companion.Combat.RuntimeAssetManifest",
	PRCompanionCombatAutomation::TestFlags)

bool FPRCompanionRuntimeAssetTest::RunTest(const FString& Parameters)
{
	struct FExpectedRuntime
	{
		const TCHAR* ObjectPath;
		FGameplayTag Id;
		EPRCompanionSupportType Type;
		float Interval;
		float Range;
		float Magnitude;
		bool bHasEffect;
	};
	const FExpectedRuntime Expected[] = {
		{ TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Axiom.DA_CompanionRuntime_Axiom"), FPRCompanionContract::AxiomTag(), EPRCompanionSupportType::Shield, 8.0f, 0.0f, 20.0f, true },
		{ TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Kindle.DA_CompanionRuntime_Kindle"), FPRCompanionContract::KindleTag(), EPRCompanionSupportType::LightDamage, 4.0f, 800.0f, 12.0f, false },
		{ TEXT("/Game/ProjectR/Companions/Runtime/DA_CompanionRuntime_Null.DA_CompanionRuntime_Null"), FPRCompanionContract::NullTag(), EPRCompanionSupportType::ControlMark, 6.0f, 700.0f, 0.0f, true }};
	for (const FExpectedRuntime& Entry : Expected)
	{
		UPRCompanionRuntimeDataAsset* Asset = LoadObject<UPRCompanionRuntimeDataAsset>(nullptr, Entry.ObjectPath);
		TestNotNull(FString::Printf(TEXT("Runtime asset exists: %s"), Entry.ObjectPath), Asset);
		if (!Asset) continue;
		TestTrue(TEXT("Runtime identity is canonical"), Asset->CompanionId.MatchesTagExact(Entry.Id));
		TestEqual(TEXT("Runtime support type is exact"), Asset->SupportType, Entry.Type);
		TestEqual(TEXT("Runtime interval is exact"), Asset->BaseSupportInterval, Entry.Interval);
		TestEqual(TEXT("Runtime range is exact"), Asset->SupportRange, Entry.Range);
		TestEqual(TEXT("Runtime magnitude is exact"), Asset->SupportMagnitude, Entry.Magnitude);
		TestEqual(TEXT("Runtime effect binding is exact"), Asset->SupportEffectClass != nullptr, Entry.bHasEffect);
		TestTrue(TEXT("Runtime Pawn class is configured"), !Asset->PawnClass.IsNull());
		FDataValidationContext Context;
		TestEqual(TEXT("Runtime asset passes the fixed manifest validation"),
			Asset->IsDataValid(Context), EDataValidationResult::Valid);
	}
	UPRCompanionRuntimeDataAsset* Invalid = NewObject<UPRCompanionRuntimeDataAsset>();
	Invalid->CompanionId = FPRCompanionContract::AxiomTag();
	Invalid->SupportType = EPRCompanionSupportType::LightDamage;
	Invalid->BaseSupportInterval = 8.0f;
	Invalid->SupportRange = 0.0f;
	Invalid->SupportMagnitude = 20.0f;
	FDataValidationContext InvalidContext;
	TestEqual(TEXT("Axiom rejects a mismatched fixed support schema"),
		Invalid->IsDataValid(InvalidContext), EDataValidationResult::Invalid);
	UClass* ShieldClass = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/ProjectR/Companions/Effects/GE_Companion_Axiom_Shield.GE_Companion_Axiom_Shield_C"));
	UGameplayEffect* Shield = ShieldClass ? ShieldClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	TestNotNull(TEXT("Axiom Shield GameplayEffect exists"), Shield);
	if (Shield)
	{
		TestEqual(TEXT("Axiom Shield is instant"), Shield->DurationPolicy, EGameplayEffectDurationType::Instant);
		TestEqual(TEXT("Axiom Shield has one modifier"), Shield->Modifiers.Num(), 1);
		TestTrue(TEXT("Axiom Shield has no executions"), Shield->Executions.IsEmpty());
	}
	return true;
}

#endif
