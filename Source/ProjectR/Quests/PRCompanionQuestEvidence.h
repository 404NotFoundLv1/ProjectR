// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Divergence/PRDivergenceTypes.h"
#include "Roguelike/PRRoomTypes.h"
#include "Save/PRAccountSaveTypes.h"

/** Fixed v0.5.1 quest facts. These consume only published value records. */
struct PROJECTR_API FPRCompanionQuestEvidenceContract
{
	static bool IsAxiomLowProbabilitySample(const FPRRoomEventResult& Event, const FGameplayTag PrimaryCompanionId);
	static bool IsAxiomRescueCandidate(const FPRDivergenceResult& Event);
	static bool IsAxiomImperfectOptimum(const FPRAccountRecord& Record, const FGuid& RescueEvidenceId);
	static bool IsKindleNoRetreat(const FPRAccountRecord& Record);
	static bool IsKindleLearnToRetreat(const FPRAccountRecord& Record);
	static bool IsNullGarbageCollection(const FPRAccountRecord& Record);
	static bool HasFiveUniqueGraveyardRecords(const TArray<FPRAccountRecord>& Records);
};
