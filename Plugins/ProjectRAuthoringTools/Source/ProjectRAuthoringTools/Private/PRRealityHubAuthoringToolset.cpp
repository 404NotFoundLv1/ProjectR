// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRRealityHubAuthoringToolset.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "WidgetBlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Memory/PRMemoryPersonaDataAsset.h"
#include "Memory/PRMemoryRegistryDataAsset.h"
#include "Memory/PRMemorySubsystem.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"
#include "Save/PRSaveStorage.h"
#include "Save/PRSaveSubsystem.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRRealityHubHUD.h"
#include "UI/PRRealityHubTrainingReturnWidget.h"
#include "UI/PRRealityHubWidget.h"
#include "UI/PRMemoryHubWidget.h"
#include "UI/PRMemorySummaryWidget.h"
#include "WidgetBlueprint.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Containers/Ticker.h"

namespace PRRealityHubAuthoring
{
struct FRoot { explicit FRoot(UToolCallAsyncResultString* In) : Value(In) { Value->AddToRoot(); } ~FRoot() { Value->RemoveFromRoot(); } UToolCallAsyncResultString* Value; };

UWidgetBlueprint* LoadWidget(const TCHAR* Path)
{
	return LoadObject<UWidgetBlueprint>(nullptr, Path);
}

void EnsureWidgetVariableGuids(UWidgetBlueprint* Blueprint)
{
#if WITH_EDITORONLY_DATA
	if (!Blueprint) return;
	Blueprint->ForEachSourceWidget([Blueprint](UWidget* Widget)
	{
		if (Widget && !Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			Blueprint->OnVariableAdded(Widget->GetFName());
		}
	});
#endif
}

FText GetFixedButtonText(const FString& Name)
{
	static const TMap<FString, FString> Labels = {
		{ TEXT("Button_CassetteSlot"), TEXT("Cassette Slot") },
		{ TEXT("Button_CreateProfile"), TEXT("Create Profile") },
		{ TEXT("Button_IdentityTechnician"), TEXT("Technician") },
		{ TEXT("Button_IdentitySecurity"), TEXT("Security") },
		{ TEXT("Button_IdentityExile"), TEXT("Exile") },
		{ TEXT("Button_IdentityObserver"), TEXT("Observer") },
		{ TEXT("Button_IdentityBlank"), TEXT("Blank") },
		{ TEXT("Button_EnterNetwork"), TEXT("Enter Network") },
		{ TEXT("Button_Companion"), TEXT("Companion Terminal") },
		{ TEXT("Button_Graveyard"), TEXT("Account Graveyard") },
		{ TEXT("Button_TrainingSimulator"), TEXT("Training Simulator") },
		{ TEXT("Button_DirectorForecaster"), TEXT("Director Forecaster") },
		{ TEXT("Button_Progression"), TEXT("Progression") },
		{ TEXT("Button_ReturnToRealityHub"), TEXT("Return to Reality Hub") }
	};
	if (const FString* Label = Labels.Find(Name)) return FText::FromString(*Label);
	return FText::FromString(Name.Replace(TEXT("Button_"), TEXT("")).Replace(TEXT("_"), TEXT(" ")));
}

bool ConfigureWidget(UWidgetBlueprint* Blueprint, const FString& Heading, const TArray<FString>& Buttons)
{
	if (!Blueprint || !Blueprint->WidgetTree) return false;
	Blueprint->Modify();
	UVerticalBox* Root = Cast<UVerticalBox>(Blueprint->WidgetTree->RootWidget);
	if (!Root)
	{
		Root = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		Blueprint->WidgetTree->RootWidget = Root;
	}
	if (!Blueprint->WidgetTree->FindWidget(TEXT("Heading")))
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Heading"));
		Text->SetText(FText::FromString(Heading));
		Root->AddChildToVerticalBox(Text);
	}
	if (Heading == TEXT("Reality Hub") && !Blueprint->WidgetTree->FindWidget(TEXT("Text_Status")))
	{
		UTextBlock* Status = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Status"));
		Status->SetText(FText::FromString(TEXT("Loading Reality Hub state...")));
		Root->AddChildToVerticalBox(Status);
	}
	for (const FString& Name : Buttons)
	{
		UButton* Button = Cast<UButton>(Blueprint->WidgetTree->FindWidget(FName(*Name)));
		if (!Button)
		{
			Button = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*Name));
			Root->AddChildToVerticalBox(Button);
		}
		UTextBlock* Text = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(FName(*(Name + TEXT("_Text")))));
		if (!Text)
		{
			Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Name + TEXT("_Text"))));
			Button->AddChild(Text);
		}
		Text->SetText(GetFixedButtonText(Name));
	}
	Blueprint->MarkPackageDirty();
	return true;
}

#if WITH_DEV_AUTOMATION_TESTS
class FFixedMemorySingleDeathRunner final : public TSharedFromThis<FFixedMemorySingleDeathRunner>
{
public:
	static UToolCallAsyncResultString* Start(const bool bInUseMixedTermination = false)
	{
		TSharedRef<FFixedMemorySingleDeathRunner> Runner = MakeShared<FFixedMemorySingleDeathRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->bUseMixedTermination = bInUseMixedTermination;
		Runner->Phase = EPhase::LoadProfile;
		if (!UPRSaveSubsystem::HasAutomationStorageOverride())
		{
			Runner->Result->SetError(TEXT("PrepareV052MemoryPIE must succeed before the fixed Memory PIE flow."));
			return Runner->Result.Get();
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([KeepAlive = Runner.ToSharedPtr()](float) { return KeepAlive->Tick(); }));
		return Runner->Result.Get();
	}

private:
	enum class EPhase : uint8 { LoadProfile, CreateProfile, WaitProfile, CreateAccount, WaitAccount, StartRun, WaitRun, Finalize, RetryPending, WaitRetry, WaitMemory, Verify };

	bool Tick()
	{
		if (FPlatformTime::Seconds() - StartedAt > 55.0) return Fail(TEXT("Fixed v0.5.2 Memory PIE timed out."));
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		if (!World || World->GetNetMode() == NM_Client) return Fail(TEXT("Fixed v0.5.2 Memory PIE requires an active authoritative in-process PIE world."));
		UGameInstance* GameInstance = World->GetGameInstance();
		UPRSaveSubsystem* Save = GameInstance ? GameInstance->GetSubsystem<UPRSaveSubsystem>() : nullptr;
		UPRRunStateSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<UPRRunStateSubsystem>() : nullptr;
		UPRMemorySubsystem* Memory = GameInstance ? GameInstance->GetSubsystem<UPRMemorySubsystem>() : nullptr;
		if (!Save || !Run || !Memory) return Fail(TEXT("Fixed v0.5.2 Memory PIE is missing Save, RunState, or Memory subsystem."));

		FPRProfileSaveData Profile;
		if (FPlatformTime::Seconds() - LastDiagnosticsAt >= 1.0)
		{
			Save->GetLoadedProfileSnapshot(Profile);
			const FPRSaveRuntimeState SaveState = Save->GetSaveRuntimeState();
			UE_LOG(LogTemp, Log, TEXT("Memory PIE phase=%d runState=%d saveState=%d saveResult=%d rev=%lld summaries=%d memoryFragments=%d."),
				static_cast<int32>(Phase), static_cast<int32>(Run->GetRunRuntimeState().State), static_cast<int32>(SaveState.State), static_cast<int32>(SaveState.LastResult), SaveState.SaveRevision, Profile.MemoryPersistence.Summaries.Num(), Profile.ProgressionPersistence.MemoryFragments);
			LastDiagnosticsAt = FPlatformTime::Seconds();
		}
		switch (Phase)
		{
		case EPhase::LoadProfile:
		{
			const EPRSaveResult LoadResult = Save->LoadDefaultProfile(RequestId);
			if (LoadResult == EPRSaveResult::Success || LoadResult == EPRSaveResult::RecoveredFromAlternate)
			{
				if (!Save->GetLoadedProfileSnapshot(Profile)) return Fail(TEXT("Fixed Memory PIE could not read its isolated profile."));
				CompletedDeaths = Profile.MemoryPersistence.Summaries.Num();
				Advance(EPhase::WaitProfile); return true;
			}
			if (LoadResult == EPRSaveResult::NotFound) { Advance(EPhase::CreateProfile); return true; }
			return Fail(TEXT("Fixed Memory PIE could not load its isolated profile."));
		}
		case EPhase::CreateProfile:
			if (Save->CreateNewDefaultProfile(RequestId) != EPRSaveResult::Success) return Fail(TEXT("Could not create the isolated Memory automation profile."));
			Advance(EPhase::WaitProfile); return true;
		case EPhase::WaitProfile:
			if (!Save->GetLoadedProfileSnapshot(Profile) || !IsSaveReadyStable(*Save)) return true;
			Advance(EPhase::CreateAccount); return true;
		case EPhase::CreateAccount:
			if (Run->RequestCreateAccount(FPrimaryAssetId(TEXT("ProjectRAccountIdentity"), TEXT("Technician")), RequestId) != EPRAccountOperationResult::Started) return Fail(TEXT("Fixed Memory PIE account creation did not start."));
			Advance(EPhase::WaitAccount); return true;
		case EPhase::WaitAccount:
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::AccountReady || !IsSaveReadyStable(*Save)) return true;
			Advance(EPhase::StartRun); return true;
		case EPhase::StartRun:
			if (!IsSaveReadyStable(*Save)) return true;
			if (Run->RequestStartRun(52000 + CompletedDeaths, RequestId) != EPRAccountOperationResult::Started)
			{
				if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed) { Advance(EPhase::RetryPending); return true; }
				return Fail(TEXT("Fixed Memory PIE run start did not persist."));
			}
			Advance(EPhase::WaitRun); return true;
		case EPhase::WaitRun:
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed) { Advance(EPhase::RetryPending); return true; }
			if (Run->GetRunRuntimeState().State != EPRRunLifecycleState::RunActive || !IsSaveReadyStable(*Save)) return true;
			Advance(EPhase::Finalize); return true;
		case EPhase::Finalize:
			if (!IsSaveReadyStable(*Save) || FPlatformTime::Seconds() - PhaseEnteredAt < 1.0) return true;
			if (!Run->FinalizeActiveAccountForAutomation(GetFixedTerminationReason(CompletedDeaths)))
			{
				if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed) { Advance(EPhase::RetryPending); return true; }
				return Fail(TEXT("Fixed Memory PIE death finalization did not start."));
			}
			Advance(EPhase::WaitMemory); return true;
		case EPhase::RetryPending:
			if (!IsSaveReadyStable(*Save)) return true;
			Run->RetryPendingPersistence(RequestId);
			RetryObservedAt = FPlatformTime::Seconds();
			Advance(EPhase::WaitRetry); return true;
		case EPhase::WaitRetry:
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::PersistenceFailed)
			{
				if (FPlatformTime::Seconds() - RetryObservedAt >= 2.0) Advance(EPhase::RetryPending);
				return true;
			}
			if (!IsSaveReadyStable(*Save)) return true;
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::RunActive) { Advance(EPhase::Finalize); return true; }
			if (Run->GetRunRuntimeState().State == EPRRunLifecycleState::Idle) { Advance(EPhase::WaitMemory); return true; }
			return true;
		case EPhase::WaitMemory:
		{
			if (!Save->GetLoadedProfileSnapshot(Profile) || !IsSaveReadyStable(*Save) || Run->GetRunRuntimeState().State != EPRRunLifecycleState::Idle || Profile.MemoryPersistence.Summaries.Num() != CompletedDeaths + 1)
			{
				FPRMemorySnapshot MemorySnapshot;
				if (Memory->GetSnapshot(MemorySnapshot) && MemorySnapshot.State == EPRMemoryState::ReadyToRetry) return Fail(TEXT("Memory persistence entered ReadyToRetry during fixed PIE."));
				return true;
			}
			const int32 ExpectedFragments = GetExpectedMemoryFragments(CompletedDeaths + 1);
			if (Profile.ProgressionPersistence.MemoryFragments != ExpectedFragments) return Fail(TEXT("Memory fragment award was not atomically published with its summary."));
			++CompletedDeaths;
			Advance(EPhase::Verify);
			return true;
		}
		case EPhase::Verify:
			const int32 ExpectedCount = CompletedDeaths;
			if (!Save->GetLoadedProfileSnapshot(Profile) || !FPRMemoryPersistenceContract::IsCanonical(Profile.MemoryPersistence)
				|| Profile.MemoryPersistence.Summaries.Num() != ExpectedCount || Profile.MemoryPersistence.LifetimeSummaryCount < ExpectedCount
				|| Profile.MemoryPersistence.LifetimeMemoryFragmentsAwarded < GetExpectedMemoryFragments(ExpectedCount)
				|| Profile.ProgressionPersistence.MemoryFragments != GetExpectedMemoryFragments(ExpectedCount))
			{
				return Fail(TEXT("Fixed Memory PIE did not retain five bounded atomic summaries and awards."));
			}
			FPRMemorySnapshot Snapshot;
			if (!Memory->GetSnapshot(Snapshot) || !Snapshot.bHasLatestSummary || Snapshot.LatestSummary.GraveyardOrdinal != ExpectedCount) return Fail(TEXT("Memory snapshot did not publish the final persisted summary."));
			Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"runs\":%d,\"summaries\":%d,\"memoryFragments\":%d,\"reason\":\"%s\",\"automationSlotsOnly\":true,\"runtimeClean\":true}"), ExpectedCount, ExpectedCount, GetExpectedMemoryFragments(ExpectedCount), *StaticEnum<EPRAccountTerminationReason>()->GetNameStringByValue(static_cast<int64>(GetFixedTerminationReason(ExpectedCount - 1)))));
			return false;
		}
		return Fail(TEXT("Fixed Memory PIE entered an invalid phase."));
	}

	bool IsSaveReadyStable(const UPRSaveSubsystem& Save)
	{
		if (Save.GetSaveRuntimeState().State != EPRSaveSubsystemState::Ready)
		{
			ReadySince = 0.0;
			return false;
		}
		const double Now = FPlatformTime::Seconds();
		if (ReadySince <= 0.0)
		{
			ReadySince = Now;
			return false;
		}
		return Now - ReadySince >= 0.20;
	}

	EPRAccountTerminationReason GetFixedTerminationReason(const int32 ExistingSummaryCount) const
	{
		if (!bUseMixedTermination) return EPRAccountTerminationReason::PlayerDeath;
		static constexpr EPRAccountTerminationReason Sequence[] = {
			EPRAccountTerminationReason::PlayerDeath,
			EPRAccountTerminationReason::DivergenceEvacuation,
			EPRAccountTerminationReason::RoomSequenceCompleted,
			EPRAccountTerminationReason::InterruptedRecovery,
			EPRAccountTerminationReason::PlayerDeath,
			EPRAccountTerminationReason::DivergenceEvacuation,
			EPRAccountTerminationReason::RoomSequenceCompleted,
			EPRAccountTerminationReason::InterruptedRecovery,
			EPRAccountTerminationReason::PlayerDeath,
			EPRAccountTerminationReason::RoomSequenceCompleted };
		return Sequence[FMath::Clamp(ExistingSummaryCount, 0, UE_ARRAY_COUNT(Sequence) - 1)];
	}

	int32 GetExpectedMemoryFragments(const int32 CompletedSummaryCount) const
	{
		if (!bUseMixedTermination) return CompletedSummaryCount;
		int32 AwardCount = 0;
		for (int32 Index = 0; Index < CompletedSummaryCount; ++Index)
		{
			if (GetFixedTerminationReason(Index) != EPRAccountTerminationReason::InterruptedRecovery) ++AwardCount;
		}
		return AwardCount;
	}

	void Advance(const EPhase Next)
	{
		Phase = Next;
		ReadySince = 0.0;
		PhaseEnteredAt = FPlatformTime::Seconds();
	}

	bool Fail(const FString& Message)
	{
		UPRSaveSubsystem::CleanupAutomationStorageOverride();
		Result->SetError(Message);
		return false;
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	EPhase Phase = EPhase::CreateProfile;
	bool bUseMixedTermination = false;
	FGuid RequestId;
	int32 CompletedDeaths = 0;
	double StartedAt = FPlatformTime::Seconds();
	double LastDiagnosticsAt = 0.0;
	double ReadySince = 0.0;
	double PhaseEnteredAt = StartedAt;
	double RetryObservedAt = 0.0;
};
#endif
}

UToolCallAsyncResultString* UPRRealityHubAuthoringToolset::ConfigureFixedRealityHubWidgetManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); PRRealityHubAuthoring::FRoot Root(Result);
	using namespace PRRealityHubAuthoring;
	UWidgetBlueprint* HubRoot = LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubRoot.WBP_RealityHubRoot"));
	UWidgetBlueprint* Cassette = LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubCassetteSlot.WBP_RealityHubCassetteSlot"));
	UWidgetBlueprint* Companion = LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubCompanionTerminal.WBP_RealityHubCompanionTerminal"));
	UWidgetBlueprint* Graveyard = LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubGraveyard.WBP_RealityHubGraveyard"));
	UWidgetBlueprint* Training = LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubTrainingSimulator.WBP_RealityHubTrainingSimulator"));
	UWidgetBlueprint* Forecaster = LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubDirectorForecaster.WBP_RealityHubDirectorForecaster"));
	UWidgetBlueprint* Progression = LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubProgressionPanel.WBP_RealityHubProgressionPanel"));
	UWidgetBlueprint* Return = LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubTrainingReturn.WBP_RealityHubTrainingReturn"));
	if (!HubRoot || !Cassette || !Companion || !Graveyard || !Training || !Forecaster || !Progression || !Return)
	{
		Result->SetError(TEXT("The fixed Reality Hub widget manifest is incomplete; no package was created by this tool."));
		return Result;
	}
	const bool bConfigured = ConfigureWidget(HubRoot, TEXT("Reality Hub"), { TEXT("Button_CassetteSlot"), TEXT("Button_CreateProfile"), TEXT("Button_IdentityTechnician"), TEXT("Button_IdentitySecurity"), TEXT("Button_IdentityExile"), TEXT("Button_IdentityObserver"), TEXT("Button_IdentityBlank"), TEXT("Button_EnterNetwork"), TEXT("Button_Companion"), TEXT("Button_Graveyard"), TEXT("Button_TrainingSimulator"), TEXT("Button_DirectorForecaster"), TEXT("Button_Progression") })
		&& ConfigureWidget(Cassette, TEXT("Cassette Slot"), {})
		&& ConfigureWidget(Companion, TEXT("Companion Terminal — future provider unavailable"), {})
		&& ConfigureWidget(Graveyard, TEXT("Account Graveyard — read only"), {})
		&& ConfigureWidget(Training, TEXT("Training Simulator"), { TEXT("Button_TrainingSimulator") })
		&& ConfigureWidget(Forecaster, TEXT("Director Forecaster — local deterministic preview"), {})
		&& ConfigureWidget(Progression, TEXT("Progression — next-run effects only"), {})
		&& ConfigureWidget(Return, TEXT("Training Return"), { TEXT("Button_ReturnToRealityHub") });
	if (!bConfigured) { Result->SetError(TEXT("Reality Hub widget configuration failed.")); return Result; }
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"configured\":8,\"saved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRRealityHubAuthoringToolset::ConfigureV051CompanionQuestWidget()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); PRRealityHubAuthoring::FRoot Root(Result);
	UWidgetBlueprint* Companion = PRRealityHubAuthoring::LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubCompanionTerminal.WBP_RealityHubCompanionTerminal"));
	UWidgetBlueprint* HubRoot = PRRealityHubAuthoring::LoadWidget(TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubRoot.WBP_RealityHubRoot"));
	if (!Companion || !Companion->WidgetTree || !HubRoot || !HubRoot->WidgetTree) { Result->SetError(TEXT("v0.5.1 Companion Terminal or Hub Root is unavailable; no package was changed.")); return Result; }
	Companion->Modify(); UVerticalBox* Panel = Cast<UVerticalBox>(Companion->WidgetTree->RootWidget);
	if (!Panel) { Result->SetError(TEXT("v0.5.1 Companion Terminal root is not a VerticalBox; no package was changed.")); return Result; }
	auto AddText = [Companion, Panel](const TCHAR* Name, const TCHAR* Label)
	{
		if (UTextBlock* Existing = Cast<UTextBlock>(Companion->WidgetTree->FindWidget(FName(Name)))) { Existing->SetText(FText::FromString(Label)); return; }
		UTextBlock* Text = Companion->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name)); Text->SetText(FText::FromString(Label)); Panel->AddChildToVerticalBox(Text);
	};
	auto AddButton = [Companion, Panel](const TCHAR* Name, const TCHAR* Label)
	{
		UButton* Button = Cast<UButton>(Companion->WidgetTree->FindWidget(FName(Name)));
		if (!Button) { Button = Companion->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name)); Panel->AddChildToVerticalBox(Button); }
		const FString TextName = FString(Name) + TEXT("_Text"); UTextBlock* Text = Cast<UTextBlock>(Companion->WidgetTree->FindWidget(FName(*TextName)));
		if (!Text) { Text = Companion->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*TextName)); Button->AddChild(Text); }
		Text->SetText(FText::FromString(Label));
	};
	AddText(TEXT("QuestStatus"), TEXT("Companion quests: select a fixed available objective."));
	AddText(TEXT("GraveyardProjection"), TEXT("Read-only graveyard projection is unavailable until five unique records exist."));
	AddButton(TEXT("Button_Quest_Axiom_LowProbabilitySample"), TEXT("Axiom — Low Probability Sample"));
	AddButton(TEXT("Button_Quest_Axiom_ImperfectOptimum"), TEXT("Axiom — Imperfect Optimum"));
	AddButton(TEXT("Button_Quest_Kindle_NoRetreatLine"), TEXT("Kindle — No Retreat Line"));
	AddButton(TEXT("Button_Quest_Kindle_LearnToRetreat"), TEXT("Kindle — Learn To Retreat"));
	AddButton(TEXT("Button_Quest_Null_GarbageCollection"), TEXT("Null — Garbage Collection"));
	AddButton(TEXT("Button_Quest_Null_RememberMe"), TEXT("Null — Remember Me"));
	AddButton(TEXT("Button_Quest_RetrySave"), TEXT("Retry quest save"));
	AddButton(TEXT("Button_ConfirmRememberMe"), TEXT("Confirm five graveyard records viewed"));
	HubRoot->Modify(); UVerticalBox* RootPanel = Cast<UVerticalBox>(HubRoot->WidgetTree->RootWidget);
	if (!RootPanel || !Companion->GeneratedClass || !Companion->GeneratedClass->IsChildOf(UUserWidget::StaticClass())) { Result->SetError(TEXT("v0.5.1 Hub Root is not a VerticalBox or terminal class is unavailable; no package was changed.")); return Result; }
	if (!HubRoot->WidgetTree->FindWidget(TEXT("CompanionQuestTerminal")))
	{
		const TSubclassOf<UUserWidget> TerminalClass(Companion->GeneratedClass);
		UUserWidget* Terminal = HubRoot->WidgetTree->ConstructWidget<UUserWidget>(TerminalClass, TEXT("CompanionQuestTerminal"));
		if (!Terminal) { Result->SetError(TEXT("v0.5.1 companion terminal could not be embedded; no package was changed.")); return Result; }
		RootPanel->AddChildToVerticalBox(Terminal);
	}
	PRRealityHubAuthoring::EnsureWidgetVariableGuids(Companion); PRRealityHubAuthoring::EnsureWidgetVariableGuids(HubRoot);
	Companion->MarkPackageDirty(); HubRoot->MarkPackageDirty(); Result->SetValue(TEXT("{\"status\":\"PASS\",\"packages\":[\"/Game/ProjectR/UI/RealityHub/WBP_RealityHubCompanionTerminal\",\"/Game/ProjectR/UI/RealityHub/WBP_RealityHubRoot\"],\"saved\":false}")); return Result;
}

UToolCallAsyncResultString* UPRRealityHubAuthoringToolset::CreateV052MemoryManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); PRRealityHubAuthoring::FRoot Root(Result);
	constexpr const TCHAR* RegistryPath = TEXT("/Game/ProjectR/Data/Memory/DA_MemoryRegistry");
	constexpr const TCHAR* AxiomPath = TEXT("/Game/ProjectR/Data/Memory/Companions/DA_MemoryPersona_Axiom");
	constexpr const TCHAR* KindlePath = TEXT("/Game/ProjectR/Data/Memory/Companions/DA_MemoryPersona_Kindle");
	constexpr const TCHAR* NullPath = TEXT("/Game/ProjectR/Data/Memory/Companions/DA_MemoryPersona_Null");
	constexpr const TCHAR* SummaryWidgetPath = TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubMemorySummary");
	constexpr const TCHAR* RootWidgetPath = TEXT("/Game/ProjectR/UI/RealityHub/WBP_RealityHubRoot.WBP_RealityHubRoot");
	const TArray<const TCHAR*> NewPaths = { RegistryPath, AxiomPath, KindlePath, NullPath, SummaryWidgetPath };
	for (const TCHAR* Path : NewPaths)
	{
		if (FindObject<UObject>(nullptr, Path) || FPackageName::DoesPackageExist(Path)) { Result->SetError(FString::Printf(TEXT("v0.5.2 fixed package path already exists: %s"), Path)); return Result; }
	}
	UWidgetBlueprint* HubRoot = LoadObject<UWidgetBlueprint>(nullptr, RootWidgetPath);
	if (!HubRoot || !HubRoot->WidgetTree || !HubRoot->ParentClass || !HubRoot->ParentClass->IsChildOf(UPRCompanionQuestHubWidget::StaticClass())) { Result->SetError(TEXT("Reality Hub Root is missing or is not the frozen v0.5.1 companion-quest class.")); return Result; }
	auto CreateData = [](const TCHAR* Path, UClass* Class) -> UPrimaryDataAsset*
	{
		UPackage* Package = CreatePackage(Path);
		UPrimaryDataAsset* Asset = NewObject<UPrimaryDataAsset>(Package, Class, FName(*FPackageName::GetLongPackageAssetName(Path)), RF_Public | RF_Standalone);
		if (Asset) FAssetRegistryModule::AssetCreated(Asset);
		return Asset;
	};
	UPRMemoryPersonaDataAsset* Axiom = Cast<UPRMemoryPersonaDataAsset>(CreateData(AxiomPath, UPRMemoryPersonaDataAsset::StaticClass()));
	UPRMemoryPersonaDataAsset* Kindle = Cast<UPRMemoryPersonaDataAsset>(CreateData(KindlePath, UPRMemoryPersonaDataAsset::StaticClass()));
	UPRMemoryPersonaDataAsset* Null = Cast<UPRMemoryPersonaDataAsset>(CreateData(NullPath, UPRMemoryPersonaDataAsset::StaticClass()));
	UPRMemoryRegistryDataAsset* Registry = Cast<UPRMemoryRegistryDataAsset>(CreateData(RegistryPath, UPRMemoryRegistryDataAsset::StaticClass()));
	if (!Axiom || !Kindle || !Null || !Registry) { Result->SetError(TEXT("v0.5.2 DataAsset creation failed before configuration.")); return Result; }
	auto ConfigurePersona = [](UPRMemoryPersonaDataAsset* Persona, const TCHAR* Tag, const TCHAR* ProviderId, std::initializer_list<const TCHAR*> Emotions, std::initializer_list<const TCHAR*> Templates, std::initializer_list<const TCHAR*> OptionIds, std::initializer_list<const TCHAR*> Labels)
	{
		Persona->CompanionId = FGameplayTag::RequestGameplayTag(Tag, false); Persona->ProviderCompanionId = ProviderId;
		for (const TCHAR* Value : Emotions) Persona->EmotionIds.Add(FName(Value));
		for (const TCHAR* Value : Templates) Persona->SummaryTemplates.Add(FText::FromString(Value));
		auto OptionIt = OptionIds.begin(); auto LabelIt = Labels.begin();
		for (; OptionIt != OptionIds.end() && LabelIt != Labels.end(); ++OptionIt, ++LabelIt) { FPRMemoryPlayerOptionDefinition& Option = Persona->PlayerOptions.AddDefaulted_GetRef(); Option.OptionId = FName(*OptionIt); Option.DisplayText = FText::FromString(*LabelIt); }
		Persona->MarkPackageDirty();
	};
	ConfigurePersona(Axiom, TEXT("Companion.Axiom"), TEXT("Axiom"), { TEXT("analytical"), TEXT("concerned"), TEXT("quietly_proud") }, { TEXT("The verified outcome is ready for review."), TEXT("The bounded facts require careful attention."), TEXT("The record supports the next deliberate choice.") }, { TEXT("axiom_reflect"), TEXT("axiom_challenge"), TEXT("axiom_deflect") }, { TEXT("Reflect"), TEXT("Challenge"), TEXT("Deflect") });
	ConfigurePersona(Kindle, TEXT("Companion.Kindle"), TEXT("Kindle"), { TEXT("fired_up"), TEXT("frustrated"), TEXT("relieved") }, { TEXT("Keep the verified momentum."), TEXT("The loss is recorded without excuse."), TEXT("We survived the facts and move forward.") }, { TEXT("kindle_steady"), TEXT("kindle_critique"), TEXT("kindle_thank") }, { TEXT("Steady"), TEXT("Critique"), TEXT("Thank") });
	ConfigurePersona(Null, TEXT("Companion.Null"), TEXT("Null"), { TEXT("sarcastic"), TEXT("sarcastic_worried"), TEXT("sincere") }, { TEXT("A neat archive is still an archive."), TEXT("The facts are intact, which is annoyingly useful."), TEXT("This run is remembered in bounded form.") }, { TEXT("null_promise"), TEXT("null_callout"), TEXT("null_analyze") }, { TEXT("Promise"), TEXT("Call Out"), TEXT("Analyze") });
	Registry->Personas = { Axiom, Kindle, Null }; Registry->MarkPackageDirty();
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>(); Factory->ParentClass = UPRMemorySummaryWidget::StaticClass();
	UWidgetBlueprint* SummaryWidget = Cast<UWidgetBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(FPackageName::GetLongPackageAssetName(SummaryWidgetPath), FPackageName::GetLongPackagePath(SummaryWidgetPath), UWidgetBlueprint::StaticClass(), Factory));
	if (!SummaryWidget || !SummaryWidget->WidgetTree) { Result->SetError(TEXT("v0.5.2 Memory Summary Widget creation failed.")); return Result; }
	UVerticalBox* SummaryRoot = SummaryWidget->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root")); SummaryWidget->WidgetTree->RootWidget = SummaryRoot;
	auto AddText = [SummaryWidget, SummaryRoot](const TCHAR* Name, const TCHAR* Label) { UTextBlock* Text = SummaryWidget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name); Text->SetText(FText::FromString(Label)); SummaryRoot->AddChildToVerticalBox(Text); };
	auto AddButton = [SummaryWidget, SummaryRoot](const TCHAR* Name, const TCHAR* Label) { UButton* Button = SummaryWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name); UTextBlock* Text = SummaryWidget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(FString(Name) + TEXT("_Text")))); Text->SetText(FText::FromString(Label)); Button->AddChild(Text); SummaryRoot->AddChildToVerticalBox(Button); };
	AddText(TEXT("Text_MemoryStatus"), TEXT("Memory Summary: waiting for an archived bounded record.")); AddButton(TEXT("Button_MemoryOptionOne"), TEXT("Option One")); AddButton(TEXT("Button_MemoryOptionTwo"), TEXT("Option Two")); AddButton(TEXT("Button_MemoryOptionThree"), TEXT("Option Three"));
	PRRealityHubAuthoring::EnsureWidgetVariableGuids(SummaryWidget); FKismetEditorUtilities::CompileBlueprint(SummaryWidget); SummaryWidget->MarkPackageDirty();
	HubRoot->Modify(); HubRoot->ParentClass = UPRMemoryHubWidget::StaticClass();
	UVerticalBox* RootPanel = Cast<UVerticalBox>(HubRoot->WidgetTree->RootWidget);
	if (!RootPanel || !SummaryWidget->GeneratedClass) { Result->SetError(TEXT("Reality Hub Root lacks its fixed VerticalBox or Memory widget class.")); return Result; }
	if (!HubRoot->WidgetTree->FindWidget(TEXT("Button_MemorySummary"))) { UButton* Button = HubRoot->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_MemorySummary")); UTextBlock* Text = HubRoot->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Button_MemorySummary_Text")); Text->SetText(FText::FromString(TEXT("Memory Summary"))); Button->AddChild(Text); RootPanel->AddChildToVerticalBox(Button); }
	if (!HubRoot->WidgetTree->FindWidget(TEXT("MemorySummaryPanel"))) { const TSubclassOf<UUserWidget> SummaryClass(Cast<UClass>(SummaryWidget->GeneratedClass)); UUserWidget* Panel = HubRoot->WidgetTree->ConstructWidget<UUserWidget>(SummaryClass, TEXT("MemorySummaryPanel")); if (!Panel) { Result->SetError(TEXT("Memory Summary panel could not be embedded in Reality Hub Root.")); return Result; } RootPanel->AddChildToVerticalBox(Panel); }
	PRRealityHubAuthoring::EnsureWidgetVariableGuids(HubRoot); FKismetEditorUtilities::CompileBlueprint(HubRoot); HubRoot->MarkPackageDirty();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":[\"/Game/ProjectR/Data/Memory/DA_MemoryRegistry\",\"/Game/ProjectR/Data/Memory/Companions/DA_MemoryPersona_Axiom\",\"/Game/ProjectR/Data/Memory/Companions/DA_MemoryPersona_Kindle\",\"/Game/ProjectR/Data/Memory/Companions/DA_MemoryPersona_Null\",\"/Game/ProjectR/UI/RealityHub/WBP_RealityHubMemorySummary\"],\"modified\":[\"/Game/ProjectR/UI/RealityHub/WBP_RealityHubRoot\"],\"saved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRRealityHubAuthoringToolset::PrepareV052MemoryPIE()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
#if WITH_DEV_AUTOMATION_TESTS
	if (GEditor && GEditor->PlayWorld) { Result->SetError(TEXT("PrepareV052MemoryPIE must run before PIE starts.")); return Result; }
	// Save slot policy permits only this fixed automation prefix; the unique GUID
	// keeps the Memory fixture physically separate from every real profile.
	const FString Base = FString::Printf(TEXT("ProjectR_Automation_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower());
	TSharedPtr<FPRSaveStorage> Storage = FPRSaveStorage::CreateAutomation(Base);
	if (!Storage) { Result->SetError(TEXT("Could not create isolated v0.5.2 automation storage.")); return Result; }
	UPRSaveSubsystem::SetAutomationStorageOverride(MoveTemp(Storage));
	Result->SetValue(TEXT("{\"status\":\"READY\",\"slots\":\"automation-guid-only\",\"userSlotsTouched\":false}"));
#else
	Result->SetError(TEXT("v0.5.2 Memory PIE verification is unavailable outside development automation builds."));
#endif
	return Result;
}

UToolCallAsyncResultString* UPRRealityHubAuthoringToolset::RunV052MemorySingleDeathPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRRealityHubAuthoring::FFixedMemorySingleDeathRunner::Start();
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRRealityHubAuthoringToolset::RunV052MemoryMixedTerminationPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	return PRRealityHubAuthoring::FFixedMemorySingleDeathRunner::Start(true);
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}

UToolCallAsyncResultString* UPRRealityHubAuthoringToolset::CleanupV052MemoryPIE()
{
#if WITH_DEV_AUTOMATION_TESTS
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	if (GEditor && GEditor->PlayWorld)
	{
		Result->SetError(TEXT("CleanupV052MemoryPIE must run after PIE stops."));
		return Result;
	}
	UPRSaveSubsystem::CleanupAutomationStorageOverride();
	Result->SetValue(TEXT("{\"status\":\"CLEAN\",\"automationSlotsOnly\":true}"));
	return Result;
#else
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); Result->SetError(TEXT("Unavailable.")); return Result;
#endif
}
