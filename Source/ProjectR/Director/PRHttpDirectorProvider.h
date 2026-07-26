// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorProvider.h"

/** v0.4.0 boundary only: it has no endpoint, credentials, or live transport. */
class PROJECTR_API FPRHttpDirectorProvider final : public IPRDirectorProvider
{
public:
	virtual FName GetProviderId() const override;
	virtual bool IsAvailable() const override;
	virtual void RequestRule(const FPRDirectorRequest& Request, FPRDirectorProviderCompletion Completion) override;
	virtual void CancelRequest(FGuid RequestId) override;
};
