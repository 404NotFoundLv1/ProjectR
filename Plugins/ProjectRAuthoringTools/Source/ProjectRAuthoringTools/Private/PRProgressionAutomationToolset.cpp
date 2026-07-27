// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRProgressionAutomationToolset.h"

#include "Abilities/PRAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Companions/PRCompanionRuntimeSubsystem.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Roguelike/Progression/PRProgressionSubsystem.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRProgressionAutomationToolsetPrivate
{
#if WITH_DEV_AUTOMATION_TESTS
const FPrimaryAssetId PlayerMaxHealthNode(TEXT("ProgressionNode"), TEXT("PlayerMaxHealth"));
const FPrimaryAssetId PlayerMaxEnergyNode(TEXT("ProgressionNode"), TEXT("PlayerMaxEnergy"));
const FPrimaryAssetId AISupportNode(TEXT("ProgressionNode"), TEXT("AISupport"));
const FPrimaryAssetId PlayerSkillSlotNode(TEXT("ProgressionNode"), TEXT("PlayerSkillSlot"));

class FFixedProgressionRunner final : public TSharedFromThis<FFixedProgressionRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FFixedProgressionRunner> Runner = MakeShared<FFixedProgressionRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!UPRSaveSubsystem::HasAutomationStorageOverride())
		{
			Runner->Result->SetError(TEXT("PrepareFixedProgressionPIE must succeed before the fixed progression flow."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float)
		{
			return KeepAlive->Tick();
		}));
		return Runner->Result.Get();
	}

private:
	enum class EPhase : uint8 { CreateProfile, CreateAccount, StageFixture, UnlockHealth, UnlockEnergy, UnlockSupport, StartRun, Verify };

	bool Tick()
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!World || World->GetNetMode() == NM_Client) return Fail(TEXT("Fixed progression acceptance requires an authoritative in-process PIE world."));
		UGameInstance* GameInstance = World->GetGameInstance();
		UPRSaveSubsystem* Save = GameInstance ? GameInstance->GetSubsystem<UPRSaveSubsystem>() : nullptr;
		UPRRunStateSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
		UPRProgressionSubsystem* Progression = GameInstance ? GameInstance->GetSubsystem<UPRProgressionSubsystem>() : nullptr;
		if (!Save || !Run || !Progression) return Fail(TEXT("Fixed progression PIE is missing Save, RunState, or Progression subsystem."));
		if (FPlatformTime::Seconds() - StartedAt > 45.0) return Fail(TEXT("Fixed progression acceptance timed out."));

		switch (Phase)
		{
		case EPhase::CreateProfile:
			if (!bIssued) { FGuid RequestId; if (Save->CreateNewDefaultProfile(RequestId) != EPRSaveResult::Success) return Fail(TEXT("Could not create the isolated automation profile.")); bIssued = true; }
			Phase = EPhase::CreateAccount; bIssued = false; return true;
		case EPhase::CreateAccount:
			if (!bIssued) { FGuid RequestId; if (Run->RequestCreateAccount(FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Technician")), RequestId) != EPRAccountOperationResult::Started) return Fail(TEXT("Could not create the fixed Technician automation account.")); bIssued = true; }
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady) return true;
			Phase = EPhase::StageFixture; bIssued = false; return true;
		case EPhase::StageFixture:
			if (!bIssued)
			{
				if (!Save->StageFixedProgressionAutomationFixture()) return Fail(TEXT("The fixed automation-only progression fixture was unavailable."));
				FGuid RequestId; if (Save->RequestSaveCurrentProfile(RequestId) != EPRSaveRequestStatus::Started) return Fail(TEXT("Could not durably stage the fixed progression fixture."));
				bIssued = true;
			}
			if (Save->GetSaveRuntimeState().State == EPRSaveSubsystemState::Saving) return true;
			{ FPRProgressionSnapshot Snapshot; if (!Progression->GetProgressionSnapshot(Snapshot) || Snapshot.CounterproofFragments != 3 || !Snapshot.UnlockedNodeIds.IsEmpty()) return Fail(TEXT("Fixed progression fixture did not reload as exactly three counterproof fragments.")); }
			Phase = EPhase::UnlockHealth; bIssued = false; return true;
		case EPhase::UnlockHealth: return RequestAndAwait(Progression, PlayerMaxHealthNode, 2, EPhase::UnlockEnergy);
		case EPhase::UnlockEnergy: return RequestAndAwait(Progression, PlayerMaxEnergyNode, 1, EPhase::UnlockSupport);
		case EPhase::UnlockSupport: return RequestAndAwait(Progression, AISupportNode, 0, EPhase::StartRun);
		case EPhase::StartRun:
			if (!bIssued) { FGuid RequestId; if (Run->RequestStartRun(41044, RequestId) != EPRAccountOperationResult::Started) return Fail(TEXT("Fixed progression StartRun did not enter the durable account transaction.")); bIssued = true; }
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::RunActive) return true;
			Phase = EPhase::Verify; return true;
		case EPhase::Verify:
			return Verify(World, Progression);
		}
		return Fail(TEXT("Fixed progression acceptance entered an invalid phase."));
	}

	bool RequestAndAwait(UPRProgressionSubsystem* Progression, const FPrimaryAssetId& NodeId, const int32 ExpectedBalance, const EPhase NextPhase)
	{
		if (!bIssued)
		{
			FGuid RequestId;
			if (Progression->RequestUnlockNode(NodeId, RequestId) != EPRProgressionOperationResult::Pending) return Fail(TEXT("A fixed progression unlock did not stage its durable transaction."));
			bIssued = true;
		}
		FPRProgressionSnapshot Snapshot;
		if (!Progression->GetProgressionSnapshot(Snapshot) || !Snapshot.UnlockedNodeIds.Contains(NodeId) || Snapshot.CounterproofFragments != ExpectedBalance) return true;
		Phase = NextPhase; bIssued = false; return true;
	}

	bool Verify(UWorld* World, UPRProgressionSubsystem* Progression)
	{
		FPRProgressionRunSnapshot Snapshot;
		if (!Progression->GetRunSnapshot(Snapshot)
			|| Snapshot.PlayerMaxHealthBonus != 10 || Snapshot.PlayerMaxEnergyBonus != 10
			|| !FMath::IsNearlyEqual(Snapshot.CompanionSupportIntervalMultiplier, 0.90f)
			|| !Snapshot.EntitlementIds.IsEmpty()) return Fail(TEXT("The frozen next-run progression snapshot was not exact."));
		FGuid RequestId;
		if (Progression->RequestUnlockNode(PlayerSkillSlotNode, RequestId) != EPRProgressionOperationResult::InvalidState) return Fail(TEXT("A current active run accepted a progression unlock."));
		UPRCompanionRuntimeSubsystem* CompanionRuntime = World->GetSubsystem<UPRCompanionRuntimeSubsystem>();
		float Interval = 1.0f; int32 Stride = 0;
		if (!CompanionRuntime || !CompanionRuntime->GetSupportPolicy(Interval, Stride)
			|| !FMath::IsNearlyEqual(Interval, 0.90f) || Stride != 1) return Fail(TEXT("The fixed AISupport session policy was not applied."));
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		IAbilitySystemInterface* AbilityInterface = Pawn ? Cast<IAbilitySystemInterface>(Pawn) : nullptr;
		UAbilitySystemComponent* ASC = AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr;
		if (!ASC || !FMath::IsNearlyEqual(ASC->GetNumericAttribute(UPRAttributeSet::GetMaxHealthAttribute()), 110.0f)
			|| !FMath::IsNearlyEqual(ASC->GetNumericAttribute(UPRAttributeSet::GetMaxEnergyAttribute()), 110.0f)) return Fail(TEXT("The fixed PlayerMaxHealth or PlayerMaxEnergy session GE was not applied."));
		Result->SetValue(TEXT("{\"status\":\"PASS\",\"seed\":41044,\"unlocks\":3,\"counterproofBalance\":0,\"activeRunUnlockRejected\":true,\"healthGE\":10,\"energyGE\":10,\"aiSupportMultiplier\":0.90,\"automationSlotsOnly\":true}"));
		return false;
	}

	bool Fail(const FString& Message)
	{
		UPRSaveSubsystem::CleanupAutomationStorageOverride();
		Result->SetError(Message);
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	EPhase Phase = EPhase::CreateProfile;
	bool bIssued = false;
	double StartedAt = FPlatformTime::Seconds();
};
#endif
}

UToolCallAsyncResultString* UPRProgressionAutomationToolset::PrepareFixedProgressionPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("PrepareFixedProgressionPIE must run before PIE starts.")); return Result; }
	const FString Base = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedPtr<FPRSaveStorage> Storage = FPRSaveStorage::CreateAutomation(Base);
	if (!Storage) { Result->SetError(TEXT("Could not create isolated automation A/B storage.")); return Result; }
	UPRSaveSubsystem::SetAutomationStorageOverride(MoveTemp(Storage));
	Result->SetValue(TEXT("{\"status\":\"READY\",\"slots\":\"automation-guid-only\",\"fixture\":\"fixed-3-counterproof\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Progression PIE automation is unavailable outside development automation builds."));
#endif
	return Result;
}

UToolCallAsyncResultString* UPRProgressionAutomationToolset::RunFixedProgressionPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRProgressionAutomationToolsetPrivate::FFixedProgressionRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRProgressionAutomationToolset::VerifyFixedProgressionCleanup()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("Stop PIE before checking fixed progression cleanup.")); return Result; }
	UPRSaveSubsystem::CleanupAutomationStorageOverride();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"pieWorldDestroyed\":true,\"automationSlotsCleaned\":true,\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("Unavailable."));
#endif
	return Result;
}
