// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Director/PRDirectorTypes.h"

class PROJECTR_API IPRDirectorProvider
{
public:
	virtual ~IPRDirectorProvider() = default;
	virtual FName GetProviderId() const = 0;
	virtual bool IsAvailable() const = 0;
	virtual void RequestRule(const FPRDirectorRequest& Request, FPRDirectorProviderCompletion Completion) = 0;
	virtual void CancelRequest(FGuid RequestId) = 0;
};
