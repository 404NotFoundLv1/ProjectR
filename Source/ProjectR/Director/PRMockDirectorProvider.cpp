// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRMockDirectorProvider.h"

FName FPRMockDirectorProvider::GetProviderId() const { return TEXT("Mock"); }
bool FPRMockDirectorProvider::IsAvailable() const { return true; }
void FPRMockDirectorProvider::RequestRule(const FPRDirectorRequest& Request, FPRDirectorProviderCompletion Completion)
{
	FPRDirectorResponse Response;
	BuildDeterministicResponse(Request, Response);
	Completion.ExecuteIfBound(Response);
}
void FPRMockDirectorProvider::CancelRequest(FGuid RequestId) {}

bool FPRMockDirectorProvider::BuildDeterministicResponse(const FPRDirectorRequest& Request, FPRDirectorResponse& OutResponse)
{
	OutResponse = FPRDirectorResponse();
	OutResponse.RequestId = Request.RequestId;
	if (!Request.RequestId.IsValid()) return false;
	struct FScore { FString Id; int32 Level; };
	TArray<FScore> Scores;
	int32 MaximumUseCount = 0;
	for (const FPRPlayerProfileSkillMetric& Metric : Request.Profile.SkillMetrics) MaximumUseCount = FMath::Max(MaximumUseCount, Metric.UseCount);
	Scores.Add({ TEXT("Rule.RepetitionPenalty"), MaximumUseCount >= 3 ? FMath::Clamp(1 + (MaximumUseCount - 3) / 3, 1, 3) : 0 });
	int32 NegativeQTE = 0;
	for (const FPRPlayerProfileTaggedCount& Count : Request.Profile.QTEResultCounts) if (Count.Tag.ToString().Contains(TEXT("Failure")) || Count.Tag.ToString().Contains(TEXT("Rejected"))) NegativeQTE += Count.Count;
	int32 PrimaryOverload = 0;
	for (const FPRCompanionRelationshipRecord& Relationship : Request.Profile.Relationships) if (Relationship.CompanionId == Request.Profile.PrimaryCompanionId) PrimaryOverload = Relationship.State.Overload;
	Scores.Add({ TEXT("Rule.CooperationAudit"), FMath::Max(NegativeQTE >= 3 ? FMath::Clamp(1 + (NegativeQTE - 3) / 3, 1, 3) : 0, PrimaryOverload >= 80 ? FMath::Clamp(1 + (PrimaryOverload - 80) / 10, 1, 3) : 0) });
	const int32 SurvivalLevel = Request.Profile.DeathCount >= 1 ? FMath::Clamp(Request.Profile.DeathCount, 1, 3) : (Request.Profile.Resources.MinimumHealthRatio <= 0.25f ? (Request.Profile.Resources.MinimumHealthRatio <= 0.05f ? 3 : Request.Profile.Resources.MinimumHealthRatio <= 0.15f ? 2 : 1) : 0);
	Scores.Add({ TEXT("Rule.SurvivalProtocol"), SurvivalLevel });
	const float Distance = Request.Profile.CombatDistance.AverageDistanceCm;
	Scores.Add({ TEXT("Rule.DistanceCorrection"), Distance >= 650.0f ? (Distance >= 1200.0f ? 3 : Distance >= 900.0f ? 2 : 1) : 0 });
	Scores.Sort([](const FScore& Left, const FScore& Right) { return Left.Level != Right.Level ? Left.Level > Right.Level : Left.Id < Right.Id; });
	FScore Selected = Scores[0];
	if (Selected.Level == 0) { Selected.Id = TEXT("Rule.SurvivalProtocol"); Selected.Level = 1; }
	for (const FGameplayTag& Candidate : Request.CandidateRuleIds)
	{
		if (Candidate.ToString() == Selected.Id) { OutResponse.RuleId = Candidate; break; }
	}
	if (!OutResponse.RuleId.IsValid() && Request.CandidateRuleIds.Num() > 0) OutResponse.RuleId = Request.CandidateRuleIds[0];
	OutResponse.Level = Selected.Level;
	OutResponse.VisibleReason = TEXT("Local deterministic Director evaluation.");
	OutResponse.ExpressionText = Selected.Id;
	return OutResponse.RuleId.IsValid();
}
