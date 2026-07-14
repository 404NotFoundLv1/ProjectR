// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PRDebugCommandTypes.h"

/** Game-thread-only registry for fixed-schema development commands. */
class PROJECTRDEBUG_API FPRDebugCommandRegistry
{
public:
	FPRDebugProviderHandle RegisterProvider(
		FName ProviderId,
		TConstArrayView<FPRDebugCommandDescriptor> Descriptors,
		FPRDebugCommandHandler Handler);

	bool UnregisterProvider(FPRDebugProviderHandle Handle);
	TArray<FPRDebugCommandDescriptor> GetCommandDescriptors() const;
	FPRDebugCommandResult ExecuteCommand(
		const FPRDebugCommandRequest& Request,
		UGameInstance* GameInstance);

private:
	struct FProviderRecord
	{
		FGuid ProviderGuid;
		FName ProviderId;
		TWeakObjectPtr<UObject> ProviderObject;
		TArray<FPRDebugCommandDescriptor> Descriptors;
		FPRDebugCommandHandler Handler;
	};

	static bool AreDescriptorsIdentical(
		TConstArrayView<FPRDebugCommandDescriptor> Left,
		TConstArrayView<FPRDebugCommandDescriptor> Right);
	static bool ValidateArguments(
		const FPRDebugCommandDescriptor& Descriptor,
		const FPRDebugCommandRequest& Request);
	static EPRDebugCommandResultCode AvailabilityToResultCode(
		EPRDebugCommandAvailability Availability);
	static void AuditExecution(
		const FPRDebugCommandDescriptor& Descriptor,
		const FPRDebugCommandResult& Result);

	TMap<FGuid, FProviderRecord> ProviderRecords;
};
