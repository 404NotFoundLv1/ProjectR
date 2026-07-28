// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Memory/PRPostRunDialogueProvider.h"

class UPRMemoryRegistryDataAsset;

/** Deterministic, offline-only v0.5.2 provider. */
class PROJECTR_API FPRMockPostRunDialogueProvider final : public IPRPostRunDialogueProvider
{
public:
	explicit FPRMockPostRunDialogueProvider(const UPRMemoryRegistryDataAsset* InRegistry);
	virtual bool BeginRequest(const FPRPostRunDialogueRequest& Request, FCompletion Completion) override;
	virtual void CancelRequest(const FGuid& RequestId) override;
private:
	const UPRMemoryRegistryDataAsset* Registry = nullptr;
	TSet<FGuid> CancelledRequests;
};
