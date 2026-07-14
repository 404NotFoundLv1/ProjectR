// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDebugCommandRegistry.h"

#include "Core/Logging/PRLogSanitizer.h"
#include "PRDebugLog.h"

namespace
{
bool AreArgumentSpecsIdentical(const FPRDebugArgumentSpec& Left, const FPRDebugArgumentSpec& Right)
{
	return Left.ArgumentId == Right.ArgumentId
		&& Left.Type == Right.Type
		&& Left.bRequired == Right.bRequired
		&& Left.MinValue == Right.MinValue
		&& Left.MaxValue == Right.MaxValue
		&& Left.DefaultValue == Right.DefaultValue
		&& Left.AllowedChoices == Right.AllowedChoices;
}

bool AreDescriptorValuesIdentical(const FPRDebugCommandDescriptor& Left, const FPRDebugCommandDescriptor& Right)
{
	if (Left.CommandId != Right.CommandId
		|| Left.StableName != Right.StableName
		|| Left.SchemaVersion != Right.SchemaVersion
		|| !Left.DisplayName.IdenticalTo(Right.DisplayName)
		|| !Left.Description.IdenticalTo(Right.Description)
		|| Left.Availability != Right.Availability
		|| Left.bChangesRuntimeState != Right.bChangesRuntimeState
		|| Left.Arguments.Num() != Right.Arguments.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Left.Arguments.Num(); ++Index)
	{
		if (!AreArgumentSpecsIdentical(Left.Arguments[Index], Right.Arguments[Index]))
		{
			return false;
		}
	}

	return true;
}
}

FPRDebugProviderHandle::FPRDebugProviderHandle(const FGuid& InProviderGuid)
	: ProviderGuid(InProviderGuid)
{
}

bool FPRDebugProviderHandle::IsValid() const
{
	return ProviderGuid.IsValid();
}

bool FPRDebugProviderHandle::operator==(const FPRDebugProviderHandle& Other) const
{
	return ProviderGuid == Other.ProviderGuid;
}

FPRDebugProviderHandle FPRDebugCommandRegistry::RegisterProvider(
	FName ProviderId,
	TConstArrayView<FPRDebugCommandDescriptor> Descriptors,
	FPRDebugCommandHandler Handler)
{
	if (!IsInGameThread() || ProviderId.IsNone() || Descriptors.IsEmpty() || !Handler.IsBound())
	{
		UE_LOG(LogProjectRDebug, Error, TEXT("Rejected invalid debug provider registration."));
		return FPRDebugProviderHandle();
	}

	UObject* ProviderObject = Handler.GetUObject();
	if (!IsValid(ProviderObject))
	{
		UE_LOG(LogProjectRDebug, Error, TEXT("Debug provider handler must be weakly bound to a valid UObject."));
		return FPRDebugProviderHandle();
	}

	TSet<EPRDebugCommandId> IncomingIds;
	TSet<FName> IncomingNames;
	for (const FPRDebugCommandDescriptor& Descriptor : Descriptors)
	{
		if (Descriptor.StableName.IsNone()
			|| IncomingIds.Contains(Descriptor.CommandId)
			|| IncomingNames.Contains(Descriptor.StableName))
		{
			UE_LOG(LogProjectRDebug, Error, TEXT("Rejected duplicate or unnamed debug command descriptor."));
			return FPRDebugProviderHandle();
		}
		IncomingIds.Add(Descriptor.CommandId);
		IncomingNames.Add(Descriptor.StableName);
	}

	for (const TPair<FGuid, FProviderRecord>& Pair : ProviderRecords)
	{
		const FProviderRecord& Existing = Pair.Value;
		if (Existing.ProviderId == ProviderId)
		{
			if (Existing.ProviderObject.Get() == ProviderObject
				&& AreDescriptorsIdentical(Existing.Descriptors, Descriptors))
			{
				return FPRDebugProviderHandle(Existing.ProviderGuid);
			}

			UE_LOG(LogProjectRDebug, Error, TEXT("Rejected conflicting debug provider id."));
			return FPRDebugProviderHandle();
		}

		for (const FPRDebugCommandDescriptor& ExistingDescriptor : Existing.Descriptors)
		{
			if (IncomingIds.Contains(ExistingDescriptor.CommandId)
				|| IncomingNames.Contains(ExistingDescriptor.StableName))
			{
				UE_LOG(LogProjectRDebug, Error, TEXT("Rejected conflicting debug command id or stable name."));
				return FPRDebugProviderHandle();
			}
		}
	}

	FProviderRecord Record;
	Record.ProviderGuid = FGuid::NewGuid();
	Record.ProviderId = ProviderId;
	Record.ProviderObject = ProviderObject;
	Record.Descriptors = TArray<FPRDebugCommandDescriptor>(Descriptors);
	Record.Handler = MoveTemp(Handler);
	const FGuid ProviderGuid = Record.ProviderGuid;
	ProviderRecords.Add(ProviderGuid, MoveTemp(Record));
	return FPRDebugProviderHandle(ProviderGuid);
}

bool FPRDebugCommandRegistry::UnregisterProvider(FPRDebugProviderHandle Handle)
{
	return IsInGameThread() && Handle.IsValid() && ProviderRecords.Remove(Handle.ProviderGuid) == 1;
}

TArray<FPRDebugCommandDescriptor> FPRDebugCommandRegistry::GetCommandDescriptors() const
{
	TArray<FPRDebugCommandDescriptor> Result;
	for (const TPair<FGuid, FProviderRecord>& Pair : ProviderRecords)
	{
		Result.Append(Pair.Value.Descriptors);
	}
	Result.Sort([](const FPRDebugCommandDescriptor& Left, const FPRDebugCommandDescriptor& Right)
	{
		return static_cast<uint8>(Left.CommandId) < static_cast<uint8>(Right.CommandId);
	});
	return Result;
}

FPRDebugCommandResult FPRDebugCommandRegistry::ExecuteCommand(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	FPRDebugCommandResult Result;
	Result.RequestId = Request.RequestId.IsValid() ? Request.RequestId : FGuid::NewGuid();
	Result.CommandId = Request.CommandId;

	if (!IsInGameThread())
	{
		Result.ResultCode = EPRDebugCommandResultCode::Rejected;
		Result.PlayerMessage = FText::FromString(TEXT("Command rejected."));
		return Result;
	}

	FProviderRecord* Record = nullptr;
	const FPRDebugCommandDescriptor* Descriptor = nullptr;
	for (TPair<FGuid, FProviderRecord>& Pair : ProviderRecords)
	{
		for (const FPRDebugCommandDescriptor& Candidate : Pair.Value.Descriptors)
		{
			if (Candidate.CommandId == Request.CommandId)
			{
				Record = &Pair.Value;
				Descriptor = &Candidate;
				break;
			}
		}
		if (Descriptor != nullptr)
		{
			break;
		}
	}

	if (Descriptor == nullptr || Record == nullptr)
	{
		Result.ResultCode = EPRDebugCommandResultCode::NotAvailable;
		Result.PlayerMessage = FText::FromString(TEXT("Command is not available."));
		return Result;
	}

	if (Request.SchemaVersion != Descriptor->SchemaVersion || !ValidateArguments(*Descriptor, Request))
	{
		Result.ResultCode = EPRDebugCommandResultCode::InvalidParameter;
		Result.PlayerMessage = FText::FromString(TEXT("Command parameters are invalid."));
		AuditExecution(*Descriptor, Result);
		return Result;
	}

	if (Descriptor->Availability != EPRDebugCommandAvailability::Available)
	{
		Result.ResultCode = AvailabilityToResultCode(Descriptor->Availability);
		Result.PlayerMessage = FText::FromString(TEXT("Command is not available in this version."));
		AuditExecution(*Descriptor, Result);
		return Result;
	}

	if (!Record->ProviderObject.IsValid() || !Record->Handler.IsBound())
	{
		Result.ResultCode = EPRDebugCommandResultCode::NotAvailable;
		Result.PlayerMessage = FText::FromString(TEXT("Command provider is not available."));
		AuditExecution(*Descriptor, Result);
		return Result;
	}

	Result = Record->Handler.Execute(Request, GameInstance);
	Result.RequestId = Request.RequestId.IsValid() ? Request.RequestId : Result.RequestId;
	if (!Result.RequestId.IsValid())
	{
		Result.RequestId = FGuid::NewGuid();
	}
	Result.CommandId = Request.CommandId;
	AuditExecution(*Descriptor, Result);
	return Result;
}

bool FPRDebugCommandRegistry::AreDescriptorsIdentical(
	TConstArrayView<FPRDebugCommandDescriptor> Left,
	TConstArrayView<FPRDebugCommandDescriptor> Right)
{
	if (Left.Num() != Right.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Left.Num(); ++Index)
	{
		if (!AreDescriptorValuesIdentical(Left[Index], Right[Index]))
		{
			return false;
		}
	}
	return true;
}

bool FPRDebugCommandRegistry::ValidateArguments(
	const FPRDebugCommandDescriptor& Descriptor,
	const FPRDebugCommandRequest& Request)
{
	if (Request.Arguments.Num() > Descriptor.Arguments.Num())
	{
		return false;
	}

	TSet<FName> Seen;
	for (const FPRDebugArgumentValue& Value : Request.Arguments)
	{
		if (Value.ArgumentId.IsNone() || Seen.Contains(Value.ArgumentId))
		{
			return false;
		}
		Seen.Add(Value.ArgumentId);

		const FPRDebugArgumentSpec* Spec = Descriptor.Arguments.FindByPredicate(
			[&Value](const FPRDebugArgumentSpec& Candidate)
			{
				return Candidate.ArgumentId == Value.ArgumentId;
			});
		if (Spec == nullptr || Spec->Type != Value.Type)
		{
			return false;
		}

		if (Value.Type == EPRDebugArgumentType::Float)
		{
			if (!FMath::IsFinite(Value.FloatValue)
				|| Value.FloatValue < Spec->MinValue
				|| Value.FloatValue > Spec->MaxValue
				|| !Value.ChoiceValue.IsNone())
			{
				return false;
			}
		}
		else if (Value.Type == EPRDebugArgumentType::EnumChoice)
		{
			if (Value.FloatValue != 0.0 || !Spec->AllowedChoices.Contains(Value.ChoiceValue))
			{
				return false;
			}
		}
		else if (Value.FloatValue != 0.0 || !Value.ChoiceValue.IsNone())
		{
			return false;
		}
	}

	for (const FPRDebugArgumentSpec& Spec : Descriptor.Arguments)
	{
		if (Spec.bRequired && !Seen.Contains(Spec.ArgumentId))
		{
			return false;
		}
	}
	return true;
}

EPRDebugCommandResultCode FPRDebugCommandRegistry::AvailabilityToResultCode(
	EPRDebugCommandAvailability Availability)
{
	switch (Availability)
	{
	case EPRDebugCommandAvailability::NotAvailable:
		return EPRDebugCommandResultCode::NotAvailable;
	case EPRDebugCommandAvailability::Disabled:
		return EPRDebugCommandResultCode::Disabled;
	case EPRDebugCommandAvailability::InvalidContext:
		return EPRDebugCommandResultCode::InvalidContext;
	default:
		return EPRDebugCommandResultCode::Failed;
	}
}

void FPRDebugCommandRegistry::AuditExecution(
	const FPRDebugCommandDescriptor& Descriptor,
	const FPRDebugCommandResult& Result)
{
	UE_LOG(
		LogProjectRDebug,
		Log,
		TEXT("Request=%s StableName=%s Result=%d ChangesRuntimeState=%s"),
		*FPRLogSanitizer::ToOpaqueGuidToken(Result.RequestId),
		*Descriptor.StableName.ToString(),
		static_cast<int32>(Result.ResultCode),
		Descriptor.bChangesRuntimeState ? TEXT("true") : TEXT("false"));
}
