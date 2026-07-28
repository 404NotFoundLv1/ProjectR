// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRRealityHubAuthoringToolset.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRRealityHubHUD.h"
#include "UI/PRRealityHubTrainingReturnWidget.h"
#include "UI/PRRealityHubWidget.h"
#include "WidgetBlueprint.h"

namespace PRRealityHubAuthoring
{
struct FRoot { explicit FRoot(UToolCallAsyncResultString* In) : Value(In) { Value->AddToRoot(); } ~FRoot() { Value->RemoveFromRoot(); } UToolCallAsyncResultString* Value; };

UWidgetBlueprint* LoadWidget(const TCHAR* Path)
{
	return LoadObject<UWidgetBlueprint>(nullptr, Path);
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
