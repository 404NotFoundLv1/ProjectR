// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Memory/PRMemoryTypes.h"

/** Request identity is envelope metadata, never one of the five provider JSON fields. */
namespace FPRPostRunDialogueProviderContract
{
	inline bool IsMatchingActiveRequest(const FGuid& ActiveRequestId, const FGuid& ResponseRequestId)
	{
		return ActiveRequestId.IsValid() && ResponseRequestId.IsValid() && ActiveRequestId == ResponseRequestId;
	}
}

class IPRPostRunDialogueProvider
{
public:
	using FCompletion = TFunction<void(const FGuid& ResponseRequestId, const FPRPostRunDialogueCandidate&, const TArray<FName>&)>;
	virtual ~IPRPostRunDialogueProvider() = default;
	virtual bool BeginRequest(const FPRPostRunDialogueRequest& Request, FCompletion Completion) = 0;
	virtual void CancelRequest(const FGuid& RequestId) = 0;
};
