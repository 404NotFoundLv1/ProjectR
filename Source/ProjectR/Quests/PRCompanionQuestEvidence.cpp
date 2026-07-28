// Copyright Epic Games, Inc. All Rights Reserved.

#include "Quests/PRCompanionQuestEvidence.h"

namespace PRCompanionQuestEvidencePrivate
{
const FGameplayTag Axiom = FGameplayTag::RequestGameplayTag(TEXT("Companion.Axiom"), false);
const FGameplayTag Kindle = FGameplayTag::RequestGameplayTag(TEXT("Companion.Kindle"), false);
const FGameplayTag Null = FGameplayTag::RequestGameplayTag(TEXT("Companion.Null"), false);
const FGameplayTag DeleteEcho = FGameplayTag::RequestGameplayTag(TEXT("Rule.DeleteEcho"), false);

bool HasRoom(const FPRRunSummary& Summary, const TCHAR* Name)
{
	return Summary.RoomIds.Contains(FPrimaryAssetId(TEXT("ProjectRRoom"), Name));
}
}

bool FPRCompanionQuestEvidenceContract::IsAxiomLowProbabilitySample(const FPRRoomEventResult& Event, const FGameplayTag PrimaryCompanionId)
{
	return Event.ResolutionId.IsValid()
		&& PrimaryCompanionId == PRCompanionQuestEvidencePrivate::Axiom
		&& Event.bChoiceApplied
		&& Event.EventId == FPrimaryAssetId(TEXT("ProjectRRoomEvent"), TEXT("DA_RoomEvent_Commission"))
		&& Event.ChoiceId == TEXT("Fulfill");
}

bool FPRCompanionQuestEvidenceContract::IsAxiomRescueCandidate(const FPRDivergenceResult& Event)
{
	return Event.ResultId.IsValid()
		&& Event.CompanionId == PRCompanionQuestEvidencePrivate::Axiom
		&& Event.Choice == EPRDivergenceChoice::Rescue
		&& Event.Resolution == EPRDivergenceResolution::Applied
		&& Event.FutureDisposition == EPRDivergenceFutureDisposition::RescueEvacuationRequested;
}

bool FPRCompanionQuestEvidenceContract::IsAxiomImperfectOptimum(const FPRAccountRecord& Record, const FGuid& RescueEvidenceId)
{
	return RescueEvidenceId.IsValid()
		&& Record.TerminationReason == EPRAccountTerminationReason::DivergenceEvacuation
		&& Record.Summary.PrimaryCompanionId == PRCompanionQuestEvidencePrivate::Axiom;
}

bool FPRCompanionQuestEvidenceContract::IsKindleNoRetreat(const FPRAccountRecord& Record)
{
	return Record.TerminationReason == EPRAccountTerminationReason::RoomSequenceCompleted
		&& Record.Summary.PrimaryCompanionId == PRCompanionQuestEvidencePrivate::Kindle
		&& PRCompanionQuestEvidencePrivate::HasRoom(Record.Summary, TEXT("DA_Room_EliteAudit"))
		&& Record.Summary.MinimumHealthRatio <= 0.25f
		&& Record.Summary.bBossCompleted;
}

bool FPRCompanionQuestEvidenceContract::IsKindleLearnToRetreat(const FPRAccountRecord& Record)
{
	return Record.TerminationReason == EPRAccountTerminationReason::DivergenceEvacuation
		&& Record.Summary.PrimaryCompanionId == PRCompanionQuestEvidencePrivate::Kindle
		&& !Record.Summary.RewardIds.IsEmpty();
}

bool FPRCompanionQuestEvidenceContract::IsNullGarbageCollection(const FPRAccountRecord& Record)
{
	return Record.TerminationReason == EPRAccountTerminationReason::RoomSequenceCompleted
		&& Record.Summary.PrimaryCompanionId == PRCompanionQuestEvidencePrivate::Null
		&& Record.Summary.CounterproofFragmentsAwarded == 1
		&& Record.Summary.DirectorRules.ContainsByPredicate([](const FPRRunDirectorRuleSummary& Rule)
		{
			return Rule.RuleId == PRCompanionQuestEvidencePrivate::DeleteEcho;
		});
}

bool FPRCompanionQuestEvidenceContract::HasFiveUniqueGraveyardRecords(const TArray<FPRAccountRecord>& Records)
{
	TSet<FGuid> UniqueRecords;
	for (const FPRAccountRecord& Record : Records)
	{
		if (Record.RecordId.IsValid())
		{
			UniqueRecords.Add(Record.RecordId);
		}
	}
	return UniqueRecords.Num() >= 5;
}
