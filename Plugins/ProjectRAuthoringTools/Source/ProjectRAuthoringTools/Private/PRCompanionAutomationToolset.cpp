// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRCompanionAutomationToolset.h"

#include "Companions/PRCompanionSubsystem.h"
#include "Core/PRRelationshipTypes.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"

namespace PRCompanionAutomationToolset
{
class FPIECompanionSyncRunner final : public TSharedFromThis<FPIECompanionSyncRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FPIECompanionSyncRunner> Runner = MakeShared<FPIECompanionSyncRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}

		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](const float DeltaSeconds) mutable
			{
				const bool bContinue = RunnerPtr->Tick(DeltaSeconds);
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

private:
	bool Initialize()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			return Fail(TEXT("RunPIECompanionSyncSmoke requires an active authoritative in-process PIE world."));
		}

		ElapsedSeconds = 0.0f;
		return true;
	}

	bool Tick(const float DeltaSeconds)
	{
		ElapsedSeconds += DeltaSeconds;
		if (ElapsedSeconds > 20.0f)
		{
			Fail(bTravelStarted
				? TEXT("Fixed Companion PIE travel did not reach BossGym within 20 seconds.")
				: TEXT("The fixed Companion registry did not become ready within 20 seconds of PIE startup."));
			return false;
		}

		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld))
		{
			return true;
		}

		UGameInstance* GameInstance = PlayWorld->GetGameInstance();
		UPRCompanionSubsystem* CompanionSubsystem = GameInstance ? GameInstance->GetSubsystem<UPRCompanionSubsystem>() : nullptr;
		if (!bTravelStarted)
		{
			if (!CompanionSubsystem || !CompanionSubsystem->IsRegistryReady())
			{
				return true;
			}
			if (CompanionSubsystem->SelectPrimaryCompanion(FPRCompanionContract::AxiomTag()) != EPRPrimaryCompanionSelectionResult::Selected
				|| !HasExpectedAxiomSync(*CompanionSubsystem))
			{
				Fail(TEXT("The fixed Axiom primary selection did not derive the canonical two backgrounds."));
				return false;
			}
			bTravelStarted = true;
			UGameplayStatics::OpenLevel(PlayWorld, FName(TEXT("/Game/ProjectR/Maps/L_BossGym")));
			return true;
		}

		if (!PlayWorld->GetMapName().Contains(TEXT("L_BossGym")))
		{
			return true;
		}

		if (!CompanionSubsystem || !CompanionSubsystem->IsRegistryReady() || !HasExpectedAxiomSync(*CompanionSubsystem))
		{
			Fail(TEXT("Companion run-local selection did not persist through fixed BossGym travel."));
			return false;
		}

		TArray<FPRCompanionRelationshipRecord> Records;
		CompanionSubsystem->GetAllRelationshipSnapshots(Records);
		if (!FPRCompanionContract::AreCanonicalRelationshipRecords(Records))
		{
			Fail(TEXT("Companion relationship snapshots are not canonical after fixed PIE travel."));
			return false;
		}

		Result->SetValue(TEXT("{\"status\":\"PASS\",\"primary\":\"Companion.Axiom\",\"backgroundCount\":2,\"travel\":\"L_BossGym\",\"saveIO\":false,\"packagesSaved\":false}"));
		return false;
	}

	bool HasExpectedAxiomSync(const UPRCompanionSubsystem& CompanionSubsystem) const
	{
		const FPRCompanionSyncState State = CompanionSubsystem.GetSyncState();
		return State.PrimaryCompanionId.MatchesTagExact(FPRCompanionContract::AxiomTag())
			&& State.BackgroundCompanionIds.Num() == 2
			&& State.BackgroundCompanionIds[0].MatchesTagExact(FPRCompanionContract::KindleTag())
			&& State.BackgroundCompanionIds[1].MatchesTagExact(FPRCompanionContract::NullTag());
	}

	bool Fail(const FString& Message)
	{
		Result->SetError(Message);
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	float ElapsedSeconds = 0.0f;
	bool bTravelStarted = false;
};
} // namespace PRCompanionAutomationToolset

UToolCallAsyncResultString* UPRCompanionAutomationToolset::RunPIECompanionSyncSmoke()
{
	return PRCompanionAutomationToolset::FPIECompanionSyncRunner::Start();
}
