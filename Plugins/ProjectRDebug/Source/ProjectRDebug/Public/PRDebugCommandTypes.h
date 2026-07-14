// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRSaveTypes.h"

class UGameInstance;

enum class EPRDebugCommandId : uint8
{
	StatusSnapshot,
	DamageSelf,
	ReviveSelf,
	SaveRuntimeState,
	ReturnToCombatGym,
	GrantResource,
	ClearCooldown,
	GenerateDirectorRule,
	SpawnQTE,
	CycleCompanionAI,
	JumpToBoss
};

enum class EPRDebugArgumentType : uint8
{
	None,
	Float,
	EnumChoice
};

enum class EPRDebugCommandAvailability : uint8
{
	Available,
	NotAvailable,
	Disabled,
	InvalidContext
};

enum class EPRDebugCommandResultCode : uint8
{
	Success,
	NotAvailable,
	Disabled,
	InvalidContext,
	InvalidParameter,
	Rejected,
	Failed
};

struct PROJECTRDEBUG_API FPRDebugArgumentSpec
{
	FName ArgumentId;
	EPRDebugArgumentType Type = EPRDebugArgumentType::None;
	bool bRequired = false;
	double MinValue = 0.0;
	double MaxValue = 0.0;
	double DefaultValue = 0.0;
	TArray<FName> AllowedChoices;
};

struct PROJECTRDEBUG_API FPRDebugArgumentValue
{
	FName ArgumentId;
	EPRDebugArgumentType Type = EPRDebugArgumentType::None;
	double FloatValue = 0.0;
	FName ChoiceValue;
};

struct PROJECTRDEBUG_API FPRDebugCommandDescriptor
{
	EPRDebugCommandId CommandId = EPRDebugCommandId::StatusSnapshot;
	FName StableName;
	uint16 SchemaVersion = 1;
	FText DisplayName;
	FText Description;
	TArray<FPRDebugArgumentSpec> Arguments;
	EPRDebugCommandAvailability Availability = EPRDebugCommandAvailability::Available;
	bool bChangesRuntimeState = false;
};

struct PROJECTRDEBUG_API FPRDebugCommandRequest
{
	FGuid RequestId;
	EPRDebugCommandId CommandId = EPRDebugCommandId::StatusSnapshot;
	uint16 SchemaVersion = 1;
	TArray<FPRDebugArgumentValue> Arguments;
};

struct PROJECTRDEBUG_API FPRDebugStatusSnapshot
{
	FName MapShortName;
	bool bHasLocalPlayer = false;
	bool bHasPawn = false;
	bool bHasAbilitySystem = false;
	float Health = 0.0f;
	float Shield = 0.0f;
	float Energy = 0.0f;
	int32 GrantedAbilityCount = 0;
	int32 ActiveAbilityCount = 0;
	int32 ActiveEffectCount = 0;
	EPRSaveSubsystemState SaveState = EPRSaveSubsystemState::Uninitialized;
	bool bHasLoadedProfile = false;
	int32 SaveSchemaVersion = 0;
	int64 SaveRevision = 0;
	bool bSaveNeedsResave = false;
	bool bSaveRequestQueued = false;
	EPRSaveResult SaveLastResult = EPRSaveResult::InvalidRequest;
	EPRSaveGeneration SaveGeneration = EPRSaveGeneration::None;
};

struct PROJECTRDEBUG_API FPRDebugCommandResult
{
	FGuid RequestId;
	EPRDebugCommandId CommandId = EPRDebugCommandId::StatusSnapshot;
	EPRDebugCommandResultCode ResultCode = EPRDebugCommandResultCode::Failed;
	FText PlayerMessage;
	bool bHasStatusSnapshot = false;
	FPRDebugStatusSnapshot StatusSnapshot;
};

struct PROJECTRDEBUG_API FPRDebugProviderHandle
{
public:
	FPRDebugProviderHandle() = default;
	bool IsValid() const;
	bool operator==(const FPRDebugProviderHandle& Other) const;

private:
	explicit FPRDebugProviderHandle(const FGuid& InProviderGuid);

	FGuid ProviderGuid;

	friend class FPRDebugCommandRegistry;
};

DECLARE_DELEGATE_RetVal_TwoParams(
	FPRDebugCommandResult,
	FPRDebugCommandHandler,
	const FPRDebugCommandRequest&,
	UGameInstance*);
