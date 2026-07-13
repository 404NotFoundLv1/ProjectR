// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Save/PRSaveTypes.h"

#include "SaveGameSystem.h"

class FPRSaveMigrationRegistry;
class UPRSaveGame;
class UObject;

/** Injectable SaveGame payload serialization seam. */
class PROJECTR_API IPRSavePayloadCodec
{
public:
	virtual ~IPRSavePayloadCodec() = default;

	virtual bool SaveToMemory(UPRSaveGame& SaveGame, TArray<uint8>& OutPayload) = 0;
	virtual UObject* LoadFromMemory(const TArray<uint8>& Payload) = 0;
};

struct PROJECTR_API FPRSaveDecodedEnvelope
{
	UPRSaveGame* SaveGame = nullptr;
	int32 SourceSchemaVersion = 0;
	bool bNeedsResave = false;
	TArray<uint8> Payload;
	TArray<uint8> Envelope;
};

/** Stable PRSV framing around Unreal's SaveGame payload. */
class PROJECTR_API FPRSaveEnvelope
{
public:
	static constexpr uint16 EnvelopeVersion = 1;
	static constexpr uint16 HeaderSize = 16;
	static constexpr uint32 MaxPayloadSize = 16u * 1024u * 1024u;

	static EPRSaveResult Serialize(const UPRSaveGame& SaveGame, TArray<uint8>& OutEnvelope);
	static EPRSaveResult SerializeWithCodec(
		const UPRSaveGame& SaveGame,
		TArray<uint8>& OutEnvelope,
		IPRSavePayloadCodec& Codec);
	static EPRSaveResult Deserialize(
		const TArray<uint8>& Envelope,
		FPRSaveDecodedEnvelope& OutDecoded,
		const FPRSaveMigrationRegistry* MigrationRegistry = nullptr);
	static EPRSaveResult DeserializeWithCodec(
		const TArray<uint8>& Envelope,
		FPRSaveDecodedEnvelope& OutDecoded,
		IPRSavePayloadCodec& Codec,
		const FPRSaveMigrationRegistry* MigrationRegistry = nullptr);
};

struct PROJECTR_API FPRSaveAccessRecord
{
	FString Operation;
	FString Slot;
};

/** Centralized, exact slot-name policy. */
class PROJECTR_API FPRSaveSlotPolicy
{
public:
	static void BuildProductionSlots(FString& OutSlotA, FString& OutSlotB);
	static bool BuildAutomationSlots(const FString& Base, FString& OutSlotA, FString& OutSlotB);

#if WITH_DEV_AUTOMATION_TESTS
	static bool IsAutomationGenerationSlot(const FString& Slot);
#endif
};

/** Injected storage seam; production implementation delegates only to ISaveGameSystem. */
class PROJECTR_API IPRSaveStorageBackend
{
public:
	virtual ~IPRSaveStorageBackend() = default;

	virtual ISaveGameSystem::ESaveExistsResult DoesSaveGameExist(const FString& Slot) = 0;
	virtual bool LoadGame(const FString& Slot, TArray<uint8>& OutData) = 0;
	virtual void SaveGameAsync(
		const FString& Slot,
		TSharedRef<const TArray<uint8>> Data,
		TFunction<void(bool)> Completion) = 0;
	virtual void LoadGameAsync(
		const FString& Slot,
		TFunction<void(bool, TArray<uint8>)> Completion) = 0;

#if WITH_DEV_AUTOMATION_TESTS
	virtual bool DeleteGame(const FString& Slot) = 0;
#endif
};

struct PROJECTR_API FPRSaveGenerationRead
{
	EPRSaveGeneration Generation = EPRSaveGeneration::None;
	EPRSaveResult Result = EPRSaveResult::ReadFailed;
	ISaveGameSystem::ESaveExistsResult ExistsResult = ISaveGameSystem::ESaveExistsResult::UnspecifiedError;
	UPRSaveGame* SaveGame = nullptr;
	int32 SourceSchemaVersion = 0;
	bool bNeedsResave = false;
	TArray<uint8> Payload;
	TArray<uint8> RawEnvelope;
};

/** Authorized A/B access wrapper used by the subsystem and automation fixtures. */
class PROJECTR_API FPRSaveStorage : public TSharedFromThis<FPRSaveStorage>
{
public:
	static TSharedRef<FPRSaveStorage> CreateProduction();

#if WITH_DEV_AUTOMATION_TESTS
	static TSharedPtr<FPRSaveStorage> CreateAutomation(
		const FString& Base,
		TSharedRef<IPRSaveStorageBackend> Backend,
		TSharedPtr<IPRSavePayloadCodec> Codec = nullptr);
	static TSharedPtr<FPRSaveStorage> CreateAutomation(const FString& Base);
#endif

	ISaveGameSystem::ESaveExistsResult CheckExists(EPRSaveGeneration Generation);
	FPRSaveGenerationRead ReadGenerationSync(EPRSaveGeneration Generation);
	void SaveGenerationAsync(
		EPRSaveGeneration Generation,
		TSharedRef<const TArray<uint8>> Data,
		TFunction<void(bool)> Completion);
	void LoadGenerationAsync(
		EPRSaveGeneration Generation,
		TFunction<void(bool, TArray<uint8>)> Completion);
	EPRSaveResult SerializeEnvelope(const UPRSaveGame& SaveGame, TArray<uint8>& OutEnvelope);
	EPRSaveResult DeserializeEnvelope(
		const TArray<uint8>& Envelope,
		FPRSaveDecodedEnvelope& OutDecoded,
		const FPRSaveMigrationRegistry* MigrationRegistry = nullptr);

#if WITH_DEV_AUTOMATION_TESTS
	bool DeleteGeneration(EPRSaveGeneration Generation);
#endif

	const FString& GetSlot(EPRSaveGeneration Generation) const;
	const TArray<FPRSaveAccessRecord>& GetAccessRecords() const;

private:
	FPRSaveStorage(
		FString InSlotA,
		FString InSlotB,
		TSharedRef<IPRSaveStorageBackend> InBackend,
		TSharedRef<IPRSavePayloadCodec> InCodec,
		bool bInAllowsAutomationCleanup);
	bool AuthorizeAndRecord(const TCHAR* Operation, const FString& Slot);

	FString SlotA;
	FString SlotB;
	TSharedRef<IPRSaveStorageBackend> Backend;
	TSharedRef<IPRSavePayloadCodec> Codec;
	bool bAllowsAutomationCleanup = false;
	TArray<FPRSaveAccessRecord> AccessRecords;
};
