// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memory/PRMockPostRunDialogueProvider.h"

#include "Memory/PRMemoryPersonaDataAsset.h"
#include "Memory/PRMemoryRegistryDataAsset.h"
#include "Misc/Crc.h"

FPRMockPostRunDialogueProvider::FPRMockPostRunDialogueProvider(const UPRMemoryRegistryDataAsset* InRegistry)
	: Registry(InRegistry)
{
}

bool FPRMockPostRunDialogueProvider::BeginRequest(const FPRPostRunDialogueRequest& Request, FCompletion Completion)
{
	if (!Registry || !Registry->IsRegistryReady() || !Request.RequestId.IsValid() || !Request.SummaryId.IsValid() || !Completion) return false;
	const UPRMemoryPersonaDataAsset* Persona = Registry->FindPersona(Request.CompanionId);
	if (!Persona || !Persona->IsDefinitionValid() || CancelledRequests.Contains(Request.RequestId)) return false;
	FString Canonical = Request.SummaryId.ToString(EGuidFormats::Digits) + TEXT("|") + Request.CompanionId.ToString();
	for (const FName KeyEventId : Request.KeyEventIds) Canonical += TEXT("|") + KeyEventId.ToString();
	const uint32 Hash = FCrc::StrCrc32(*Canonical);
	const int32 Index = static_cast<int32>(Hash % 3u);
	FPRPostRunDialogueCandidate Candidate;
	Candidate.SceneId = TEXT("post_run_summary");
	Candidate.CompanionId = Persona->ProviderCompanionId;
	Candidate.EmotionId = Persona->EmotionIds[Index];
	Candidate.Summary = Persona->SummaryTemplates[Index].ToString();
	for (const FPRMemoryPlayerOptionDefinition& Option : Persona->PlayerOptions) Candidate.PlayerOptionIds.Add(Option.OptionId);
	Completion(Request.RequestId, Candidate, { TEXT("scene"), TEXT("companion_id"), TEXT("emotion"), TEXT("summary"), TEXT("player_options") });
	return true;
}

void FPRMockPostRunDialogueProvider::CancelRequest(const FGuid& RequestId)
{
	if (RequestId.IsValid()) CancelledRequests.Add(RequestId);
}
