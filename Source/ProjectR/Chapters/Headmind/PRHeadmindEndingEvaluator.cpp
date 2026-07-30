// Copyright ProjectR. All Rights Reserved.

#include "Chapters/Headmind/PRHeadmindEndingEvaluator.h"

#include "Save/PRAccountSaveTypes.h"
#include "Roguelike/Progression/PRProgressionTypes.h"

bool FPRHeadmindEndingEvaluator::BuildInput(const FPRRunSummary& Summary, const FPRProgressionSnapshot& Progression, FPRHeadmindEndingInputSnapshot& OutInput)
{
	OutInput = FPRHeadmindEndingInputSnapshot();
	if (!Summary.RunId.IsValid() || !Summary.AccountId.IsValid()) { OutInput.FallbackReason = TEXT("Headmind.EndingInputUnavailable"); return false; }
	int32 MinimumTrust = MAX_int32;
	int32 MaximumOverload = 0;
	if (!FPRCompanionContract::AreCanonicalRelationshipRecords(Summary.CompanionRelationships)) { OutInput.FallbackReason = TEXT("Headmind.RelationshipSnapshotUnavailable"); return false; }
	for (const FPRCompanionRelationshipRecord& Record : Summary.CompanionRelationships)
	{
		MinimumTrust = FMath::Min(MinimumTrust, Record.State.Trust);
		MaximumOverload = FMath::Max(MaximumOverload, Record.State.Overload);
	}
	OutInput.RelationshipBand = MinimumTrust >= 70 && MaximumOverload == 0 ? EPRHeadmindRelationshipBand::Resonant
		: MinimumTrust >= 60 && MaximumOverload <= 50 ? EPRHeadmindRelationshipBand::Connected : EPRHeadmindRelationshipBand::Distant;
	OutInput.CounterproofBand = Progression.CounterproofFragments >= 4 ? EPRHeadmindCounterproofBand::Abundant
		: Progression.CounterproofFragments >= 1 ? EPRHeadmindCounterproofBand::Established : EPRHeadmindCounterproofBand::None;
	for (const FPRRunDirectorRuleSummary& Rule : Summary.DirectorRules)
	{
		if (Rule.RuleId.ToString() == TEXT("Rule.ObedienceTest"))
		{
			if (Rule.Level < 1 || Rule.Level > 5) { OutInput.FallbackReason = TEXT("Headmind.ObedienceInvalid"); return false; }
			OutInput.ObedienceBand = Rule.Level <= 2 ? EPRHeadmindObedienceBand::Contested : EPRHeadmindObedienceBand::Accepted;
			break;
		}
	}
	OutInput.bAvailable = true;
	return true;
}
