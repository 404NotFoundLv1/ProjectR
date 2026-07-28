// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Memory/PRMemoryTypes.h"
#include "Save/PRMemorySaveTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRMemoryLifecycleContractTest, "ProjectR.Memory.Lifecycle.PersistedSummaryExcludesRuntimeAndIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRMemoryLifecycleContractTest::RunTest(const FString&)
{
	UScriptStruct* SummaryStruct = FPRMemorySummary::StaticStruct();
	TestNotNull(TEXT("Memory summary has a reflected value contract"), SummaryStruct);
	if (!SummaryStruct) return false;
	for (const FName Forbidden : { FName(TEXT("AccountId")), FName(TEXT("ProfileId")), FName(TEXT("Slot")), FName(TEXT("Provider")), FName(TEXT("Prompt")), FName(TEXT("RawResponse")), FName(TEXT("Actor")), FName(TEXT("Timer")), FName(TEXT("Delegate")) })
	{
		TestNull(FString::Printf(TEXT("Persisted summary omits forbidden runtime or identity field %s"), *Forbidden.ToString()), SummaryStruct->FindPropertyByName(Forbidden));
	}
	UScriptStruct* SnapshotStruct = FPRMemorySnapshot::StaticStruct();
	FProperty* DisplayTextProjection = SnapshotStruct ? SnapshotStruct->FindPropertyByName(TEXT("LatestOptionDisplayTexts")) : nullptr;
	TestNotNull(TEXT("The read-only display text projection exists only on the transient snapshot"), DisplayTextProjection);
	if (DisplayTextProjection) TestTrue(TEXT("Display text projection cannot enter Save persistence"), DisplayTextProjection->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

#endif
