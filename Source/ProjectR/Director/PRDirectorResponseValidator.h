// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorTypes.h"

#include "PRDirectorResponseValidator.generated.h"

class UPRDirectorRuleRegistryDataAsset;

UCLASS()
class PROJECTR_API UPRDirectorResponseValidator : public UObject
{
	GENERATED_BODY()
public:
	static bool ValidateRequestIdentity(const FPRDirectorRequest& Request, const FPRDirectorResponse& Response, FPRDirectorValidationResult& OutResult);
	static bool Validate(const FPRDirectorRequest& Request, const FPRDirectorResponse& Response, const UPRDirectorRuleRegistryDataAsset& Registry, double DeadlineSeconds, double NowSeconds, FPRDirectorValidationResult& OutResult);
};
