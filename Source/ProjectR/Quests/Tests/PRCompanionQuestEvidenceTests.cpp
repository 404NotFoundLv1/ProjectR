// Copyright Epic Games, Inc. All Rights Reserved.
#if WITH_DEV_AUTOMATION_TESTS
#include "Quests/PRCompanionQuestDataAsset.h"
#include "Quests/PRCompanionQuestEvidence.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPRCompanionQuestEvidenceTest,"ProjectR.CompanionQuest.Evidence.DefinitionRequiresStableFacts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FPRCompanionQuestEvidenceTest::RunTest(const FString&)
{
	UPRCompanionQuestDataAsset* Quest=NewObject<UPRCompanionQuestDataAsset>();
	TestFalse(TEXT("Blank definition is never valid evidence"),Quest->IsDefinitionValid());
	Quest->QuestId=TEXT("Quest.Null.RememberMe");Quest->CompanionId=FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"),false);Quest->EntitlementId=TEXT("Line:Null_RememberMe");Quest->CompletionLineId=Quest->QuestId;Quest->DisplayName=FText::FromString(TEXT("Remember Me"));Quest->ObjectiveText=FText::FromString(TEXT("View records"));
	TestTrue(TEXT("Stable fixed definition becomes valid"),Quest->IsDefinitionValid());

	FPRAccountRecord NoRetreat;
	NoRetreat.TerminationReason=EPRAccountTerminationReason::RoomSequenceCompleted;
	NoRetreat.Summary.PrimaryCompanionId=FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"),false);
	NoRetreat.Summary.bBossCompleted=true;
	NoRetreat.Summary.MinimumHealthRatio=0.25f;
	TestFalse(TEXT("No Retreat refuses a run without EliteAudit"),FPRCompanionQuestEvidenceContract::IsKindleNoRetreat(NoRetreat));
	NoRetreat.Summary.RoomIds.Add(FPrimaryAssetId(TEXT("ProjectRRoom"),TEXT("DA_Room_EliteAudit")));
	TestTrue(TEXT("No Retreat accepts only the fixed EliteAudit account fact"),FPRCompanionQuestEvidenceContract::IsKindleNoRetreat(NoRetreat));

	FPRRoomEventResult Commission;
	Commission.ResolutionId=FGuid::NewGuid();
	Commission.EventId=FPrimaryAssetId(TEXT("ProjectRRoomEvent"),TEXT("DA_RoomEvent_Commission"));
	Commission.ChoiceId=TEXT("Fulfill");
	Commission.bChoiceApplied=true;
	TestTrue(TEXT("Low Probability Sample accepts the fixed applied Commission fact"),FPRCompanionQuestEvidenceContract::IsAxiomLowProbabilitySample(Commission,FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"),false)));
	Commission.EventId=FPrimaryAssetId(TEXT("UnknownType"),TEXT("DA_RoomEvent_Commission"));
	TestFalse(TEXT("Low Probability Sample rejects an unknown event type"),FPRCompanionQuestEvidenceContract::IsAxiomLowProbabilitySample(Commission,FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"),false)));

	FPRDivergenceResult Rescue;
	Rescue.ResultId=FGuid::NewGuid(); Rescue.CompanionId=FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"),false); Rescue.Choice=EPRDivergenceChoice::Rescue; Rescue.Resolution=EPRDivergenceResolution::Applied; Rescue.FutureDisposition=EPRDivergenceFutureDisposition::RescueEvacuationRequested;
	TestTrue(TEXT("Axiom rescue candidate uses only the applied stable divergence result"),FPRCompanionQuestEvidenceContract::IsAxiomRescueCandidate(Rescue));
	FPRAccountRecord AxiomEvacuation; AxiomEvacuation.TerminationReason=EPRAccountTerminationReason::DivergenceEvacuation; AxiomEvacuation.Summary.PrimaryCompanionId=FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"),false);
	TestTrue(TEXT("Imperfect Optimum requires the stored rescue evidence and Axiom evacuation"),FPRCompanionQuestEvidenceContract::IsAxiomImperfectOptimum(AxiomEvacuation,Rescue.ResultId));
	TestFalse(TEXT("Imperfect Optimum rejects missing rescue evidence"),FPRCompanionQuestEvidenceContract::IsAxiomImperfectOptimum(AxiomEvacuation,FGuid()));

	FPRAccountRecord KindleEvacuation; KindleEvacuation.TerminationReason=EPRAccountTerminationReason::DivergenceEvacuation; KindleEvacuation.Summary.PrimaryCompanionId=FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"),false); KindleEvacuation.Summary.RewardIds.Add(FPrimaryAssetId(TEXT("ProjectRReward"),TEXT("DA_Reward_VitalityCache")));
	TestTrue(TEXT("Learn To Retreat requires a Kindle evacuation with an existing reward id"),FPRCompanionQuestEvidenceContract::IsKindleLearnToRetreat(KindleEvacuation));

	FPRAccountRecord Garbage; Garbage.TerminationReason=EPRAccountTerminationReason::RoomSequenceCompleted; Garbage.Summary.PrimaryCompanionId=FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"),false); Garbage.Summary.CounterproofFragmentsAwarded=1; Garbage.Summary.DirectorRules.Add({FGameplayTag::RequestGameplayTag(TEXT("Rule.DeleteEcho"),false),1});
	TestTrue(TEXT("Garbage Collection accepts only the fixed completed Null run fact"),FPRCompanionQuestEvidenceContract::IsNullGarbageCollection(Garbage));
	Garbage.Summary.CounterproofFragmentsAwarded=2;
	TestFalse(TEXT("Garbage Collection rejects any fragment count other than one"),FPRCompanionQuestEvidenceContract::IsNullGarbageCollection(Garbage));

	TArray<FPRAccountRecord> Graveyard;
	for(int32 Index=0;Index<5;++Index){FPRAccountRecord& Record=Graveyard.AddDefaulted_GetRef();Record.RecordId=FGuid::NewGuid();}
	TestTrue(TEXT("Remember Me requires five unique projected records"),FPRCompanionQuestEvidenceContract::HasFiveUniqueGraveyardRecords(Graveyard));
	Graveyard[4].RecordId=Graveyard[0].RecordId;
	TestFalse(TEXT("Remember Me refuses duplicate projected records"),FPRCompanionQuestEvidenceContract::HasFiveUniqueGraveyardRecords(Graveyard));
	return true;
}
#endif
