// Copyright Epic Games, Inc. All Rights Reserved.

#include "Roguelike/PRRoomDataAsset.h"

FPrimaryAssetId UPRRoomDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectRRoom")), GetFName());
}

bool UPRRoomDataAsset::IsRoomDefinitionValid() const
{
	return RoomId.IsValid() && RoomId.PrimaryAssetType == FPrimaryAssetType(TEXT("ProjectRRoom")) && RoomId == GetPrimaryAssetId()
		&& TypeTag.IsValid() && TypeTag.ToString().StartsWith(TEXT("Room.Type."))
		&& !LevelAsset.IsNull() && !DisplayName.IsEmpty() && !Description.IsEmpty();
}
