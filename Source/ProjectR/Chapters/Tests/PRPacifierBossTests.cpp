// Copyright ProjectR. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Chapters/Pacifier/PRPacifierBossComponent.h"
#include "Chapters/Pacifier/PRPacifierIllusionProjection.h"
#include "Enemies/PREnemyCharacter.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRPacifierBossContractTest,
	"ProjectR.Chapter.Pacifier.PacifierBoss.BoundedRuntimeAndProjectionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPRPacifierBossContractTest::RunTest(const FString& Parameters)
{
	UPRPacifierBossComponent* Component = NewObject<UPRPacifierBossComponent>();
	const FPRPacifierBossRuntimeState& Initial = Component->GetRuntimeState();
	TestEqual(TEXT("Pacifier starts dormant"), Initial.Phase, EPRPacifierBossPhase::Dormant);
	TestEqual(TEXT("Pacifier starts with no comfort pressure"), Initial.ComfortPressure, 0);
	TestEqual(TEXT("Pacifier starts with no suppression layers"), Initial.SuppressionLayers, 0);
	TestEqual(TEXT("Pacifier starts with no active illusion projections"), Initial.ActiveProjectionCount, 0);

	Component->ConfigureChapterState(7);
	TestEqual(TEXT("Only bounded ComfortPressure reaches the Boss contract"),
		Component->GetRuntimeState().ComfortPressure, 4);
	TestTrue(TEXT("Illusion projections are native transient actors, not formal enemies"),
		APRPacifierIllusionProjection::StaticClass()->IsChildOf(AActor::StaticClass()));
	TestFalse(TEXT("Illusion projections cannot become registered enemy identities"),
		APRPacifierIllusionProjection::StaticClass()->IsChildOf(APREnemyCharacter::StaticClass()));
	return true;
}

#endif
