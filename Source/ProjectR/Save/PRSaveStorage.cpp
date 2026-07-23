// Copyright Epic Games, Inc. All Rights Reserved.

#include "Save/PRSaveStorage.h"

#include "Save/PRSaveGame.h"
#include "Save/PRSaveMigration.h"

#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "PlatformFeatures.h"
#include "ProjectR.h"

namespace PRSaveStoragePrivate
{
void AppendUInt16LE(TArray<uint8>& Bytes, const uint16 Value)
{
	Bytes.Add(static_cast<uint8>(Value & 0xffu));
	Bytes.Add(static_cast<uint8>((Value >> 8u) & 0xffu));
}

void AppendUInt32LE(TArray<uint8>& Bytes, const uint32 Value)
{
	Bytes.Add(static_cast<uint8>(Value & 0xffu));
	Bytes.Add(static_cast<uint8>((Value >> 8u) & 0xffu));
	Bytes.Add(static_cast<uint8>((Value >> 16u) & 0xffu));
	Bytes.Add(static_cast<uint8>((Value >> 24u) & 0xffu));
}

uint16 ReadUInt16LE(const TArray<uint8>& Bytes, const int32 Offset)
{
	return static_cast<uint16>(Bytes[Offset]) |
		(static_cast<uint16>(Bytes[Offset + 1]) << 8u);
}

uint32 ReadUInt32LE(const TArray<uint8>& Bytes, const int32 Offset)
{
	return static_cast<uint32>(Bytes[Offset]) |
		(static_cast<uint32>(Bytes[Offset + 1]) << 8u) |
		(static_cast<uint32>(Bytes[Offset + 2]) << 16u) |
		(static_cast<uint32>(Bytes[Offset + 3]) << 24u);
}

bool IsLowerHex(const TCHAR Character)
{
	return (Character >= TEXT('0') && Character <= TEXT('9')) ||
		(Character >= TEXT('a') && Character <= TEXT('f'));
}

template <typename FCallback>
void DispatchToGameThread(FCallback&& Callback)
{
	if (IsInGameThread())
	{
		Callback();
		return;
	}

	AsyncTask(ENamedThreads::GameThread, Forward<FCallback>(Callback));
}

class FPlatformBackend final : public IPRSaveStorageBackend
{
public:
	FPlatformBackend()
		: SaveSystem(IPlatformFeaturesModule::Get().GetSaveGameSystem())
	{
	}

	virtual ISaveGameSystem::ESaveExistsResult DoesSaveGameExist(const FString& Slot) override
	{
		return SaveSystem
			? SaveSystem->DoesSaveGameExistWithResult(*Slot, UserIndex)
			: ISaveGameSystem::ESaveExistsResult::UnspecifiedError;
	}

	virtual bool LoadGame(const FString& Slot, TArray<uint8>& OutData) override
	{
		return SaveSystem && SaveSystem->LoadGame(false, *Slot, UserIndex, OutData);
	}

	virtual void SaveGameAsync(
		const FString& Slot,
		TSharedRef<const TArray<uint8>> Data,
		TFunction<void(bool)> Completion) override
	{
		if (!SaveSystem)
		{
			DispatchToGameThread([Completion = MoveTemp(Completion)]() mutable { Completion(false); });
			return;
		}

		const FPlatformUserId PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(UserIndex);
		SaveSystem->SaveGameAsync(
			false,
			*Slot,
			PlatformUserId,
			Data,
			[Completion = MoveTemp(Completion)](const FString&, FPlatformUserId, const bool bSuccess) mutable
			{
				DispatchToGameThread([Completion = MoveTemp(Completion), bSuccess]() mutable { Completion(bSuccess); });
			});
	}

	virtual void LoadGameAsync(
		const FString& Slot,
		TFunction<void(bool, TArray<uint8>)> Completion) override
	{
		if (!SaveSystem)
		{
			DispatchToGameThread(
				[Completion = MoveTemp(Completion)]() mutable { Completion(false, TArray<uint8>()); });
			return;
		}

		const FPlatformUserId PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(UserIndex);
		SaveSystem->LoadGameAsync(
			false,
			*Slot,
			PlatformUserId,
			[Completion = MoveTemp(Completion)](
				const FString&,
				FPlatformUserId,
				const bool bSuccess,
				const TArray<uint8>& Data) mutable
			{
				TArray<uint8> DataCopy = Data;
				DispatchToGameThread(
					[Completion = MoveTemp(Completion), bSuccess, Data = MoveTemp(DataCopy)]() mutable
					{
						Completion(bSuccess, MoveTemp(Data));
					});
			});
	}

#if WITH_DEV_AUTOMATION_TESTS
	virtual bool DeleteGame(const FString& Slot) override
	{
		return SaveSystem && SaveSystem->DeleteGame(false, *Slot, UserIndex);
	}
#endif

private:
	static constexpr int32 UserIndex = 0;
	ISaveGameSystem* SaveSystem = nullptr;
};

class FUnrealPayloadCodec final : public IPRSavePayloadCodec
{
public:
	virtual bool SaveToMemory(UPRSaveGame& SaveGame, TArray<uint8>& OutPayload) override
	{
		return UGameplayStatics::SaveGameToMemory(&SaveGame, OutPayload);
	}

	virtual UObject* LoadFromMemory(const TArray<uint8>& Payload) override
	{
		return UGameplayStatics::LoadGameFromMemory(Payload);
	}
};
} // namespace PRSaveStoragePrivate

EPRSaveResult FPRSaveEnvelope::Serialize(const UPRSaveGame& SaveGame, TArray<uint8>& OutEnvelope)
{
	PRSaveStoragePrivate::FUnrealPayloadCodec Codec;
	return SerializeWithCodec(SaveGame, OutEnvelope, Codec);
}

EPRSaveResult FPRSaveEnvelope::SerializeWithCodec(
	const UPRSaveGame& SaveGame,
	TArray<uint8>& OutEnvelope,
	IPRSavePayloadCodec& Codec)
{
	check(IsInGameThread());
	OutEnvelope.Reset();
	TArray<uint8> Payload;
	if (!Codec.SaveToMemory(const_cast<UPRSaveGame&>(SaveGame), Payload))
	{
		return EPRSaveResult::SerializationFailed;
	}
	if (Payload.IsEmpty())
	{
		return EPRSaveResult::EmptyData;
	}
	if (Payload.Num() > static_cast<int32>(MaxPayloadSize))
	{
		return EPRSaveResult::SerializationFailed;
	}

	OutEnvelope.Reserve(HeaderSize + Payload.Num());
	OutEnvelope.Add(static_cast<uint8>('P'));
	OutEnvelope.Add(static_cast<uint8>('R'));
	OutEnvelope.Add(static_cast<uint8>('S'));
	OutEnvelope.Add(static_cast<uint8>('V'));
	PRSaveStoragePrivate::AppendUInt16LE(OutEnvelope, EnvelopeVersion);
	PRSaveStoragePrivate::AppendUInt16LE(OutEnvelope, HeaderSize);
	PRSaveStoragePrivate::AppendUInt32LE(OutEnvelope, static_cast<uint32>(Payload.Num()));
	PRSaveStoragePrivate::AppendUInt32LE(OutEnvelope, FCrc::MemCrc32(Payload.GetData(), Payload.Num()));
	OutEnvelope.Append(Payload);
	return EPRSaveResult::Success;
}

EPRSaveResult FPRSaveEnvelope::Deserialize(
	const TArray<uint8>& Envelope,
	FPRSaveDecodedEnvelope& OutDecoded,
	const FPRSaveMigrationRegistry* MigrationRegistry)
{
	PRSaveStoragePrivate::FUnrealPayloadCodec Codec;
	return DeserializeWithCodec(Envelope, OutDecoded, Codec, MigrationRegistry);
}

EPRSaveResult FPRSaveEnvelope::DeserializeWithCodec(
	const TArray<uint8>& Envelope,
	FPRSaveDecodedEnvelope& OutDecoded,
	IPRSavePayloadCodec& Codec,
	const FPRSaveMigrationRegistry* MigrationRegistry)
{
	check(IsInGameThread());
	OutDecoded = FPRSaveDecodedEnvelope();
	if (Envelope.IsEmpty())
	{
		return EPRSaveResult::EmptyData;
	}
	if (Envelope.Num() < HeaderSize)
	{
		return EPRSaveResult::InvalidEnvelope;
	}
	if (Envelope[0] != 'P' || Envelope[1] != 'R' || Envelope[2] != 'S' || Envelope[3] != 'V')
	{
		return EPRSaveResult::InvalidEnvelope;
	}

	const uint16 StoredEnvelopeVersion = PRSaveStoragePrivate::ReadUInt16LE(Envelope, 4);
	const uint16 StoredHeaderSize = PRSaveStoragePrivate::ReadUInt16LE(Envelope, 6);
	if (StoredEnvelopeVersion > EnvelopeVersion)
	{
		return EPRSaveResult::FutureEnvelopeVersion;
	}
	if (StoredEnvelopeVersion == 0 || StoredHeaderSize != HeaderSize)
	{
		return EPRSaveResult::InvalidEnvelope;
	}

	const uint32 PayloadSize = PRSaveStoragePrivate::ReadUInt32LE(Envelope, 8);
	const uint32 StoredCrc = PRSaveStoragePrivate::ReadUInt32LE(Envelope, 12);
	if (PayloadSize == 0)
	{
		return EPRSaveResult::EmptyData;
	}
	if (PayloadSize > MaxPayloadSize)
	{
		return EPRSaveResult::InvalidEnvelope;
	}

	const uint64 ExpectedSize = static_cast<uint64>(HeaderSize) + PayloadSize;
	if (ExpectedSize > static_cast<uint64>(MAX_int32))
	{
		return EPRSaveResult::InvalidEnvelope;
	}
	if (static_cast<uint64>(Envelope.Num()) < ExpectedSize)
	{
		return EPRSaveResult::CorruptData;
	}
	if (static_cast<uint64>(Envelope.Num()) > ExpectedSize)
	{
		return EPRSaveResult::InvalidEnvelope;
	}

	TArray<uint8> Payload;
	Payload.Append(Envelope.GetData() + HeaderSize, static_cast<int32>(PayloadSize));
	if (FCrc::MemCrc32(Payload.GetData(), Payload.Num()) != StoredCrc)
	{
		return EPRSaveResult::CorruptData;
	}

	UObject* LoadedObject = Codec.LoadFromMemory(Payload);
	if (!LoadedObject)
	{
		return EPRSaveResult::CorruptData;
	}
	UPRSaveGame* LoadedSave = Cast<UPRSaveGame>(LoadedObject);
	if (!LoadedSave)
	{
		return EPRSaveResult::WrongSaveClass;
	}

	OutDecoded.SourceSchemaVersion = LoadedSave->SchemaVersion;
	if (LoadedSave->SchemaVersion == 0)
	{
		return EPRSaveResult::MissingSchemaVersion;
	}
	if (LoadedSave->SchemaVersion < UPRSaveGame::MinimumMigratableVersion)
	{
		return EPRSaveResult::UnsupportedOldVersion;
	}
	if (LoadedSave->SchemaVersion > UPRSaveGame::CurrentSchemaVersion)
	{
		return EPRSaveResult::FutureSchemaVersion;
	}
	if (LoadedSave->SchemaVersion < UPRSaveGame::CurrentSchemaVersion)
	{
		if (!MigrationRegistry)
		{
			return EPRSaveResult::MigrationFailed;
		}

		UPRSaveGame* Migrated = nullptr;
		const EPRSaveResult MigrationResult = MigrationRegistry->Migrate(
			*LoadedSave,
			UPRSaveGame::CurrentSchemaVersion,
			Migrated);
		if (MigrationResult != EPRSaveResult::Success || !Migrated)
		{
			return EPRSaveResult::MigrationFailed;
		}
		LoadedSave = Migrated;
		OutDecoded.bNeedsResave = true;
	}

	if (!LoadedSave->Profile.ProfileId.IsValid() || LoadedSave->SaveRevision <= 0
		|| !FPRCompanionContract::AreCanonicalRelationshipRecords(LoadedSave->Profile.CompanionRelationships))
	{
		return EPRSaveResult::CorruptData;
	}

	OutDecoded.SaveGame = LoadedSave;
	OutDecoded.Payload = MoveTemp(Payload);
	OutDecoded.Envelope = Envelope;
	return EPRSaveResult::Success;
}

void FPRSaveSlotPolicy::BuildProductionSlots(FString& OutSlotA, FString& OutSlotB)
{
#if WITH_EDITOR
	const FString Base = TEXT("ProjectR_Editor_Profile_0");
#elif UE_BUILD_SHIPPING
	const FString Base = TEXT("ProjectR_Profile_0");
#else
	const FString Base = TEXT("ProjectR_Development_Profile_0");
#endif
	OutSlotA = Base + TEXT("_A");
	OutSlotB = Base + TEXT("_B");
}

bool FPRSaveSlotPolicy::BuildAutomationSlots(
	const FString& Base,
	FString& OutSlotA,
	FString& OutSlotB)
{
	OutSlotA.Reset();
	OutSlotB.Reset();
	static const FString Prefix = TEXT("ProjectR_Automation_");
	if (!Base.StartsWith(Prefix, ESearchCase::CaseSensitive) || Base.Len() != Prefix.Len() + 32)
	{
		return false;
	}
	for (int32 Index = Prefix.Len(); Index < Base.Len(); ++Index)
	{
		if (!PRSaveStoragePrivate::IsLowerHex(Base[Index]))
		{
			return false;
		}
	}
	OutSlotA = Base + TEXT("_A");
	OutSlotB = Base + TEXT("_B");
	return true;
}

#if WITH_DEV_AUTOMATION_TESTS
bool FPRSaveSlotPolicy::IsAutomationGenerationSlot(const FString& Slot)
{
	if (!Slot.EndsWith(TEXT("_A"), ESearchCase::CaseSensitive) &&
		!Slot.EndsWith(TEXT("_B"), ESearchCase::CaseSensitive))
	{
		return false;
	}

	FString ExpectedA;
	FString ExpectedB;
	const FString Base = Slot.LeftChop(2);
	return BuildAutomationSlots(Base, ExpectedA, ExpectedB) &&
		(Slot == ExpectedA || Slot == ExpectedB);
}
#endif

FPRSaveStorage::FPRSaveStorage(
	FString InSlotA,
	FString InSlotB,
	TSharedRef<IPRSaveStorageBackend> InBackend,
	TSharedRef<IPRSavePayloadCodec> InCodec,
	const bool bInAllowsAutomationCleanup)
	: SlotA(MoveTemp(InSlotA))
	, SlotB(MoveTemp(InSlotB))
	, Backend(MoveTemp(InBackend))
	, Codec(MoveTemp(InCodec))
	, bAllowsAutomationCleanup(bInAllowsAutomationCleanup)
{
}

TSharedRef<FPRSaveStorage> FPRSaveStorage::CreateProduction()
{
	FString ProductionA;
	FString ProductionB;
	FPRSaveSlotPolicy::BuildProductionSlots(ProductionA, ProductionB);
	return MakeShareable(new FPRSaveStorage(
		MoveTemp(ProductionA),
		MoveTemp(ProductionB),
		MakeShared<PRSaveStoragePrivate::FPlatformBackend>(),
		MakeShared<PRSaveStoragePrivate::FUnrealPayloadCodec>(),
		false));
}

#if WITH_DEV_AUTOMATION_TESTS
TSharedPtr<FPRSaveStorage> FPRSaveStorage::CreateAutomation(
	const FString& Base,
	TSharedRef<IPRSaveStorageBackend> InBackend,
	TSharedPtr<IPRSavePayloadCodec> InCodec)
{
	FString AutomationA;
	FString AutomationB;
	if (!FPRSaveSlotPolicy::BuildAutomationSlots(Base, AutomationA, AutomationB))
	{
		return nullptr;
	}
	return MakeShareable(new FPRSaveStorage(
		MoveTemp(AutomationA),
		MoveTemp(AutomationB),
		MoveTemp(InBackend),
		InCodec.IsValid()
			? InCodec.ToSharedRef()
			: MakeShared<PRSaveStoragePrivate::FUnrealPayloadCodec>(),
		true));
}

TSharedPtr<FPRSaveStorage> FPRSaveStorage::CreateAutomation(const FString& Base)
{
	return CreateAutomation(Base, MakeShared<PRSaveStoragePrivate::FPlatformBackend>());
}
#endif

ISaveGameSystem::ESaveExistsResult FPRSaveStorage::CheckExists(const EPRSaveGeneration Generation)
{
	const FString& Slot = GetSlot(Generation);
	if (!AuthorizeAndRecord(TEXT("Exists"), Slot))
	{
		return ISaveGameSystem::ESaveExistsResult::UnspecifiedError;
	}
	return Backend->DoesSaveGameExist(Slot);
}

FPRSaveGenerationRead FPRSaveStorage::ReadGenerationSync(
	const EPRSaveGeneration Generation,
	const FPRSaveMigrationRegistry* MigrationRegistry)
{
	FPRSaveGenerationRead Read;
	Read.Generation = Generation;
	const FString& Slot = GetSlot(Generation);
	if (!AuthorizeAndRecord(TEXT("Exists"), Slot))
	{
		return Read;
	}
	Read.ExistsResult = Backend->DoesSaveGameExist(Slot);
	if (Read.ExistsResult == ISaveGameSystem::ESaveExistsResult::DoesNotExist)
	{
		Read.Result = EPRSaveResult::NotFound;
		return Read;
	}
	if (Read.ExistsResult == ISaveGameSystem::ESaveExistsResult::Corrupt)
	{
		Read.Result = EPRSaveResult::CorruptData;
		return Read;
	}
	if (Read.ExistsResult != ISaveGameSystem::ESaveExistsResult::OK)
	{
		Read.Result = EPRSaveResult::ReadFailed;
		return Read;
	}
	if (!AuthorizeAndRecord(TEXT("Load"), Slot) || !Backend->LoadGame(Slot, Read.RawEnvelope))
	{
		Read.Result = EPRSaveResult::ReadFailed;
		return Read;
	}

	FPRSaveDecodedEnvelope Decoded;
	Read.Result = DeserializeEnvelope(Read.RawEnvelope, Decoded, MigrationRegistry);
	Read.SaveGame = Decoded.SaveGame;
	Read.SourceSchemaVersion = Decoded.SourceSchemaVersion;
	Read.bNeedsResave = Decoded.bNeedsResave;
	Read.Payload = MoveTemp(Decoded.Payload);
	return Read;
}

void FPRSaveStorage::SaveGenerationAsync(
	const EPRSaveGeneration Generation,
	TSharedRef<const TArray<uint8>> Data,
	TFunction<void(bool)> Completion)
{
	const FString& Slot = GetSlot(Generation);
	if (!AuthorizeAndRecord(TEXT("SaveAsync"), Slot))
	{
		Completion(false);
		return;
	}
	Backend->SaveGameAsync(Slot, Data, MoveTemp(Completion));
}

void FPRSaveStorage::LoadGenerationAsync(
	const EPRSaveGeneration Generation,
	TFunction<void(bool, TArray<uint8>)> Completion)
{
	const FString& Slot = GetSlot(Generation);
	if (!AuthorizeAndRecord(TEXT("LoadAsync"), Slot))
	{
		Completion(false, TArray<uint8>());
		return;
	}
	Backend->LoadGameAsync(Slot, MoveTemp(Completion));
}

EPRSaveResult FPRSaveStorage::SerializeEnvelope(
	const UPRSaveGame& SaveGame,
	TArray<uint8>& OutEnvelope)
{
	return FPRSaveEnvelope::SerializeWithCodec(SaveGame, OutEnvelope, Codec.Get());
}

EPRSaveResult FPRSaveStorage::DeserializeEnvelope(
	const TArray<uint8>& Envelope,
	FPRSaveDecodedEnvelope& OutDecoded,
	const FPRSaveMigrationRegistry* MigrationRegistry)
{
	return FPRSaveEnvelope::DeserializeWithCodec(Envelope, OutDecoded, Codec.Get(), MigrationRegistry);
}

#if WITH_DEV_AUTOMATION_TESTS
bool FPRSaveStorage::DeleteGeneration(const EPRSaveGeneration Generation)
{
	const FString& Slot = GetSlot(Generation);
	if (!bAllowsAutomationCleanup || !FPRSaveSlotPolicy::IsAutomationGenerationSlot(Slot))
	{
		UE_LOG(
			LogProjectR,
			Error,
			TEXT("Save storage rejected test cleanup for non-automation slot %s."),
			*Slot);
		return false;
	}
	return AuthorizeAndRecord(TEXT("Delete"), Slot) && Backend->DeleteGame(Slot);
}
#endif

const FString& FPRSaveStorage::GetSlot(const EPRSaveGeneration Generation) const
{
	check(Generation == EPRSaveGeneration::A || Generation == EPRSaveGeneration::B);
	return Generation == EPRSaveGeneration::A ? SlotA : SlotB;
}

const TArray<FPRSaveAccessRecord>& FPRSaveStorage::GetAccessRecords() const
{
	return AccessRecords;
}

bool FPRSaveStorage::AuthorizeAndRecord(const TCHAR* Operation, const FString& Slot)
{
	if (Slot != SlotA && Slot != SlotB)
	{
		UE_LOG(LogProjectR, Error, TEXT("Save storage rejected unauthorized slot %s for %s."), *Slot, Operation);
		return false;
	}

	FPRSaveAccessRecord& Record = AccessRecords.AddDefaulted_GetRef();
	Record.Operation = Operation;
	Record.Slot = Slot;
	return true;
}
