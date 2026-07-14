// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDebugBuiltInCommands.h"

#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "Characters/PRPlayerCharacter.h"
#include "Combat/PRCombatSubsystem.h"
#include "Core/PRGameInstance.h"
#include "Core/PRMapId.h"
#include "Core/PRPlayerState.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "Misc/PackageName.h"
#include "Save/PRSaveGame.h"
#include "Save/PRSaveSubsystem.h"

#define LOCTEXT_NAMESPACE "ProjectRDebugBuiltIns"

namespace
{
FPRDebugCommandDescriptor MakeDescriptor(
	EPRDebugCommandId CommandId,
	const TCHAR* StableName,
	const FText& DisplayName,
	const FText& Description,
	EPRDebugCommandAvailability Availability,
	bool bChangesRuntimeState)
{
	FPRDebugCommandDescriptor Descriptor;
	Descriptor.CommandId = CommandId;
	Descriptor.StableName = StableName;
	Descriptor.DisplayName = DisplayName;
	Descriptor.Description = Description;
	Descriptor.Availability = Availability;
	Descriptor.bChangesRuntimeState = bChangesRuntimeState;
	return Descriptor;
}

FPRDebugArgumentSpec MakeFloatArgument(
	const TCHAR* ArgumentId,
	double Minimum,
	double Maximum,
	double DefaultValue)
{
	FPRDebugArgumentSpec Argument;
	Argument.ArgumentId = ArgumentId;
	Argument.Type = EPRDebugArgumentType::Float;
	Argument.bRequired = true;
	Argument.MinValue = Minimum;
	Argument.MaxValue = Maximum;
	Argument.DefaultValue = DefaultValue;
	return Argument;
}

FPRDebugCommandResult MakeResult(
	const FPRDebugCommandRequest& Request,
	EPRDebugCommandResultCode ResultCode,
	const FText& Message)
{
	FPRDebugCommandResult Result;
	Result.RequestId = Request.RequestId.IsValid() ? Request.RequestId : FGuid::NewGuid();
	Result.CommandId = Request.CommandId;
	Result.ResultCode = ResultCode;
	Result.PlayerMessage = Message;
	return Result;
}

APRPlayerCharacter* ResolvePlayerCharacter(UGameInstance* GameInstance)
{
	if (GameInstance == nullptr || GameInstance->GetWorld() == nullptr)
	{
		return nullptr;
	}

	APlayerController* Controller = GameInstance->GetFirstLocalPlayerController(GameInstance->GetWorld());
	return Controller != nullptr ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
}

EPRDebugCommandResultCode MapDamageStatus(EPRCombatRequestStatus Status)
{
	switch (Status)
	{
	case EPRCombatRequestStatus::Applied:
		return EPRDebugCommandResultCode::Success;
	case EPRCombatRequestStatus::RejectedInvulnerable:
	case EPRCombatRequestStatus::RejectedDead:
	case EPRCombatRequestStatus::RejectedAlive:
		return EPRDebugCommandResultCode::Rejected;
	default:
		return EPRDebugCommandResultCode::Failed;
	}
}
}

TArray<FPRDebugCommandDescriptor> FPRDebugBuiltInCommands::BuildDescriptors()
{
	TArray<FPRDebugCommandDescriptor> Descriptors;
	Descriptors.Reserve(11);
	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::StatusSnapshot,
		TEXT("Debug.Status"),
		LOCTEXT("StatusName", "Status"),
		LOCTEXT("StatusDescription", "Read the current safe ProjectR runtime summary."),
		EPRDebugCommandAvailability::Available,
		false));

	FPRDebugCommandDescriptor Damage = MakeDescriptor(
		EPRDebugCommandId::DamageSelf,
		TEXT("Combat.DamageSelf"),
		LOCTEXT("DamageName", "Damage Self"),
		LOCTEXT("DamageDescription", "Apply validated damage through the ProjectR combat subsystem."),
		EPRDebugCommandAvailability::Available,
		true);
	Damage.Arguments.Add(MakeFloatArgument(TEXT("Amount"), 1.0, 100000.0, 25.0));
	Descriptors.Add(MoveTemp(Damage));

	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::ReviveSelf,
		TEXT("Combat.ReviveSelf"),
		LOCTEXT("ReviveName", "Revive Self"),
		LOCTEXT("ReviveDescription", "Request a full validated revive through the ProjectR combat subsystem."),
		EPRDebugCommandAvailability::Available,
		true));
	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::SaveRuntimeState,
		TEXT("Save.RuntimeState"),
		LOCTEXT("SaveName", "Save Runtime State"),
		LOCTEXT("SaveDescription", "Read the redacted save subsystem runtime state without storage access."),
		EPRDebugCommandAvailability::Available,
		false));
	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::ReturnToCombatGym,
		TEXT("Travel.CombatGym"),
		LOCTEXT("TravelName", "Travel To Combat Gym"),
		LOCTEXT("TravelDescription", "Open the fixed formal CombatGym map."),
		EPRDebugCommandAvailability::Available,
		true));

	FPRDebugCommandDescriptor GrantResource = MakeDescriptor(
		EPRDebugCommandId::GrantResource,
		TEXT("Ability.GrantResource"),
		LOCTEXT("GrantResourceName", "Grant Resource"),
		LOCTEXT("GrantResourceDescription", "Reserved for a future controlled resource API."),
		EPRDebugCommandAvailability::NotAvailable,
		true);
	FPRDebugArgumentSpec Resource;
	Resource.ArgumentId = TEXT("Resource");
	Resource.Type = EPRDebugArgumentType::EnumChoice;
	Resource.bRequired = true;
	Resource.AllowedChoices = {TEXT("Energy"), TEXT("Permission"), TEXT("Resonance")};
	GrantResource.Arguments.Add(Resource);
	GrantResource.Arguments.Add(MakeFloatArgument(TEXT("Amount"), 1.0, 100000.0, 25.0));
	Descriptors.Add(MoveTemp(GrantResource));

	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::ClearCooldown,
		TEXT("Ability.ClearCooldown"),
		LOCTEXT("ClearCooldownName", "Clear Cooldown"),
		LOCTEXT("ClearCooldownDescription", "Reserved for a future controlled cooldown API."),
		EPRDebugCommandAvailability::NotAvailable,
		true));
	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::GenerateDirectorRule,
		TEXT("Director.GenerateRule"),
		LOCTEXT("DirectorName", "Generate Director Rule"),
		LOCTEXT("DirectorDescription", "Reserved until the Director whitelist API exists."),
		EPRDebugCommandAvailability::NotAvailable,
		true));
	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::SpawnQTE,
		TEXT("QTE.Spawn"),
		LOCTEXT("QTEName", "Spawn QTE"),
		LOCTEXT("QTEDescription", "Reserved until the formal QTE API exists."),
		EPRDebugCommandAvailability::NotAvailable,
		true));
	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::CycleCompanionAI,
		TEXT("Companion.CycleAI"),
		LOCTEXT("CompanionName", "Cycle Companion AI"),
		LOCTEXT("CompanionDescription", "Reserved until the formal companion API exists."),
		EPRDebugCommandAvailability::NotAvailable,
		true));
	Descriptors.Add(MakeDescriptor(
		EPRDebugCommandId::JumpToBoss,
		TEXT("Boss.Jump"),
		LOCTEXT("BossName", "Jump To Boss"),
		LOCTEXT("BossDescription", "Reserved until the formal boss flow exists."),
		EPRDebugCommandAvailability::NotAvailable,
		true));
	return Descriptors;
}

FPRDebugCommandResult FPRDebugBuiltInCommands::Execute(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	switch (Request.CommandId)
	{
	case EPRDebugCommandId::StatusSnapshot:
		return BuildStatus(Request, GameInstance);
	case EPRDebugCommandId::DamageSelf:
		return DamageSelf(Request, GameInstance);
	case EPRDebugCommandId::ReviveSelf:
		return ReviveSelf(Request, GameInstance);
	case EPRDebugCommandId::SaveRuntimeState:
		return ReadSaveState(Request, GameInstance);
	case EPRDebugCommandId::ReturnToCombatGym:
		return TravelToCombatGym(Request, GameInstance);
	default:
		return MakeResult(
			Request,
			EPRDebugCommandResultCode::NotAvailable,
			LOCTEXT("FutureUnavailable", "Command is not available in this version."));
	}
}

FPRDebugCommandResult FPRDebugBuiltInCommands::BuildStatus(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	FPRDebugCommandResult Result = MakeResult(
		Request,
		EPRDebugCommandResultCode::Success,
		LOCTEXT("StatusSuccess", "Status refreshed."));
	Result.bHasStatusSnapshot = true;
	FPRDebugStatusSnapshot& Snapshot = Result.StatusSnapshot;

	UWorld* World = GameInstance != nullptr ? GameInstance->GetWorld() : nullptr;
	if (World != nullptr)
	{
		Snapshot.MapShortName = FPackageName::GetShortFName(World->GetOutermost()->GetFName());
	}

	APlayerController* Controller = GameInstance != nullptr
		? GameInstance->GetFirstLocalPlayerController(World)
		: nullptr;
	Snapshot.bHasLocalPlayer = Controller != nullptr && Controller->GetLocalPlayer() != nullptr;
	Snapshot.bHasPawn = Controller != nullptr && Controller->GetPawn() != nullptr;

	APRPlayerState* PlayerState = Controller != nullptr ? Controller->GetPlayerState<APRPlayerState>() : nullptr;
	UPRAbilitySystemComponent* AbilitySystem = PlayerState != nullptr
		? PlayerState->GetProjectRAbilitySystemComponent()
		: nullptr;
	const UPRAttributeSet* Attributes = PlayerState != nullptr ? PlayerState->GetAttributeSet() : nullptr;
	Snapshot.bHasAbilitySystem = AbilitySystem != nullptr;
	if (Attributes != nullptr)
	{
		Snapshot.Health = Attributes->GetHealth();
		Snapshot.Shield = Attributes->GetShield();
		Snapshot.Energy = Attributes->GetEnergy();
	}
	if (AbilitySystem != nullptr)
	{
		Snapshot.GrantedAbilityCount = AbilitySystem->GetActivatableAbilities().Num();
		for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
		{
			Snapshot.ActiveAbilityCount += Spec.IsActive() ? 1 : 0;
		}
		Snapshot.ActiveEffectCount = AbilitySystem->GetActiveEffects(FGameplayEffectQuery()).Num();
	}

	const UPRSaveSubsystem* SaveSubsystem = GameInstance != nullptr
		? GameInstance->GetSubsystem<UPRSaveSubsystem>()
		: nullptr;
	if (SaveSubsystem != nullptr)
	{
		const FPRSaveRuntimeState SaveState = SaveSubsystem->GetSaveRuntimeState();
		Snapshot.SaveState = SaveState.State;
		Snapshot.bHasLoadedProfile = SaveState.bHasLoadedProfile;
		Snapshot.SaveSchemaVersion = SaveState.SchemaVersion;
		Snapshot.SaveRevision = SaveState.SaveRevision;
		Snapshot.bSaveNeedsResave = SaveState.bNeedsResave;
		Snapshot.bSaveRequestQueued = SaveState.bSaveRequestQueued;
		Snapshot.SaveLastResult = SaveState.LastResult;
		Snapshot.SaveGeneration = SaveState.LoadedGeneration;
	}
	return Result;
}

FPRDebugCommandResult FPRDebugBuiltInCommands::DamageSelf(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	APRPlayerCharacter* Character = ResolvePlayerCharacter(GameInstance);
	UWorld* World = GameInstance != nullptr ? GameInstance->GetWorld() : nullptr;
	UPRCombatSubsystem* CombatSubsystem = World != nullptr ? World->GetSubsystem<UPRCombatSubsystem>() : nullptr;
	APRPlayerState* PlayerState = Character != nullptr ? Character->GetPlayerState<APRPlayerState>() : nullptr;
	if (Character == nullptr || PlayerState == nullptr
		|| PlayerState->GetProjectRAbilitySystemComponent() == nullptr
		|| CombatSubsystem == nullptr)
	{
		return MakeResult(Request, EPRDebugCommandResultCode::InvalidContext, LOCTEXT("DamageInvalid", "Player combat context is not available."));
	}

	const FPRDebugArgumentValue* Amount = Request.Arguments.FindByPredicate([](const FPRDebugArgumentValue& Value)
	{
		return Value.ArgumentId == TEXT("Amount");
	});
	if (Amount == nullptr)
	{
		return MakeResult(Request, EPRDebugCommandResultCode::InvalidParameter, LOCTEXT("DamageParameter", "Damage amount is invalid."));
	}

	FPRDamageRequest DamageRequest;
	DamageRequest.SourceId = TEXT("ProjectR.Debug.DamageSelf");
	DamageRequest.DamageSource = CombatSubsystem;
	DamageRequest.Instigator = Character;
	DamageRequest.Target = Character;
	DamageRequest.RawDamage = static_cast<float>(Amount->FloatValue);
	const EPRDebugCommandResultCode Code = MapDamageStatus(CombatSubsystem->ApplyDamage(DamageRequest));
	return MakeResult(
		Request,
		Code,
		Code == EPRDebugCommandResultCode::Success
			? LOCTEXT("DamageSuccess", "Damage request applied.")
			: LOCTEXT("DamageRejected", "Damage request was rejected."));
}

FPRDebugCommandResult FPRDebugBuiltInCommands::ReviveSelf(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	APRPlayerCharacter* Character = ResolvePlayerCharacter(GameInstance);
	UWorld* World = GameInstance != nullptr ? GameInstance->GetWorld() : nullptr;
	UPRCombatSubsystem* CombatSubsystem = World != nullptr ? World->GetSubsystem<UPRCombatSubsystem>() : nullptr;
	APRPlayerState* PlayerState = Character != nullptr ? Character->GetPlayerState<APRPlayerState>() : nullptr;
	if (Character == nullptr || PlayerState == nullptr
		|| PlayerState->GetProjectRAbilitySystemComponent() == nullptr
		|| CombatSubsystem == nullptr)
	{
		return MakeResult(Request, EPRDebugCommandResultCode::InvalidContext, LOCTEXT("ReviveInvalid", "Player combat context is not available."));
	}

	FPRReviveRequest ReviveRequest;
	ReviveRequest.SourceId = TEXT("ProjectR.Debug.ReviveSelf");
	ReviveRequest.DamageSource = CombatSubsystem;
	ReviveRequest.Instigator = Character;
	ReviveRequest.Target = Character;
	ReviveRequest.HealthFraction = 1.0f;
	ReviveRequest.ShieldFraction = 1.0f;
	const EPRDebugCommandResultCode Code = MapDamageStatus(CombatSubsystem->Revive(ReviveRequest));
	return MakeResult(
		Request,
		Code,
		Code == EPRDebugCommandResultCode::Success
			? LOCTEXT("ReviveSuccess", "Revive request applied.")
			: LOCTEXT("ReviveRejected", "Revive request was rejected."));
}

FPRDebugCommandResult FPRDebugBuiltInCommands::ReadSaveState(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	if (GameInstance == nullptr || GameInstance->GetSubsystem<UPRSaveSubsystem>() == nullptr)
	{
		return MakeResult(Request, EPRDebugCommandResultCode::InvalidContext, LOCTEXT("SaveInvalid", "Save runtime state is not available."));
	}

	FPRDebugCommandResult Result = BuildStatus(Request, GameInstance);
	Result.CommandId = EPRDebugCommandId::SaveRuntimeState;
	Result.PlayerMessage = LOCTEXT("SaveSuccess", "Save runtime state refreshed.");
	return Result;
}

FPRDebugCommandResult FPRDebugBuiltInCommands::TravelToCombatGym(
	const FPRDebugCommandRequest& Request,
	UGameInstance* GameInstance)
{
	UPRGameInstance* ProjectRGameInstance = Cast<UPRGameInstance>(GameInstance);
	if (ProjectRGameInstance == nullptr || GameInstance->GetFirstLocalPlayerController() == nullptr)
	{
		return MakeResult(Request, EPRDebugCommandResultCode::InvalidContext, LOCTEXT("TravelInvalid", "Travel context is not available."));
	}
	return MakeResult(
		Request,
		ProjectRGameInstance->OpenMap(EPRMapId::CombatGym)
			? EPRDebugCommandResultCode::Success
			: EPRDebugCommandResultCode::Failed,
		LOCTEXT("TravelRequested", "Combat Gym travel requested."));
}

#undef LOCTEXT_NAMESPACE
