// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/Logging/PRLogSanitizer.h"

#include "Misc/SecureHash.h"

FString FPRLogSanitizer::ToOpaqueGuidToken(const FGuid& Value)
{
	if (!Value.IsValid())
	{
		return TEXT("prg_invalid");
	}

	const FString CanonicalValue = Value.ToString(EGuidFormats::DigitsWithHyphensLower);
	const FString DomainSeparatedValue = FString::Printf(
		TEXT("ProjectR.LogGuid.v1:%s"),
		*CanonicalValue);
	FTCHARToUTF8 Utf8Value(*DomainSeparatedValue);
	uint8 Digest[FSHA1::DigestSize];
	FSHA1::HashBuffer(Utf8Value.Get(), Utf8Value.Length(), Digest);
	return FString::Printf(TEXT("prg_%s"), *BytesToHex(Digest, 6).ToLower());
}

FString FPRLogSanitizer::RedactedValue()
{
	return TEXT("[REDACTED]");
}
