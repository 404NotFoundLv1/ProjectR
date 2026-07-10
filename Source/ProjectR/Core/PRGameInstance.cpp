// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/PRGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "ProjectR.h"

namespace PRMapPaths
{
	static const TCHAR* MainMenu = TEXT("/Game/ProjectR/Maps/L_MainMenu.L_MainMenu");
	static const TCHAR* RealityHub = TEXT("/Game/ProjectR/Maps/L_RealityHub.L_RealityHub");
	static const TCHAR* NetworkPrototype = TEXT("/Game/ProjectR/Maps/L_Network_Prototype.L_Network_Prototype");
	static const TCHAR* CombatGym = TEXT("/Game/ProjectR/Maps/L_CombatGym.L_CombatGym");
	static const TCHAR* BossGym = TEXT("/Game/ProjectR/Maps/L_BossGym.L_BossGym");
}

bool UPRGameInstance::OpenMap(const EPRMapId MapId)
{
	if (GetWorld() == nullptr)
	{
		UE_LOG(LogProjectR, Error, TEXT("OpenMap rejected because the GameInstance has no World."));
		return false;
	}

	const TSoftObjectPtr<UWorld> Map = ResolveMap(MapId);
	if (Map.IsNull())
	{
		UE_LOG(LogProjectR, Error, TEXT("OpenMap rejected unknown EPRMapId value %d."), static_cast<int32>(MapId));
		return false;
	}

	UE_LOG(LogProjectR, Log, TEXT("OpenMap accepted %s."), *Map.ToSoftObjectPath().ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Map);
	return true;
}

TSoftObjectPtr<UWorld> UPRGameInstance::ResolveMap(const EPRMapId MapId)
{
	switch (MapId)
	{
	case EPRMapId::MainMenu:
		return TSoftObjectPtr<UWorld>(FSoftObjectPath(PRMapPaths::MainMenu));
	case EPRMapId::RealityHub:
		return TSoftObjectPtr<UWorld>(FSoftObjectPath(PRMapPaths::RealityHub));
	case EPRMapId::NetworkPrototype:
		return TSoftObjectPtr<UWorld>(FSoftObjectPath(PRMapPaths::NetworkPrototype));
	case EPRMapId::CombatGym:
		return TSoftObjectPtr<UWorld>(FSoftObjectPath(PRMapPaths::CombatGym));
	case EPRMapId::BossGym:
		return TSoftObjectPtr<UWorld>(FSoftObjectPath(PRMapPaths::BossGym));
	default:
		return TSoftObjectPtr<UWorld>();
	}
}
