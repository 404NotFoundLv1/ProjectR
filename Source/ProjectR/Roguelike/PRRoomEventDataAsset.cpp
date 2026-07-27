// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PRRoomEventDataAsset.h"

FPrimaryAssetId UPRRoomEventDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRRoomEvent")), GetFName());
}

bool UPRRoomEventDataAsset::IsEventDefinitionValid() const
{
	if (!EventId.IsValid() || EventId.PrimaryAssetType != FPrimaryAssetType(TEXT("ProjectRRoomEvent")) || EventId != GetPrimaryAssetId() || DisplayName.IsEmpty() || Description.IsEmpty() || Choices.Num() < 2 || Choices.Num() > 3)
	{
		return false;
	}
	TSet<FName> Seen;
	bool bHasNoDeltaChoice = false;
	for (const FPRRoomEventChoice& Choice : Choices)
	{
		if (Choice.ChoiceId.IsNone() || Choice.DisplayName.IsEmpty() || Seen.Contains(Choice.ChoiceId)) return false;
		bHasNoDeltaChoice |= Choice.RelationshipDelta.TrustDelta == 0 && Choice.RelationshipDelta.AffectionDelta == 0 && Choice.RelationshipDelta.EvaluationDelta == 0 && Choice.RelationshipDelta.OverloadDelta == 0;
		Seen.Add(Choice.ChoiceId);
	}
	return bHasNoDeltaChoice;
}
