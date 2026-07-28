// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/PRRealityHubWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "RealityHub/PRRealityHubSubsystem.h"

void UPRRealityHubWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_CassetteSlot) Button_CassetteSlot->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleCassetteClicked);
	if (Button_CreateProfile) Button_CreateProfile->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleCreateProfileClicked);
	if (Button_IdentityTechnician) Button_IdentityTechnician->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleTechnicianClicked);
	if (Button_IdentitySecurity) Button_IdentitySecurity->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleSecurityClicked);
	if (Button_IdentityExile) Button_IdentityExile->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleExileClicked);
	if (Button_IdentityObserver) Button_IdentityObserver->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleObserverClicked);
	if (Button_IdentityBlank) Button_IdentityBlank->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleBlankClicked);
	if (Button_EnterNetwork) Button_EnterNetwork->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleEnterNetworkClicked);
	if (Button_Companion) Button_Companion->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleCompanionClicked);
	if (Button_Graveyard) Button_Graveyard->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleGraveyardClicked);
	if (Button_TrainingSimulator) Button_TrainingSimulator->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleTrainingClicked);
	if (Button_DirectorForecaster) Button_DirectorForecaster->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleForecasterClicked);
	if (Button_Progression) Button_Progression->OnClicked.AddDynamic(this, &UPRRealityHubWidget::HandleProgressionClicked);
	if (UPRRealityHubSubsystem* Hub = GetHub())
	{
		StateChangedHandle = Hub->OnStateChanged().AddUObject(this, &UPRRealityHubWidget::HandleStateChanged);
		OperationHandle = Hub->OnOperation().AddUObject(this, &UPRRealityHubWidget::HandleOperation);
		Hub->GetSnapshot(DisplayedSnapshot);
		DisplayedForecast = Hub->GetForecast();
		PresentSnapshot(DisplayedSnapshot);
		SetStatusText(FText::FromString(TEXT("Reality Hub ready. Select a fixed terminal.")));
	}
}

void UPRRealityHubWidget::NativeDestruct()
{
	if (UPRRealityHubSubsystem* Hub = GetHub())
	{
		Hub->OnStateChanged().Remove(StateChangedHandle);
		Hub->OnOperation().Remove(OperationHandle);
	}
	StateChangedHandle.Reset();
	OperationHandle.Reset();
	Super::NativeDestruct();
}

FPRRealityHubSnapshot UPRRealityHubWidget::GetDisplayedSnapshot() const { return DisplayedSnapshot; }
FPRRealityHubForecast UPRRealityHubWidget::GetDisplayedForecast() const { return DisplayedForecast; }
void UPRRealityHubWidget::HandleCassetteClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestLoadDefaultProfile(); }
void UPRRealityHubWidget::HandleCreateProfileClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestCreateDefaultProfile(); }
void UPRRealityHubWidget::HandleTechnicianClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestCreateFixedIdentityAccount(EPRRealityHubIdentity::Technician); }
void UPRRealityHubWidget::HandleSecurityClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestCreateFixedIdentityAccount(EPRRealityHubIdentity::Security); }
void UPRRealityHubWidget::HandleExileClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestCreateFixedIdentityAccount(EPRRealityHubIdentity::Exile); }
void UPRRealityHubWidget::HandleObserverClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestCreateFixedIdentityAccount(EPRRealityHubIdentity::Observer); }
void UPRRealityHubWidget::HandleBlankClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestCreateFixedIdentityAccount(EPRRealityHubIdentity::Blank); }
void UPRRealityHubWidget::HandleEnterNetworkClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestStartRun(); }
void UPRRealityHubWidget::HandleCompanionClicked()
{
	if (UPRRealityHubSubsystem* Hub = GetHub())
	{
		Hub->GetSnapshot(DisplayedSnapshot);
		PresentSnapshot(DisplayedSnapshot);
		SetStatusText(FText::FromString(TEXT("Companion terminal: dialogue, repair, and personal quests are future Provider content and cannot run yet.")));
	}
}

void UPRRealityHubWidget::HandleGraveyardClicked()
{
	if (UPRRealityHubSubsystem* Hub = GetHub())
	{
		TArray<FPRAccountRecord> Records;
		Hub->GetGraveyardSnapshot(Records);
		Hub->GetSnapshot(DisplayedSnapshot);
		PresentSnapshot(DisplayedSnapshot);
		SetStatusText(FText::Format(FText::FromString(TEXT("Graveyard: {0} read-only historical account records. No delete or export is available.")), FText::AsNumber(Records.Num())));
	}
}
void UPRRealityHubWidget::HandleTrainingClicked() { if (UPRRealityHubSubsystem* Hub = GetHub()) Hub->RequestTrainingTravel(); }
void UPRRealityHubWidget::HandleForecasterClicked()
{
	if (UPRRealityHubSubsystem* Hub = GetHub())
	{
		DisplayedForecast = Hub->GetForecast();
		Hub->GetSnapshot(DisplayedSnapshot);
		PresentSnapshot(DisplayedSnapshot);
		SetStatusText(DisplayedForecast.Result == EPRRealityHubForecastResult::Available
			? FText::Format(FText::FromString(TEXT("Forecast: {0}, level {1}. Local only; no Director Rule is applied.")), FText::FromString(DisplayedForecast.RuleId.ToString()), FText::AsNumber(DisplayedForecast.Level))
			: FText::FromString(TEXT("No forecast is available. Profile or fixed Registry is unavailable.")));
	}
}

void UPRRealityHubWidget::HandleProgressionClicked()
{
	if (UPRRealityHubSubsystem* Hub = GetHub())
	{
		FPRProgressionSnapshot Progression;
		const bool bAvailable = Hub->GetProgressionSnapshot(Progression);
		Hub->GetSnapshot(DisplayedSnapshot);
		PresentSnapshot(DisplayedSnapshot);
		SetStatusText(bAvailable
			? FText::Format(FText::FromString(TEXT("Progression: {0} Counterproof Fragments, {1} unlocked nodes. Effects apply only to the next successful run.")), FText::AsNumber(Progression.CounterproofFragments), FText::AsNumber(Progression.UnlockedNodeIds.Num()))
			: FText::FromString(TEXT("Progression is unavailable until a profile is loaded.")));
	}
}
void UPRRealityHubWidget::HandleStateChanged(const FPRRealityHubSnapshot& NewSnapshot) { DisplayedSnapshot = NewSnapshot; DisplayedForecast = GetHub() ? GetHub()->GetForecast() : FPRRealityHubForecast(); PresentSnapshot(DisplayedSnapshot); }
void UPRRealityHubWidget::HandleOperation(const FPRRealityHubOperationEvent& Event) { PresentOperation(Event); SetStatusText(Event.Message); }
void UPRRealityHubWidget::SetStatusText(const FText& NewStatus) { if (Text_Status) Text_Status->SetText(NewStatus); }
UPRRealityHubSubsystem* UPRRealityHubWidget::GetHub() const { return GetGameInstance() ? GetGameInstance()->GetSubsystem<UPRRealityHubSubsystem>() : nullptr; }
