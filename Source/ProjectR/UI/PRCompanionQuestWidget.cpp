// Copyright Epic Games, Inc. All Rights Reserved.
#include "UI/PRCompanionQuestWidget.h"
#include "Quests/PRCompanionQuestSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Roguelike/Account/PRRunStateSubsystem.h"

namespace PRCompanionQuestWidgetPrivate
{
const TCHAR* GetStateText(const EPRCompanionQuestState State)
{
	switch (State)
	{
	case EPRCompanionQuestState::Locked: return TEXT("Locked");
	case EPRCompanionQuestState::Available: return TEXT("Available");
	case EPRCompanionQuestState::Active: return TEXT("Active");
	case EPRCompanionQuestState::PersistencePending: return TEXT("Saving");
	case EPRCompanionQuestState::ReadyToRetry: return TEXT("Retry save");
	case EPRCompanionQuestState::Completed: return TEXT("Completed");
	default: return TEXT("Unavailable");
	}
}

const TCHAR* GetOperationText(const EPRCompanionQuestOperationResult Result)
{
	switch (Result)
	{
	case EPRCompanionQuestOperationResult::Started: return TEXT("Saving request");
	case EPRCompanionQuestOperationResult::Succeeded: return TEXT("Saved");
	case EPRCompanionQuestOperationResult::AlreadyActive: return TEXT("Already active");
	case EPRCompanionQuestOperationResult::AlreadyCompleted: return TEXT("Already completed");
	case EPRCompanionQuestOperationResult::RejectedNoProfile: return TEXT("Profile unavailable");
	case EPRCompanionQuestOperationResult::RejectedRegistryUnavailable: return TEXT("Quest registry unavailable");
	case EPRCompanionQuestOperationResult::RejectedUnknownQuest: return TEXT("Fixed quest unavailable");
	case EPRCompanionQuestOperationResult::RejectedNotEligible: return TEXT("Requirements not met");
	case EPRCompanionQuestOperationResult::RejectedBusy: return TEXT("Another save is in progress");
	case EPRCompanionQuestOperationResult::RejectedNoPendingPersistence: return TEXT("Nothing to retry");
	case EPRCompanionQuestOperationResult::PersistenceFailed: return TEXT("Save failed; retry is available");
	default: return TEXT("Request unavailable");
	}
}
}

void UPRCompanionQuestWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UPRCompanionQuestSubsystem* Quests=GetQuestSubsystem())
	{
		StateChangedHandle=Quests->OnStateChanged().AddUObject(this,&UPRCompanionQuestWidget::HandleStateChanged);
		OperationHandle=Quests->OnOperation().AddUObject(this,&UPRCompanionQuestWidget::HandleOperation);
		FPRCompanionQuestSnapshot Snapshot; Quests->GetSnapshot(Snapshot); PresentQuestSnapshot(Snapshot); PresentReadOnlyText(Snapshot);
	}
	if (UButton* Button=Cast<UButton>(GetWidgetFromName(TEXT("Button_Quest_Axiom_LowProbabilitySample")))) Button->OnClicked.AddDynamic(this,&UPRCompanionQuestWidget::ActivateAxiomLowProbabilitySample);
	if (UButton* Button=Cast<UButton>(GetWidgetFromName(TEXT("Button_Quest_Axiom_ImperfectOptimum")))) Button->OnClicked.AddDynamic(this,&UPRCompanionQuestWidget::ActivateAxiomImperfectOptimum);
	if (UButton* Button=Cast<UButton>(GetWidgetFromName(TEXT("Button_Quest_Kindle_NoRetreatLine")))) Button->OnClicked.AddDynamic(this,&UPRCompanionQuestWidget::ActivateKindleNoRetreatLine);
	if (UButton* Button=Cast<UButton>(GetWidgetFromName(TEXT("Button_Quest_Kindle_LearnToRetreat")))) Button->OnClicked.AddDynamic(this,&UPRCompanionQuestWidget::ActivateKindleLearnToRetreat);
	if (UButton* Button=Cast<UButton>(GetWidgetFromName(TEXT("Button_Quest_Null_GarbageCollection")))) Button->OnClicked.AddDynamic(this,&UPRCompanionQuestWidget::ActivateNullGarbageCollection);
	if (UButton* Button=Cast<UButton>(GetWidgetFromName(TEXT("Button_Quest_Null_RememberMe")))) Button->OnClicked.AddDynamic(this,&UPRCompanionQuestWidget::ActivateNullRememberMe);
	if (UButton* Button=Cast<UButton>(GetWidgetFromName(TEXT("Button_Quest_RetrySave")))) Button->OnClicked.AddDynamic(this,&UPRCompanionQuestWidget::RetryQuestPersistence);
	if (UButton* Confirm=Cast<UButton>(GetWidgetFromName(TEXT("Button_ConfirmRememberMe")))) Confirm->OnClicked.AddDynamic(this,&UPRCompanionQuestWidget::ConfirmRememberMeAfterDisplayedGraveyard);
	PresentReadOnlyGraveyardProjection();
}

void UPRCompanionQuestWidget::NativeDestruct()
{
	if (UPRCompanionQuestSubsystem* Quests=GetQuestSubsystem()) { Quests->OnStateChanged().Remove(StateChangedHandle); Quests->OnOperation().Remove(OperationHandle); }
	StateChangedHandle.Reset(); OperationHandle.Reset(); Super::NativeDestruct();
}

void UPRCompanionQuestWidget::ActivateAxiomLowProbabilitySample() { RequestFixedQuest(TEXT("Quest.Axiom.LowProbabilitySample")); }
void UPRCompanionQuestWidget::ActivateAxiomImperfectOptimum() { RequestFixedQuest(TEXT("Quest.Axiom.ImperfectOptimum")); }
void UPRCompanionQuestWidget::ActivateKindleNoRetreatLine() { RequestFixedQuest(TEXT("Quest.Kindle.NoRetreatLine")); }
void UPRCompanionQuestWidget::ActivateKindleLearnToRetreat() { RequestFixedQuest(TEXT("Quest.Kindle.LearnToRetreat")); }
void UPRCompanionQuestWidget::ActivateNullGarbageCollection() { RequestFixedQuest(TEXT("Quest.Null.GarbageCollection")); }
void UPRCompanionQuestWidget::ActivateNullRememberMe() { RequestFixedQuest(TEXT("Quest.Null.RememberMe")); }
void UPRCompanionQuestWidget::RetryQuestPersistence()
{
	if (UPRCompanionQuestSubsystem* Quests=GetQuestSubsystem())
	{
		const EPRCompanionQuestOperationResult Result=Quests->RetryPendingPersistence();
		if (UTextBlock* Status=Cast<UTextBlock>(GetWidgetFromName(TEXT("QuestStatus")))) Status->SetText(FText::FromString(PRCompanionQuestWidgetPrivate::GetOperationText(Result)));
	}
}

void UPRCompanionQuestWidget::RequestFixedQuest(const FName QuestId)
{
	if (UPRCompanionQuestSubsystem* Quests=GetQuestSubsystem()) { FGuid RequestId; const EPRCompanionQuestOperationResult Result=Quests->RequestActivateQuest(QuestId,RequestId); if (UTextBlock* Status=Cast<UTextBlock>(GetWidgetFromName(TEXT("QuestStatus")))) Status->SetText(FText::Format(FText::FromString(TEXT("{0}: {1}")),FText::FromName(QuestId),FText::FromString(PRCompanionQuestWidgetPrivate::GetOperationText(Result)))); }
}

void UPRCompanionQuestWidget::HandleStateChanged(const FPRCompanionQuestSnapshot& NewSnapshot) { PresentQuestSnapshot(NewSnapshot); PresentReadOnlyText(NewSnapshot); PresentReadOnlyGraveyardProjection(); }
void UPRCompanionQuestWidget::HandleOperation(const FPRCompanionQuestOperationEvent& Event) { if (UTextBlock* Status=Cast<UTextBlock>(GetWidgetFromName(TEXT("QuestStatus")))) Status->SetText(FText::Format(FText::FromString(TEXT("{0}: {1}")),FText::FromName(Event.QuestId),FText::FromString(PRCompanionQuestWidgetPrivate::GetOperationText(Event.Result)))); }
void UPRCompanionQuestWidget::PresentReadOnlyText(const FPRCompanionQuestSnapshot& NewSnapshot)
{
	if (UTextBlock* Status=Cast<UTextBlock>(GetWidgetFromName(TEXT("QuestStatus"))))
	{
		FString Text=TEXT("Companion quests — fixed objectives only\n");
		for(const FPRCompanionQuestEntrySnapshot& Entry:NewSnapshot.Entries) Text+=FString::Printf(TEXT("%s: %s\n"),*Entry.DisplayName.ToString(),PRCompanionQuestWidgetPrivate::GetStateText(Entry.State));
		Status->SetText(FText::FromString(Text));
	}
}

void UPRCompanionQuestWidget::PresentReadOnlyGraveyardProjection()
{
	UPRRunStateSubsystem* RunState=GetGameInstance()?GetGameInstance()->GetSubsystem<UPRRunStateSubsystem>():nullptr;
	TArray<FPRAccountRecord> Records; if(RunState) RunState->GetGraveyardSnapshot(Records);
	TSet<FGuid> Seen; FString Text=TEXT("Read-only graveyard projection (five unique records required):\n"); int32 Count=0;
	for(const FPRAccountRecord& Record:Records) if(Record.RecordId.IsValid() && !Seen.Contains(Record.RecordId) && Count<5) { Seen.Add(Record.RecordId); ++Count; Text+=FString::Printf(TEXT("%d. %s — %s\n"),Count,*Record.IdentityId.ToString(),*Record.RecordId.ToString(EGuidFormats::Digits)); }
	Text+=FString::Printf(TEXT("Visible unique records: %d / 5"),Count);
	if(UTextBlock* Projection=Cast<UTextBlock>(GetWidgetFromName(TEXT("GraveyardProjection")))) Projection->SetText(FText::FromString(Text));
	if(UButton* Confirm=Cast<UButton>(GetWidgetFromName(TEXT("Button_ConfirmRememberMe")))) Confirm->SetIsEnabled(Count>=5);
}

void UPRCompanionQuestWidget::ConfirmRememberMeAfterDisplayedGraveyard() { if (UPRCompanionQuestSubsystem* Quests=GetQuestSubsystem()) Quests->ConfirmNullRememberMeRecordsViewed(); }
UPRCompanionQuestSubsystem* UPRCompanionQuestWidget::GetQuestSubsystem() const { return GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRCompanionQuestSubsystem>() : nullptr; }
