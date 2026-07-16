// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRPlayerSkillPresentationToolset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/SoundFactory.h"
#include "Misc/FeedbackContext.h"
#include "Misc/PackageName.h"
#include "Sound/SoundWave.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/Package.h"

namespace PRPlayerSkillPresentationToolset
{
constexpr int32 SampleRate = 22050;
constexpr float PeakAmplitude = 0.08f;
constexpr float AttackSeconds = 0.005f;
constexpr float ReleaseSeconds = 0.020f;

struct FPlaceholderSoundDefinition
{
	const TCHAR* PackagePath;
	float StartFrequencyHz;
	float EndFrequencyHz;
	float DurationSeconds;
};

constexpr FPlaceholderSoundDefinition PlaceholderSounds[] = {
	{TEXT("/Game/ProjectR/Audio/Skills/SFX_ShadowThrust"), 520.0f, 760.0f, 0.12f},
	{TEXT("/Game/ProjectR/Audio/Skills/SFX_FireSlash"), 310.0f, 260.0f, 0.18f},
	{TEXT("/Game/ProjectR/Audio/Skills/SFX_ThunderDrop"), 105.0f, 70.0f, 0.24f},
	{TEXT("/Game/ProjectR/Audio/Skills/SFX_AfterimageDodge"), 820.0f, 1040.0f, 0.10f},
	{TEXT("/Game/ProjectR/Audio/Skills/SFX_VectorHook"), 240.0f, 520.0f, 0.16f},
	{TEXT("/Game/ProjectR/Audio/Skills/SFX_CounterProofWall"), 180.0f, 180.0f, 0.22f}};

void AppendUInt16LE(TArray<uint8>& Bytes, const uint16 Value)
{
	Bytes.Add(static_cast<uint8>(Value & 0xff));
	Bytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
}

void AppendUInt32LE(TArray<uint8>& Bytes, const uint32 Value)
{
	Bytes.Add(static_cast<uint8>(Value & 0xff));
	Bytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
	Bytes.Add(static_cast<uint8>((Value >> 16) & 0xff));
	Bytes.Add(static_cast<uint8>((Value >> 24) & 0xff));
}

void AppendFourCC(TArray<uint8>& Bytes, const ANSICHAR* Text)
{
	Bytes.Append(reinterpret_cast<const uint8*>(Text), 4);
}

TArray<uint8> MakeWaveBytes(const FPlaceholderSoundDefinition& Definition)
{
	const int32 SampleCount = FMath::RoundToInt(Definition.DurationSeconds * SampleRate);
	const uint32 DataSize = static_cast<uint32>(SampleCount * sizeof(int16));
	TArray<uint8> Bytes;
	Bytes.Reserve(44 + DataSize);
	AppendFourCC(Bytes, "RIFF");
	AppendUInt32LE(Bytes, 36 + DataSize);
	AppendFourCC(Bytes, "WAVE");
	AppendFourCC(Bytes, "fmt ");
	AppendUInt32LE(Bytes, 16);
	AppendUInt16LE(Bytes, 1);
	AppendUInt16LE(Bytes, 1);
	AppendUInt32LE(Bytes, SampleRate);
	AppendUInt32LE(Bytes, SampleRate * sizeof(int16));
	AppendUInt16LE(Bytes, sizeof(int16));
	AppendUInt16LE(Bytes, 16);
	AppendFourCC(Bytes, "data");
	AppendUInt32LE(Bytes, DataSize);

	float Phase = 0.0f;
	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float TimeSeconds = static_cast<float>(Index) / SampleRate;
		const float Alpha = FMath::Clamp(TimeSeconds / Definition.DurationSeconds, 0.0f, 1.0f);
		const float Frequency = FMath::Lerp(Definition.StartFrequencyHz, Definition.EndFrequencyHz, Alpha);
		const float Attack = FMath::Clamp(TimeSeconds / AttackSeconds, 0.0f, 1.0f);
		const float Release = FMath::Clamp((Definition.DurationSeconds - TimeSeconds) / ReleaseSeconds, 0.0f, 1.0f);
		const float Envelope = FMath::Min(Attack, Release) * PeakAmplitude;
		const int16 Sample = static_cast<int16>(FMath::RoundToInt(
			FMath::Sin(Phase) * Envelope * static_cast<float>(MAX_int16)));
		AppendUInt16LE(Bytes, static_cast<uint16>(Sample));
		Phase += 2.0f * UE_PI * Frequency / SampleRate;
	}
	return Bytes;
}

bool ValidateManifest(FString& OutError)
{
	for (const FPlaceholderSoundDefinition& Definition : PlaceholderSounds)
	{
		if (FPackageName::DoesPackageExist(Definition.PackagePath)
			|| FindObject<USoundWave>(nullptr, Definition.PackagePath) != nullptr)
		{
			OutError = FString::Printf(TEXT("Manifest package already exists: %s"), Definition.PackagePath);
			return false;
		}
	}
	return true;
}

bool CreateSound(const FPlaceholderSoundDefinition& Definition, FString& OutError)
{
	UPackage* Package = CreatePackage(Definition.PackagePath);
	if (!Package)
	{
		OutError = FString::Printf(TEXT("Could not create Package: %s"), Definition.PackagePath);
		return false;
	}
	USoundFactory* Factory = NewObject<USoundFactory>();
	const FString AssetName = FPackageName::GetLongPackageAssetName(Definition.PackagePath);
	const TArray<uint8> WaveBytes = MakeWaveBytes(Definition);
	const uint8* WaveBuffer = WaveBytes.GetData();
	UObject* CreatedObject = Factory->FactoryCreateBinary(
		USoundWave::StaticClass(),
		Package,
		*AssetName,
		RF_Public | RF_Standalone,
		nullptr,
		TEXT("wav"),
		WaveBuffer,
		WaveBytes.GetData() + WaveBytes.Num(),
		GWarn);
	USoundWave* SoundWave = Cast<USoundWave>(CreatedObject);
	if (!SoundWave)
	{
		OutError = FString::Printf(TEXT("Sound factory could not create %s"), Definition.PackagePath);
		return false;
	}
	SoundWave->bLooping = false;
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(SoundWave);
	return true;
}
} // namespace PRPlayerSkillPresentationToolset

UToolCallAsyncResultString* UPRPlayerSkillPresentationToolset::CreateCheckpointEPlaceholderSounds()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	FString Error;
	if (!PRPlayerSkillPresentationToolset::ValidateManifest(Error))
	{
		Result->SetError(Error);
		return Result;
	}
	for (const PRPlayerSkillPresentationToolset::FPlaceholderSoundDefinition& Definition
		: PRPlayerSkillPresentationToolset::PlaceholderSounds)
	{
		if (!PRPlayerSkillPresentationToolset::CreateSound(Definition, Error))
		{
			Result->SetError(Error);
			return Result;
		}
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":6,\"saved\":false,\"sampleRate\":22050}"));
	return Result;
}
