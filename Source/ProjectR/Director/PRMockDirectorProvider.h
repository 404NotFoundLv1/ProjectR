// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorProvider.h"

class PROJECTR_API FPRMockDirectorProvider final : public IPRDirectorProvider
{
public:
	virtual FName GetProviderId() const override;
	virtual bool IsAvailable() const override;
	virtual void RequestRule(const FPRDirectorRequest& Request, FPRDirectorProviderCompletion Completion) override;
	virtual void CancelRequest(FGuid RequestId) override;
	static bool BuildDeterministicResponse(const FPRDirectorRequest& Request, FPRDirectorResponse& OutResponse);
};
