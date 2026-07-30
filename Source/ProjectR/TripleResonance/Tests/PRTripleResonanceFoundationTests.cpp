// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Save/PRTripleResonanceSaveTypes.h"
#include "TripleResonance/PRTripleResonanceTypes.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRTripleResonanceFoundationTest,
	"ProjectR.TripleResonance.Foundation.FixedContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRTripleResonanceFoundationTest::RunTest(const FString& Parameters)
{
	const TArray<FName>& ExternalIds = FPRTripleResonanceContract::GetExternalQTEIds();
	TestEqual(TEXT("Exactly three external QTE identifiers are reserved"), ExternalIds.Num(), 3);
	TestEqual(TEXT("Axiom QTE remains first"), ExternalIds[0], FName(TEXT("TripleResonance_Axiom")));
	TestEqual(TEXT("Kindle QTE remains second"), ExternalIds[1], FName(TEXT("TripleResonance_Kindle")));
	TestEqual(TEXT("Null QTE remains third"), ExternalIds[2], FName(TEXT("TripleResonance_Null")));
	TestTrue(TEXT("Known external identifiers are accepted"), FPRTripleResonanceContract::IsExternalQTEId(ExternalIds[0]));
	TestFalse(TEXT("Arbitrary external identifiers are rejected"), FPRTripleResonanceContract::IsExternalQTEId(TEXT("Untrusted")));

	FPRTripleResonancePersistenceData Persistence;
	TestEqual(TEXT("New persistence waterline starts at zero"), Persistence.LastProcessedGraveyardOrdinal, int64{0});
	TestFalse(TEXT("New persistence has no skill memory"), Persistence.SkillMemory.IsValid());
	TestFalse(TEXT("New persistence has no high-risk proof"), Persistence.bHasHighRiskProof);
	return true;
}

#endif
