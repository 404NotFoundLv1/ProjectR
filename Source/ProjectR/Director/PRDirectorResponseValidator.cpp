// Copyright Epic Games, Inc. All Rights Reserved.

#include "Director/PRDirectorResponseValidator.h"
#include "Director/PRDirectorRuleDataAsset.h"
#include "Director/PRDirectorRuleRegistryDataAsset.h"

bool UPRDirectorResponseValidator::ValidateRequestIdentity(const FPRDirectorRequest& Request, const FPRDirectorResponse& Response, FPRDirectorValidationResult& OutResult)
{
	if (Request.RequestId != Response.RequestId) { OutResult.Result = EPRDirectorEvaluationResult::RejectedRequestId; return false; }
	return true;
}

bool UPRDirectorResponseValidator::Validate(const FPRDirectorRequest& Request, const FPRDirectorResponse& Response, const UPRDirectorRuleRegistryDataAsset& Registry, const double DeadlineSeconds, const double NowSeconds, FPRDirectorValidationResult& OutResult)
{
	OutResult = FPRDirectorValidationResult();
	if (Request.SchemaVersion != 1 || Response.SchemaVersion != 1) { OutResult.Result = EPRDirectorEvaluationResult::RejectedSchema; return false; }
	if (!ValidateRequestIdentity(Request, Response, OutResult)) return false;
	if (NowSeconds > DeadlineSeconds) { OutResult.Result = EPRDirectorEvaluationResult::TimedOut; return false; }
	const UPRDirectorRuleDataAsset* Rule = Registry.FindRule(Response.RuleId);
	if (!Rule || !Request.CandidateRuleIds.Contains(Response.RuleId)) { OutResult.Result = EPRDirectorEvaluationResult::RejectedUnknownRule; return false; }
	if (Response.ReasonTags.Num() > 8) { OutResult.Result = EPRDirectorEvaluationResult::RejectedReasonTags; return false; }
	for (const FGameplayTag& Tag : Response.ReasonTags) if (!Rule->AllowedReasonTags.HasTagExact(Tag)) { OutResult.Result = EPRDirectorEvaluationResult::RejectedReasonTags; return false; }
	if (Response.Parameters.Num() > 8) { OutResult.Result = EPRDirectorEvaluationResult::RejectedParameters; return false; }
	if (Response.VisibleReason.Len() > 256 || Response.ExpressionText.Len() > 512) { OutResult.Result = EPRDirectorEvaluationResult::RejectedText; return false; }
	OutResult.CanonicalResponse = Response;
	OutResult.CanonicalResponse.Level = FMath::Clamp(Response.Level, 1, Rule->MaximumLevel);
	TSet<FName> Names;
	for (FPRDirectorNumericParameter& Parameter : OutResult.CanonicalResponse.Parameters)
	{
		const FPRDirectorParameterDefinition* Definition = Rule->ParameterSchema.FindByPredicate([&Parameter](const FPRDirectorParameterDefinition& Item) { return Item.Name == Parameter.Name; });
		if (!Definition || Names.Contains(Parameter.Name) || !FMath::IsFinite(Parameter.Value)) { OutResult.Result = EPRDirectorEvaluationResult::RejectedParameters; return false; }
		Names.Add(Parameter.Name); Parameter.Value = FMath::Clamp(Parameter.Value, Definition->Minimum, Definition->Maximum);
	}
	OutResult.Result = EPRDirectorEvaluationResult::Applied;
	return true;
}
