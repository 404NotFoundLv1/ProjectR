// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Save/PRSaveGame.h"
#include "Save/PRSaveMigration.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"
#include "Save/PRSaveTypes.h"

#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "UObject/UnrealType.h"

namespace PRSaveAutomation
{
constexpr EAutomationTestFlags TestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

void WriteUInt16LE(TArray<uint8>& Bytes, const int32 Offset, const uint16 Value)
{
	Bytes[Offset] = static_cast<uint8>(Value & 0xffu);
	Bytes[Offset + 1] = static_cast<uint8>((Value >> 8u) & 0xffu);
}

void WriteUInt32LE(TArray<uint8>& Bytes, const int32 Offset, const uint32 Value)
{
	Bytes[Offset] = static_cast<uint8>(Value & 0xffu);
	Bytes[Offset + 1] = static_cast<uint8>((Value >> 8u) & 0xffu);
	Bytes[Offset + 2] = static_cast<uint8>((Value >> 16u) & 0xffu);
	Bytes[Offset + 3] = static_cast<uint8>((Value >> 24u) & 0xffu);
}

TArray<uint8> BuildEnvelopeFromPayload(const TArray<uint8>& Payload)
{
	TArray<uint8> Envelope;
	Envelope.SetNumZeroed(FPRSaveEnvelope::HeaderSize);
	Envelope[0] = 'P';
	Envelope[1] = 'R';
	Envelope[2] = 'S';
	Envelope[3] = 'V';
	WriteUInt16LE(Envelope, 4, FPRSaveEnvelope::EnvelopeVersion);
	WriteUInt16LE(Envelope, 6, FPRSaveEnvelope::HeaderSize);
	WriteUInt32LE(Envelope, 8, static_cast<uint32>(Payload.Num()));
	WriteUInt32LE(Envelope, 12, FCrc::MemCrc32(Payload.GetData(), Payload.Num()));
	Envelope.Append(Payload);
	return Envelope;
}

UPRSaveGame* MakeSave(const int64 Revision, const FGuid ProfileId = FGuid::NewGuid())
{
	UPRSaveGame* Save = NewObject<UPRSaveGame>();
	Save->SchemaVersion = UPRSaveGame::CurrentSchemaVersion;
	Save->SaveRevision = Revision;
	Save->Profile.ProfileId = ProfileId;
	return Save;
}

TArray<uint8> SerializeSave(UPRSaveGame& Save)
{
	TArray<uint8> Envelope;
	if (FPRSaveEnvelope::Serialize(Save, Envelope) != EPRSaveResult::Success)
	{
		Envelope.Reset();
	}
	return Envelope;
}

class FFakeStorageBackend final : public IPRSaveStorageBackend
{
public:
	struct FPendingSave
	{
		FString Slot;
		TArray<uint8> Data;
		TFunction<void(bool)> Completion;
	};

	virtual ISaveGameSystem::ESaveExistsResult DoesSaveGameExist(const FString& Slot) override
	{
		if (IndeterminateSlots.Contains(Slot))
		{
			return ISaveGameSystem::ESaveExistsResult::UnspecifiedError;
		}
		if (CorruptSlots.Contains(Slot))
		{
			return ISaveGameSystem::ESaveExistsResult::Corrupt;
		}
		return Slots.Contains(Slot)
			? ISaveGameSystem::ESaveExistsResult::OK
			: ISaveGameSystem::ESaveExistsResult::DoesNotExist;
	}

	virtual bool LoadGame(const FString& Slot, TArray<uint8>& OutData) override
	{
		if (bFailSyncLoad || CorruptSlots.Contains(Slot))
		{
			return false;
		}
		const TArray<uint8>* Found = Slots.Find(Slot);
		if (!Found)
		{
			return false;
		}
		OutData = *Found;
		return true;
	}

	virtual void SaveGameAsync(
		const FString& Slot,
		TSharedRef<const TArray<uint8>> Data,
		TFunction<void(bool)> Completion) override
	{
		if (bDeferSaves)
		{
			FPendingSave& Pending = PendingSaves.AddDefaulted_GetRef();
			Pending.Slot = Slot;
			Pending.Data = *Data;
			Pending.Completion = MoveTemp(Completion);
			return;
		}
		if (!bFailNextSave)
		{
			Slots.Add(Slot, *Data);
		}
		const bool bSuccess = !bFailNextSave;
		bFailNextSave = false;
		Completion(bSuccess);
	}

	virtual void LoadGameAsync(
		const FString& Slot,
		TFunction<void(bool, TArray<uint8>)> Completion) override
	{
		if (bFailNextAsyncLoad)
		{
			bFailNextAsyncLoad = false;
			Completion(false, TArray<uint8>());
			return;
		}
		TArray<uint8> Data;
		const bool bLoaded = LoadGame(Slot, Data);
		if (bLoaded && bTamperNextAsyncLoad && !Data.IsEmpty())
		{
			bTamperNextAsyncLoad = false;
			Data.Last() ^= 0xffu;
		}
		Completion(bLoaded, MoveTemp(Data));
	}

	virtual bool DeleteGame(const FString& Slot) override
	{
		CorruptSlots.Remove(Slot);
		IndeterminateSlots.Remove(Slot);
		return Slots.Remove(Slot) > 0;
	}

	void CompleteNextSave(const bool bSuccess)
	{
		check(!PendingSaves.IsEmpty());
		FPendingSave Pending = MoveTemp(PendingSaves[0]);
		PendingSaves.RemoveAt(0);
		if (bSuccess)
		{
			Slots.Add(Pending.Slot, Pending.Data);
		}
		Pending.Completion(bSuccess);
	}

	TMap<FString, TArray<uint8>> Slots;
	TSet<FString> CorruptSlots;
	TSet<FString> IndeterminateSlots;
	TArray<FPendingSave> PendingSaves;
	bool bFailSyncLoad = false;
	bool bFailNextSave = false;
	bool bFailNextAsyncLoad = false;
	bool bTamperNextAsyncLoad = false;
	bool bDeferSaves = false;
};

class FFailingPayloadCodec final : public IPRSavePayloadCodec
{
public:
	virtual bool SaveToMemory(UPRSaveGame&, TArray<uint8>&) override { return false; }
	virtual UObject* LoadFromMemory(const TArray<uint8>&) override { return nullptr; }
};

class FWrongClassPayloadCodec final : public IPRSavePayloadCodec
{
public:
	virtual bool SaveToMemory(UPRSaveGame&, TArray<uint8>& OutPayload) override
	{
		OutPayload = {1, 2, 3, 4};
		return true;
	}
	virtual UObject* LoadFromMemory(const TArray<uint8>&) override
	{
		return NewObject<UGameInstance>();
	}
};

struct FPhysicalSaveState
{
	FAutomationTestBase* Test = nullptr;
	TSharedPtr<FPRSaveStorage> Storage;
	TArray<uint8> ExpectedEnvelope;
	double Deadline = 0.0;
	bool bSaveCallback = false;
	bool bSaveSuccess = false;
	bool bLoadStarted = false;
	bool bLoadCallback = false;
	bool bLoadSuccess = false;
	TArray<uint8> LoadedEnvelope;
};
} // namespace PRSaveAutomation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRSaveSchemaTest,
	"ProjectR.Save.Schema.TypesAndTransientExclusions",
	PRSaveAutomation::TestFlags)

bool FPRSaveSchemaTest::RunTest(const FString& Parameters)
{
	const UPRSaveGame* SaveGame = GetDefault<UPRSaveGame>();
	TestEqual(TEXT("Current schema is one"), UPRSaveGame::CurrentSchemaVersion, 1);
	TestEqual(TEXT("Minimum migratable schema is one"), UPRSaveGame::MinimumMigratableVersion, 1);
	TestEqual(TEXT("Schema defaults missing"), SaveGame->SchemaVersion, 0);
	TestEqual(TEXT("Revision defaults zero"), SaveGame->SaveRevision, int64{0});
	TestFalse(TEXT("Profile id defaults invalid"), SaveGame->Profile.ProfileId.IsValid());

	TArray<FName> DeclaredProperties;
	for (TFieldIterator<FProperty> It(UPRSaveGame::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		DeclaredProperties.Add(It->GetFName());
		TestTrue(
			FString::Printf(TEXT("%s expresses SaveGame intent"), *It->GetName()),
			It->HasAnyPropertyFlags(CPF_SaveGame));
		TestFalse(
			FString::Printf(TEXT("%s is not transient runtime state"), *It->GetName()),
			It->HasAnyPropertyFlags(CPF_Transient));
	}
	TestEqual(TEXT("Save root has exactly three persistent fields"), DeclaredProperties.Num(), 3);
	if (DeclaredProperties.Num() == 3)
	{
		TestEqual(TEXT("Schema is first"), DeclaredProperties[0], GET_MEMBER_NAME_CHECKED(UPRSaveGame, SchemaVersion));
		TestEqual(TEXT("Revision is second"), DeclaredProperties[1], GET_MEMBER_NAME_CHECKED(UPRSaveGame, SaveRevision));
		TestEqual(TEXT("Profile is third"), DeclaredProperties[2], GET_MEMBER_NAME_CHECKED(UPRSaveGame, Profile));
	}

	const FPRSaveRuntimeState RuntimeState;
	TestEqual(TEXT("Runtime state starts uninitialized"), RuntimeState.State, EPRSaveSubsystemState::Uninitialized);
	TestEqual(TEXT("Runtime result uses explicit sentinel"), RuntimeState.LastResult, EPRSaveResult::InvalidRequest);
	TestFalse(TEXT("Runtime queue is not persisted by default"), RuntimeState.bSaveRequestQueued);
	TestTrue(
		TEXT("Automation generation slot is eligible for test cleanup"),
		FPRSaveSlotPolicy::IsAutomationGenerationSlot(
			TEXT("ProjectR_Automation_0123456789abcdef0123456789abcdef_A")));
	TestFalse(
		TEXT("Editor production slot is never eligible for test cleanup"),
		FPRSaveSlotPolicy::IsAutomationGenerationSlot(TEXT("ProjectR_Editor_Profile_0_A")));
	TestFalse(
		TEXT("Development production slot is never eligible for test cleanup"),
		FPRSaveSlotPolicy::IsAutomationGenerationSlot(TEXT("ProjectR_Development_Profile_0_B")));
	TestFalse(
		TEXT("Shipping production slot is never eligible for test cleanup"),
		FPRSaveSlotPolicy::IsAutomationGenerationSlot(TEXT("ProjectR_Profile_0_A")));
	TSharedRef<FPRSaveStorage> ProductionStorage = FPRSaveStorage::CreateProduction();
	AddExpectedError(
		TEXT("rejected test cleanup for non-automation slot"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	TestFalse(
		TEXT("Production storage rejects test cleanup before platform access"),
		ProductionStorage->DeleteGeneration(EPRSaveGeneration::A));
	TestEqual(
		TEXT("Rejected production cleanup is absent from the access ledger"),
		ProductionStorage->GetAccessRecords().Num(),
		0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRSaveMigrationTest,
	"ProjectR.Save.Migration.VersionGateAndValidation",
	PRSaveAutomation::TestFlags)

bool FPRSaveMigrationTest::RunTest(const FString& Parameters)
{
	FPRSaveMigrationRegistry Registry;
	TestFalse(TEXT("Migration registry starts with no production steps"), Registry.HasRegisteredSteps());
	TestFalse(TEXT("Negative migration source is rejected"), Registry.RegisterStep(-1, 0, [](UPRSaveGame&) { return true; }));
	TestFalse(TEXT("Migration skips are rejected"), Registry.RegisterStep(1, 3, [](UPRSaveGame&) { return true; }));
	TestTrue(TEXT("Migration 1 to 2 is accepted"), Registry.RegisterStep(1, 2, [](UPRSaveGame& Save) { Save.SchemaVersion = 2; return true; }));
	TestFalse(TEXT("Duplicate source migration is rejected"), Registry.RegisterStep(1, 2, [](UPRSaveGame&) { return true; }));
	TestTrue(TEXT("Migration 2 to 3 is accepted"), Registry.RegisterStep(2, 3, [](UPRSaveGame& Save) { Save.SchemaVersion = 3; return true; }));

	UPRSaveGame* Source = PRSaveAutomation::MakeSave(7);
	Source->SchemaVersion = 1;
	UPRSaveGame* Migrated = nullptr;
	TestEqual(TEXT("Strict migration succeeds"), Registry.Migrate(*Source, 3, Migrated), EPRSaveResult::Success);
	TestNotNull(TEXT("Migration returns a copy"), Migrated);
	if (Migrated)
	{
		TestEqual(TEXT("Migrated schema reached target"), Migrated->SchemaVersion, 3);
		TestEqual(TEXT("Migrated revision preserved"), Migrated->SaveRevision, Source->SaveRevision);
		TestEqual(TEXT("Migrated profile preserved"), Migrated->Profile.ProfileId, Source->Profile.ProfileId);
	}
	TestEqual(TEXT("Source schema remains unchanged"), Source->SchemaVersion, 1);

	FPRSaveMigrationRegistry FailingRegistry;
	FailingRegistry.RegisterStep(1, 2, [](UPRSaveGame&) { return false; });
	UPRSaveGame* FailedCopy = nullptr;
	TestEqual(TEXT("Explicit step failure is reported"), FailingRegistry.Migrate(*Source, 2, FailedCopy), EPRSaveResult::MigrationFailed);
	TestNull(TEXT("Failed migration publishes no copy"), FailedCopy);

	FPRSaveMigrationRegistry WrongIncrementRegistry;
	WrongIncrementRegistry.RegisterStep(1, 2, [](UPRSaveGame& Save) { Save.SchemaVersion = 3; return true; });
	TestEqual(TEXT("Wrong version increment is rejected"), WrongIncrementRegistry.Migrate(*Source, 2, FailedCopy), EPRSaveResult::MigrationFailed);
	TestNull(TEXT("Wrong increment publishes no copy"), FailedCopy);

	UPRSaveGame* MissingSchema = PRSaveAutomation::MakeSave(1);
	MissingSchema->SchemaVersion = 0;
	TestEqual(TEXT("Schema zero is missing"), Registry.Migrate(*MissingSchema, 1, FailedCopy), EPRSaveResult::MissingSchemaVersion);
	UPRSaveGame* OldSchema = PRSaveAutomation::MakeSave(1);
	OldSchema->SchemaVersion = -1;
	TestEqual(TEXT("Old schema is unsupported"), Registry.Migrate(*OldSchema, 1, FailedCopy), EPRSaveResult::UnsupportedOldVersion);
	TestEqual(TEXT("Future source is blocked"), Registry.Migrate(*Source, 0, FailedCopy), EPRSaveResult::FutureSchemaVersion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRSaveSerializationTest,
	"ProjectR.Save.Serialization.EnvelopeIntegrityAndCorruption",
	PRSaveAutomation::TestFlags)

bool FPRSaveSerializationTest::RunTest(const FString& Parameters)
{
	UPRSaveGame* Source = PRSaveAutomation::MakeSave(1);
	TArray<uint8> Envelope;
	TestEqual(TEXT("Valid save serializes"), FPRSaveEnvelope::Serialize(*Source, Envelope), EPRSaveResult::Success);
	TestTrue(TEXT("Envelope contains fixed header and payload"), Envelope.Num() > FPRSaveEnvelope::HeaderSize);
	TestEqual(TEXT("Magic P"), Envelope[0], static_cast<uint8>('P'));
	TestEqual(TEXT("Magic R"), Envelope[1], static_cast<uint8>('R'));
	TestEqual(TEXT("Magic S"), Envelope[2], static_cast<uint8>('S'));
	TestEqual(TEXT("Magic V"), Envelope[3], static_cast<uint8>('V'));

	FPRSaveDecodedEnvelope Decoded;
	TestEqual(TEXT("Valid envelope decodes"), FPRSaveEnvelope::Deserialize(Envelope, Decoded), EPRSaveResult::Success);
	TestNotNull(TEXT("Decoded save is ProjectR save"), Decoded.SaveGame);
	if (Decoded.SaveGame)
	{
		TestEqual(TEXT("Decoded profile matches"), Decoded.SaveGame->Profile.ProfileId, Source->Profile.ProfileId);
		TestEqual(TEXT("Decoded revision matches"), Decoded.SaveGame->SaveRevision, Source->SaveRevision);
	}

	TArray<uint8> Empty;
	TestEqual(TEXT("Empty bytes are empty data"), FPRSaveEnvelope::Deserialize(Empty, Decoded), EPRSaveResult::EmptyData);
	TArray<uint8> ShortHeader = {static_cast<uint8>('P')};
	TestEqual(TEXT("Short header is invalid"), FPRSaveEnvelope::Deserialize(ShortHeader, Decoded), EPRSaveResult::InvalidEnvelope);
	TArray<uint8> BadMagic = Envelope;
	BadMagic[0] = 'X';
	TestEqual(TEXT("Bad magic is invalid"), FPRSaveEnvelope::Deserialize(BadMagic, Decoded), EPRSaveResult::InvalidEnvelope);
	TArray<uint8> EnvelopeZero = Envelope;
	PRSaveAutomation::WriteUInt16LE(EnvelopeZero, 4, 0);
	TestEqual(TEXT("Envelope zero is invalid"), FPRSaveEnvelope::Deserialize(EnvelopeZero, Decoded), EPRSaveResult::InvalidEnvelope);
	TArray<uint8> FutureEnvelope = Envelope;
	PRSaveAutomation::WriteUInt16LE(FutureEnvelope, 4, 2);
	TestEqual(TEXT("Future envelope is blocked"), FPRSaveEnvelope::Deserialize(FutureEnvelope, Decoded), EPRSaveResult::FutureEnvelopeVersion);
	TArray<uint8> BadHeaderSize = Envelope;
	PRSaveAutomation::WriteUInt16LE(BadHeaderSize, 6, 15);
	TestEqual(TEXT("Wrong header size is invalid"), FPRSaveEnvelope::Deserialize(BadHeaderSize, Decoded), EPRSaveResult::InvalidEnvelope);
	TArray<uint8> EmptyPayload = Envelope;
	PRSaveAutomation::WriteUInt32LE(EmptyPayload, 8, 0);
	TestEqual(TEXT("Zero payload is empty"), FPRSaveEnvelope::Deserialize(EmptyPayload, Decoded), EPRSaveResult::EmptyData);
	TArray<uint8> OversizedPayload = Envelope;
	PRSaveAutomation::WriteUInt32LE(OversizedPayload, 8, FPRSaveEnvelope::MaxPayloadSize + 1);
	TestEqual(TEXT("Oversized payload is invalid"), FPRSaveEnvelope::Deserialize(OversizedPayload, Decoded), EPRSaveResult::InvalidEnvelope);
	TArray<uint8> Truncated = Envelope;
	Truncated.Pop();
	TestEqual(TEXT("Truncated payload is corrupt"), FPRSaveEnvelope::Deserialize(Truncated, Decoded), EPRSaveResult::CorruptData);
	TArray<uint8> ExtraTail = Envelope;
	ExtraTail.Add(0);
	TestEqual(TEXT("Extra tail is invalid"), FPRSaveEnvelope::Deserialize(ExtraTail, Decoded), EPRSaveResult::InvalidEnvelope);
	TArray<uint8> BadCrc = Envelope;
	BadCrc.Last() ^= 0xffu;
	TestEqual(TEXT("CRC mismatch is corrupt"), FPRSaveEnvelope::Deserialize(BadCrc, Decoded), EPRSaveResult::CorruptData);

	PRSaveAutomation::FWrongClassPayloadCodec WrongClassCodec;
	TestEqual(TEXT("Wrong SaveGame class is classified"), FPRSaveEnvelope::DeserializeWithCodec(Envelope, Decoded, WrongClassCodec), EPRSaveResult::WrongSaveClass);
	PRSaveAutomation::FFailingPayloadCodec FailingCodec;
	TestEqual(TEXT("Codec serialization failure is classified"), FPRSaveEnvelope::SerializeWithCodec(*Source, Empty, FailingCodec), EPRSaveResult::SerializationFailed);

	UPRSaveGame* MissingSchema = PRSaveAutomation::MakeSave(1);
	MissingSchema->SchemaVersion = 0;
	TestEqual(TEXT("Disk schema zero is missing"), FPRSaveEnvelope::Deserialize(PRSaveAutomation::SerializeSave(*MissingSchema), Decoded), EPRSaveResult::MissingSchemaVersion);
	UPRSaveGame* OldSchema = PRSaveAutomation::MakeSave(1);
	OldSchema->SchemaVersion = -1;
	TestEqual(TEXT("Disk old schema is unsupported"), FPRSaveEnvelope::Deserialize(PRSaveAutomation::SerializeSave(*OldSchema), Decoded), EPRSaveResult::UnsupportedOldVersion);
	UPRSaveGame* FutureSchema = PRSaveAutomation::MakeSave(1);
	FutureSchema->SchemaVersion = UPRSaveGame::CurrentSchemaVersion + 1;
	TestEqual(TEXT("Disk future schema is blocked"), FPRSaveEnvelope::Deserialize(PRSaveAutomation::SerializeSave(*FutureSchema), Decoded), EPRSaveResult::FutureSchemaVersion);
	UPRSaveGame* InvalidProfile = PRSaveAutomation::MakeSave(1, FGuid());
	TestEqual(TEXT("Invalid profile is corrupt"), FPRSaveEnvelope::Deserialize(PRSaveAutomation::SerializeSave(*InvalidProfile), Decoded), EPRSaveResult::CorruptData);
	UPRSaveGame* InvalidRevision = PRSaveAutomation::MakeSave(0);
	TestEqual(TEXT("Disk revision zero is corrupt"), FPRSaveEnvelope::Deserialize(PRSaveAutomation::SerializeSave(*InvalidRevision), Decoded), EPRSaveResult::CorruptData);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRSaveRuntimeTest,
	"ProjectR.Save.Runtime.LoadSaveQueueAndRecovery",
	PRSaveAutomation::TestFlags)

bool FPRSaveRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace PRSaveAutomation;
	const FString Base = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> Backend = MakeShared<FFakeStorageBackend>();
	TSharedPtr<FPRSaveStorage> TestStorage = FPRSaveStorage::CreateAutomation(Base, Backend);
	TestTrue(TEXT("Fake automation storage is authorized"), TestStorage.IsValid());

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UPRSaveSubsystem* Subsystem = NewObject<UPRSaveSubsystem>(GameInstance);
	Subsystem->State = EPRSaveSubsystemState::Ready;
	Subsystem->Storage = TestStorage;
	TArray<FPRSaveOperationEvent> Events;
	const FDelegateHandle EventHandle = Subsystem->OnSaveOperation().AddLambda(
		[&Events](const FPRSaveOperationEvent& Event) { Events.Add(Event); });

	FGuid RequestId;
	TestEqual(TEXT("Empty generations load as not found"), Subsystem->LoadDefaultProfile(RequestId), EPRSaveResult::NotFound);
	TestTrue(TEXT("Not-found load allocates request id"), RequestId.IsValid());
	TestFalse(TEXT("Load does not auto-create"), Subsystem->GetSaveRuntimeState().bHasLoadedProfile);
	TestEqual(TEXT("Create succeeds only on empty A/B"), Subsystem->CreateNewDefaultProfile(RequestId), EPRSaveResult::Success);
	TestTrue(TEXT("Create loads a valid profile"), Subsystem->GetSaveRuntimeState().ProfileId.IsValid());
	TestEqual(TEXT("Create revision starts zero"), Subsystem->GetSaveRuntimeState().SaveRevision, int64{0});
	TestTrue(TEXT("Create needs explicit save"), Subsystem->GetSaveRuntimeState().bNeedsResave);

	Backend->bDeferSaves = true;
	FGuid ActiveId;
	FGuid TrailingId;
	FGuid RepeatedTrailingId;
	TestEqual(TEXT("First save starts"), Subsystem->RequestSaveCurrentProfile(ActiveId), EPRSaveRequestStatus::Started);
	TestEqual(TEXT("One physical write is active"), Backend->PendingSaves.Num(), 1);
	TestEqual(TEXT("First duplicate coalesces"), Subsystem->RequestSaveCurrentProfile(TrailingId), EPRSaveRequestStatus::Coalesced);
	TestEqual(TEXT("Later duplicate stays coalesced"), Subsystem->RequestSaveCurrentProfile(RepeatedTrailingId), EPRSaveRequestStatus::Coalesced);
	TestEqual(TEXT("Trailing request id is stable"), TrailingId, RepeatedTrailingId);
	TestTrue(TEXT("Exactly one trailing request is reported"), Subsystem->GetSaveRuntimeState().bSaveRequestQueued);

	Backend->CompleteNextSave(true);
	TestEqual(TEXT("First save publishes revision one"), Subsystem->GetSaveRuntimeState().SaveRevision, int64{1});
	TestEqual(TEXT("Trailing starts after active verification"), Backend->PendingSaves.Num(), 1);
	Backend->CompleteNextSave(true);
	TestEqual(TEXT("Trailing publishes revision two"), Subsystem->GetSaveRuntimeState().SaveRevision, int64{2});
	TestEqual(TEXT("A/B alternation ends on B"), Subsystem->GetSaveRuntimeState().LoadedGeneration, EPRSaveGeneration::B);
	TestEqual(TEXT("Queue drains to ready"), Subsystem->GetSaveRuntimeState().State, EPRSaveSubsystemState::Ready);
	TestFalse(TEXT("No trailing request remains"), Subsystem->GetSaveRuntimeState().bSaveRequestQueued);

	const int64 PublishedRevision = Subsystem->GetSaveRuntimeState().SaveRevision;
	Backend->Slots.FindChecked(TestStorage->GetSlot(EPRSaveGeneration::A)).Last() ^= 0xffu;
	FGuid ConflictId;
	TestEqual(TEXT("External generation change is accepted then diagnosed"), Subsystem->RequestSaveCurrentProfile(ConflictId), EPRSaveRequestStatus::Started);
	TestEqual(TEXT("External change blocks the write"), Subsystem->GetSaveRuntimeState().LastResult, EPRSaveResult::GenerationConflict);
	TestEqual(TEXT("Conflict preserves published revision"), Subsystem->GetSaveRuntimeState().SaveRevision, PublishedRevision);

	Subsystem->OnSaveOperation().Remove(EventHandle);
	TestTrue(TEXT("Create/load/save events were published"), Events.Num() >= 5);

	const FString RecoveryBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> RecoveryBackend = MakeShared<FFakeStorageBackend>();
	TSharedPtr<FPRSaveStorage> RecoveryStorage = FPRSaveStorage::CreateAutomation(RecoveryBase, RecoveryBackend);
	const FGuid RecoveryProfile = FGuid::NewGuid();
	RecoveryBackend->Slots.Add(RecoveryStorage->GetSlot(EPRSaveGeneration::A), SerializeSave(*MakeSave(7, RecoveryProfile)));
	RecoveryBackend->Slots.Add(RecoveryStorage->GetSlot(EPRSaveGeneration::B), {1, 2, 3});
	UGameInstance* RecoveryGameInstance = NewObject<UGameInstance>();
	UPRSaveSubsystem* RecoverySubsystem = NewObject<UPRSaveSubsystem>(RecoveryGameInstance);
	RecoverySubsystem->State = EPRSaveSubsystemState::Ready;
	RecoverySubsystem->Storage = RecoveryStorage;
	TestEqual(TEXT("Valid generation recovers from corrupt alternate"), RecoverySubsystem->LoadDefaultProfile(RequestId), EPRSaveResult::RecoveredFromAlternate);
	TestEqual(TEXT("Recovered revision is selected"), RecoverySubsystem->GetSaveRuntimeState().SaveRevision, int64{7});
	TestTrue(TEXT("Recovery requires explicit repair save"), RecoverySubsystem->GetSaveRuntimeState().bNeedsResave);

	const FString ConflictBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> ConflictBackend = MakeShared<FFakeStorageBackend>();
	TSharedPtr<FPRSaveStorage> ConflictStorage = FPRSaveStorage::CreateAutomation(ConflictBase, ConflictBackend);
	ConflictBackend->Slots.Add(ConflictStorage->GetSlot(EPRSaveGeneration::A), SerializeSave(*MakeSave(3)));
	ConflictBackend->Slots.Add(ConflictStorage->GetSlot(EPRSaveGeneration::B), SerializeSave(*MakeSave(4)));
	UGameInstance* ConflictGameInstance = NewObject<UGameInstance>();
	UPRSaveSubsystem* ConflictSubsystem = NewObject<UPRSaveSubsystem>(ConflictGameInstance);
	ConflictSubsystem->State = EPRSaveSubsystemState::Ready;
	ConflictSubsystem->Storage = ConflictStorage;
	TestEqual(TEXT("Different profiles conflict"), ConflictSubsystem->LoadDefaultProfile(RequestId), EPRSaveResult::GenerationConflict);
	TestFalse(TEXT("Conflict does not publish profile"), ConflictSubsystem->GetSaveRuntimeState().bHasLoadedProfile);

	const FString FailureBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> FailureBackend = MakeShared<FFakeStorageBackend>();
	TSharedPtr<FPRSaveStorage> FailureStorage = FPRSaveStorage::CreateAutomation(FailureBase, FailureBackend);
	UGameInstance* FailureGameInstance = NewObject<UGameInstance>();
	UPRSaveSubsystem* FailureSubsystem = NewObject<UPRSaveSubsystem>(FailureGameInstance);
	FailureSubsystem->State = EPRSaveSubsystemState::Ready;
	FailureSubsystem->Storage = FailureStorage;
	FailureSubsystem->CreateNewDefaultProfile(RequestId);
	FailureBackend->bDeferSaves = true;
	TArray<EPRSaveResult> FailureEvents;
	FailureSubsystem->OnSaveOperation().AddLambda(
		[&FailureEvents](const FPRSaveOperationEvent& Event)
		{
			if (Event.Operation == EPRSaveOperationType::Save)
			{
				FailureEvents.Add(Event.Result);
			}
		});
	FailureSubsystem->RequestSaveCurrentProfile(ActiveId);
	FailureSubsystem->RequestSaveCurrentProfile(TrailingId);
	FailureBackend->CompleteNextSave(false);
	TestEqual(TEXT("Failed active returns ready"), FailureSubsystem->GetSaveRuntimeState().State, EPRSaveSubsystemState::Ready);
	TestEqual(TEXT("Failed active preserves revision zero"), FailureSubsystem->GetSaveRuntimeState().SaveRevision, int64{0});
	TestEqual(TEXT("Active and trailing each receive result"), FailureEvents.Num(), 2);
	if (FailureEvents.Num() == 2)
	{
		TestEqual(TEXT("Active write fails"), FailureEvents[0], EPRSaveResult::WriteFailed);
		TestEqual(TEXT("Trailing is cancelled"), FailureEvents[1], EPRSaveResult::CancelledAfterPriorFailure);
	}

	AddExpectedError(TEXT("non-Game-Thread"), EAutomationExpectedErrorFlags::Contains, 1);
	const EPRSaveResult OffThreadResult = Async(EAsyncExecution::ThreadPool, [FailureSubsystem]()
	{
		FGuid OffThreadId;
		return FailureSubsystem->LoadDefaultProfile(OffThreadId);
	}).Get();
	TestEqual(TEXT("Non-game-thread load is rejected"), OffThreadResult, EPRSaveResult::InvalidRequest);

	const FString VerificationBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> VerificationBackend = MakeShared<FFakeStorageBackend>();
	UPRSaveSubsystem* VerificationSubsystem = NewObject<UPRSaveSubsystem>(NewObject<UGameInstance>());
	VerificationSubsystem->State = EPRSaveSubsystemState::Ready;
	VerificationSubsystem->Storage = FPRSaveStorage::CreateAutomation(VerificationBase, VerificationBackend);
	VerificationSubsystem->CreateNewDefaultProfile(RequestId);
	VerificationBackend->bTamperNextAsyncLoad = true;
	TestEqual(TEXT("Verification fixture save starts"), VerificationSubsystem->RequestSaveCurrentProfile(ActiveId), EPRSaveRequestStatus::Started);
	TestEqual(TEXT("Tampered write-back fails verification"), VerificationSubsystem->GetSaveRuntimeState().LastResult, EPRSaveResult::VerificationFailed);
	TestEqual(TEXT("Verification failure preserves revision"), VerificationSubsystem->GetSaveRuntimeState().SaveRevision, int64{0});
	TestEqual(TEXT("Verification failure remains retryable"), VerificationSubsystem->GetSaveRuntimeState().State, EPRSaveSubsystemState::Ready);

	const FString CodecBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> CodecBackend = MakeShared<FFakeStorageBackend>();
	TSharedPtr<IPRSavePayloadCodec> FailingCodec = MakeShared<FFailingPayloadCodec>();
	UPRSaveSubsystem* CodecSubsystem = NewObject<UPRSaveSubsystem>(NewObject<UGameInstance>());
	CodecSubsystem->State = EPRSaveSubsystemState::Ready;
	CodecSubsystem->Storage = FPRSaveStorage::CreateAutomation(CodecBase, CodecBackend, FailingCodec);
	CodecSubsystem->CreateNewDefaultProfile(RequestId);
	TestEqual(TEXT("Serialization failure request is accepted"), CodecSubsystem->RequestSaveCurrentProfile(ActiveId), EPRSaveRequestStatus::Started);
	TestEqual(TEXT("Serialization failure is diagnosed"), CodecSubsystem->GetSaveRuntimeState().LastResult, EPRSaveResult::SerializationFailed);
	TestTrue(TEXT("Serialization failure writes no slot"), CodecBackend->Slots.IsEmpty());

	const FString FutureBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> FutureBackend = MakeShared<FFakeStorageBackend>();
	TSharedPtr<FPRSaveStorage> FutureStorage = FPRSaveStorage::CreateAutomation(FutureBase, FutureBackend);
	const FGuid FutureProfile = FGuid::NewGuid();
	FutureBackend->Slots.Add(FutureStorage->GetSlot(EPRSaveGeneration::A), SerializeSave(*MakeSave(2, FutureProfile)));
	UPRSaveGame* FutureSave = MakeSave(3, FutureProfile);
	FutureSave->SchemaVersion = UPRSaveGame::CurrentSchemaVersion + 1;
	FutureBackend->Slots.Add(FutureStorage->GetSlot(EPRSaveGeneration::B), SerializeSave(*FutureSave));
	UPRSaveSubsystem* FutureSubsystem = NewObject<UPRSaveSubsystem>(NewObject<UGameInstance>());
	FutureSubsystem->State = EPRSaveSubsystemState::Ready;
	FutureSubsystem->Storage = FutureStorage;
	TestEqual(TEXT("Future schema blocks fallback to valid generation"), FutureSubsystem->LoadDefaultProfile(RequestId), EPRSaveResult::FutureSchemaVersion);
	TestFalse(TEXT("Future barrier publishes no profile"), FutureSubsystem->GetSaveRuntimeState().bHasLoadedProfile);

	const FString EqualBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> EqualBackend = MakeShared<FFakeStorageBackend>();
	TSharedPtr<FPRSaveStorage> EqualStorage = FPRSaveStorage::CreateAutomation(EqualBase, EqualBackend);
	TArray<uint8> EqualEnvelope = SerializeSave(*MakeSave(5));
	EqualBackend->Slots.Add(EqualStorage->GetSlot(EPRSaveGeneration::A), EqualEnvelope);
	EqualBackend->Slots.Add(EqualStorage->GetSlot(EPRSaveGeneration::B), EqualEnvelope);
	UPRSaveSubsystem* EqualSubsystem = NewObject<UPRSaveSubsystem>(NewObject<UGameInstance>());
	EqualSubsystem->State = EPRSaveSubsystemState::Ready;
	EqualSubsystem->Storage = EqualStorage;
	TestEqual(TEXT("Identical equal-revision generations load"), EqualSubsystem->LoadDefaultProfile(RequestId), EPRSaveResult::Success);
	TestEqual(TEXT("Identical equal-revision generations choose A"), EqualSubsystem->GetSaveRuntimeState().LoadedGeneration, EPRSaveGeneration::A);

	const FString OccupiedBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> OccupiedBackend = MakeShared<FFakeStorageBackend>();
	TSharedPtr<FPRSaveStorage> OccupiedStorage = FPRSaveStorage::CreateAutomation(OccupiedBase, OccupiedBackend);
	OccupiedBackend->CorruptSlots.Add(OccupiedStorage->GetSlot(EPRSaveGeneration::A));
	UPRSaveSubsystem* OccupiedSubsystem = NewObject<UPRSaveSubsystem>(NewObject<UGameInstance>());
	OccupiedSubsystem->State = EPRSaveSubsystemState::Ready;
	OccupiedSubsystem->Storage = OccupiedStorage;
	TestEqual(TEXT("Create never overwrites occupied corrupt slot"), OccupiedSubsystem->CreateNewDefaultProfile(RequestId), EPRSaveResult::SlotOccupied);
	TestFalse(TEXT("Occupied create publishes no profile"), OccupiedSubsystem->GetSaveRuntimeState().bHasLoadedProfile);

	const FString ReentrantBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> ReentrantBackend = MakeShared<FFakeStorageBackend>();
	UPRSaveSubsystem* ReentrantSubsystem = NewObject<UPRSaveSubsystem>(NewObject<UGameInstance>());
	ReentrantSubsystem->State = EPRSaveSubsystemState::Ready;
	ReentrantSubsystem->Storage = FPRSaveStorage::CreateAutomation(ReentrantBase, ReentrantBackend);
	ReentrantSubsystem->CreateNewDefaultProfile(RequestId);
	EPRSaveRequestStatus ReentrantStatus = EPRSaveRequestStatus::Invalid;
	ReentrantSubsystem->OnSaveOperation().AddLambda(
		[ReentrantSubsystem, &ReentrantStatus](const FPRSaveOperationEvent& Event)
		{
			if (Event.Operation == EPRSaveOperationType::Save && Event.Result == EPRSaveResult::Success)
			{
				FGuid ReentrantId;
				ReentrantStatus = ReentrantSubsystem->RequestSaveCurrentProfile(ReentrantId);
			}
		});
	ReentrantSubsystem->RequestSaveCurrentProfile(ActiveId);
	TestEqual(TEXT("Completion reentrancy is rejected busy"), ReentrantStatus, EPRSaveRequestStatus::RejectedBusy);

	const FString ShutdownBase = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedRef<FFakeStorageBackend> ShutdownBackend = MakeShared<FFakeStorageBackend>();
	ShutdownBackend->bDeferSaves = true;
	UPRSaveSubsystem* ShutdownSubsystem = NewObject<UPRSaveSubsystem>(NewObject<UGameInstance>());
	ShutdownSubsystem->State = EPRSaveSubsystemState::Ready;
	ShutdownSubsystem->Storage = FPRSaveStorage::CreateAutomation(ShutdownBase, ShutdownBackend);
	ShutdownSubsystem->CreateNewDefaultProfile(RequestId);
	TArray<EPRSaveResult> ShutdownEvents;
	ShutdownSubsystem->OnSaveOperation().AddLambda(
		[&ShutdownEvents](const FPRSaveOperationEvent& Event)
		{
			if (Event.Operation == EPRSaveOperationType::Save)
			{
				ShutdownEvents.Add(Event.Result);
			}
		});
	ShutdownSubsystem->RequestSaveCurrentProfile(ActiveId);
	ShutdownSubsystem->RequestSaveCurrentProfile(TrailingId);
	ShutdownSubsystem->Deinitialize();
	TestEqual(TEXT("Shutdown enters terminal state"), ShutdownSubsystem->GetSaveRuntimeState().State, EPRSaveSubsystemState::ShuttingDown);
	TestTrue(TEXT("Shutdown cancels unstarted trailing request"), ShutdownEvents.Contains(EPRSaveResult::CancelledOnShutdown));
	ShutdownBackend->CompleteNextSave(true);
	TestEqual(TEXT("Late active callback remains silent after shutdown"), ShutdownEvents.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPRSaveFilesystemIsolationTest,
	"ProjectR.Save.Filesystem.IsolationAndCleanup",
	PRSaveAutomation::TestFlags)

bool FPRSaveFilesystemIsolationTest::RunTest(const FString& Parameters)
{
	using namespace PRSaveAutomation;
	TSharedPtr<FPRSaveStorage> SelectedStorage;
	for (int32 Attempt = 0; Attempt < 16; ++Attempt)
	{
		const FString Base = FString::Printf(
			TEXT("ProjectR_Automation_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
		TSharedPtr<FPRSaveStorage> Candidate = FPRSaveStorage::CreateAutomation(Base);
		if (Candidate &&
			Candidate->CheckExists(EPRSaveGeneration::A) == ISaveGameSystem::ESaveExistsResult::DoesNotExist &&
			Candidate->CheckExists(EPRSaveGeneration::B) == ISaveGameSystem::ESaveExistsResult::DoesNotExist)
		{
			SelectedStorage = MoveTemp(Candidate);
			break;
		}
	}
	if (!TestTrue(TEXT("A unique automation-only A/B pair was found"), SelectedStorage.IsValid()))
	{
		return false;
	}

	UPRSaveGame* Save = MakeSave(1);
	TArray<uint8> Envelope;
	if (!TestEqual(TEXT("Physical fixture serializes"), SelectedStorage->SerializeEnvelope(*Save, Envelope), EPRSaveResult::Success))
	{
		return false;
	}

	TSharedRef<FPhysicalSaveState> State = MakeShared<FPhysicalSaveState>();
	State->Test = this;
	State->Storage = SelectedStorage;
	State->ExpectedEnvelope = Envelope;
	State->Deadline = FPlatformTime::Seconds() + 30.0;
	SelectedStorage->SaveGenerationAsync(
		EPRSaveGeneration::A,
		MakeShared<TArray<uint8>>(Envelope),
		[State](const bool bSuccess)
		{
			State->bSaveSuccess = bSuccess;
			State->bSaveCallback = true;
		});

	AddCommand(new FFunctionLatentCommand([State]()
	{
		const auto Cleanup = [State]()
		{
			State->Storage->DeleteGeneration(EPRSaveGeneration::A);
			State->Storage->DeleteGeneration(EPRSaveGeneration::B);
			State->Test->TestEqual(
				TEXT("Automation A is absent after cleanup"),
				State->Storage->CheckExists(EPRSaveGeneration::A),
				ISaveGameSystem::ESaveExistsResult::DoesNotExist);
			State->Test->TestEqual(
				TEXT("Automation B is absent after cleanup"),
				State->Storage->CheckExists(EPRSaveGeneration::B),
				ISaveGameSystem::ESaveExistsResult::DoesNotExist);
			for (const FPRSaveAccessRecord& Record : State->Storage->GetAccessRecords())
			{
				State->Test->TestTrue(
					FString::Printf(TEXT("Ledger slot is authorized: %s"), *Record.Slot),
					Record.Slot == State->Storage->GetSlot(EPRSaveGeneration::A) ||
					Record.Slot == State->Storage->GetSlot(EPRSaveGeneration::B));
				State->Test->TestTrue(
					TEXT("Ledger uses automation namespace"),
					Record.Slot.StartsWith(TEXT("ProjectR_Automation_"), ESearchCase::CaseSensitive));
			}
		};

		if (FPlatformTime::Seconds() > State->Deadline)
		{
			State->Test->AddError(TEXT("Timed out waiting for platform SaveGame callbacks."));
			Cleanup();
			return true;
		}
		if (!State->bSaveCallback)
		{
			return false;
		}
		if (!State->bSaveSuccess)
		{
			State->Test->AddError(TEXT("Platform SaveGameAsync failed for automation slot."));
			Cleanup();
			return true;
		}
		if (!State->bLoadStarted)
		{
			State->bLoadStarted = true;
			State->Storage->LoadGenerationAsync(
				EPRSaveGeneration::A,
				[State](const bool bSuccess, TArray<uint8> Data)
				{
					State->bLoadSuccess = bSuccess;
					State->LoadedEnvelope = MoveTemp(Data);
					State->bLoadCallback = true;
				});
			return false;
		}
		if (!State->bLoadCallback)
		{
			return false;
		}

		State->Test->TestTrue(TEXT("Platform async verification read succeeds"), State->bLoadSuccess);
		State->Test->TestEqual(TEXT("Platform round trip is byte exact"), State->LoadedEnvelope, State->ExpectedEnvelope);
		Cleanup();
		return true;
	}));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
