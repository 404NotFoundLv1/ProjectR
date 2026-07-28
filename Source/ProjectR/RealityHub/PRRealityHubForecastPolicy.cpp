// Copyright Epic Games, Inc. All Rights Reserved.

#include "RealityHub/PRRealityHubForecastPolicy.h"

#include "Director/PRPlayerProfileTypes.h"

bool FPRRealityHubForecastPolicy::BuildUnavailable(FPRRealityHubForecast& OutForecast)
{
	OutForecast = FPRRealityHubForecast();
	OutForecast.Result = EPRRealityHubForecastResult::UnavailableProfile;
	OutForecast.Explanation = FText::FromString(TEXT("No forecast is available."));
	return false;
}

bool FPRRealityHubForecastPolicy::BuildForecast(
	const FPRPlayerProfileSnapshot& Profile,
	const TArray<FGameplayTag>& CandidateRuleIds,
	FPRRealityHubForecast& OutForecast)
{
	OutForecast = FPRRealityHubForecast();
	if (!Profile.ProfileSessionId.IsValid()) return BuildUnavailable(OutForecast);

	struct FScore
	{
		FString Id;
		int32 Level = 0;
	};
	TArray<FScore> Scores;
	int32 MaximumUseCount = 0;
	for (const FPRPlayerProfileSkillMetric& Metric : Profile.SkillMetrics) MaximumUseCount = FMath::Max(MaximumUseCount, Metric.UseCount);
	Scores.Add({ TEXT("Rule.RepetitionPenalty"), MaximumUseCount >= 3 ? FMath::Clamp(1 + (MaximumUseCount - 3) / 3, 1, 3) : 0 });

	int32 NegativeQTE = 0;
	for (const FPRPlayerProfileTaggedCount& Count : Profile.QTEResultCounts)
	{
		if (Count.Tag.ToString().Contains(TEXT("Failure")) || Count.Tag.ToString().Contains(TEXT("Rejected"))) NegativeQTE += Count.Count;
	}
	int32 PrimaryOverload = 0;
	for (const FPRCompanionRelationshipRecord& Relationship : Profile.Relationships)
	{
		if (Relationship.CompanionId == Profile.PrimaryCompanionId) PrimaryOverload = Relationship.State.Overload;
	}
	Scores.Add({ TEXT("Rule.CooperationAudit"), FMath::Max(NegativeQTE >= 3 ? FMath::Clamp(1 + (NegativeQTE - 3) / 3, 1, 3) : 0, PrimaryOverload >= 80 ? FMath::Clamp(1 + (PrimaryOverload - 80) / 10, 1, 3) : 0) });
	const int32 SurvivalLevel = Profile.DeathCount >= 1 ? FMath::Clamp(Profile.DeathCount, 1, 3) : (Profile.Resources.MinimumHealthRatio <= 0.25f ? (Profile.Resources.MinimumHealthRatio <= 0.05f ? 3 : Profile.Resources.MinimumHealthRatio <= 0.15f ? 2 : 1) : 0);
	Scores.Add({ TEXT("Rule.SurvivalProtocol"), SurvivalLevel });
	const float Distance = Profile.CombatDistance.AverageDistanceCm;
	Scores.Add({ TEXT("Rule.DistanceCorrection"), Distance >= 650.0f ? (Distance >= 1200.0f ? 3 : Distance >= 900.0f ? 2 : 1) : 0 });
	Scores.Sort([](const FScore& Left, const FScore& Right) { return Left.Level != Right.Level ? Left.Level > Right.Level : Left.Id < Right.Id; });
	FScore Selected = Scores[0];
	if (Selected.Level == 0)
	{
		Selected.Id = TEXT("Rule.SurvivalProtocol");
		Selected.Level = 1;
	}
	for (const FGameplayTag& Candidate : CandidateRuleIds)
	{
		if (Candidate.ToString() == Selected.Id)
		{
			OutForecast.RuleId = Candidate;
			break;
		}
	}
	if (!OutForecast.RuleId.IsValid())
	{
		OutForecast.Result = EPRRealityHubForecastResult::UnavailableRegistry;
		OutForecast.Explanation = FText::FromString(TEXT("No forecast is available."));
		return false;
	}
	OutForecast.Result = EPRRealityHubForecastResult::Available;
	OutForecast.Level = Selected.Level;
	OutForecast.Explanation = FText::FromString(TEXT("Local deterministic forecast."));
	return true;
}
