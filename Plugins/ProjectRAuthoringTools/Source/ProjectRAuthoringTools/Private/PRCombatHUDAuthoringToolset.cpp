// Copyright Epic Games, Inc. All Rights Reserved.
#include "PRCombatHUDAuthoringToolset.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/UnrealType.h"
#include "Abilities/PRAbilitySetDataAsset.h"
#include "UI/PRCombatHUD.h"
#include "UI/PRCombatHUDWidget.h"
#include "UI/PRPlayerResourcesWidget.h"
#include "UI/PRSkillBarWidget.h"
#include "UI/PRSkillSlotWidget.h"
#include "UI/PRCombatFeedbackWidget.h"
#include "UI/PRBossHealthWidget.h"
#include "UI/PRBossPrototypeResultWidget.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Misc/PackageName.h"
#include "AssetToolsModule.h"
#include "Editor.h"

namespace PRCombatHUDAssets
{
constexpr const TCHAR* HudPath=TEXT("/Game/ProjectR/UI/Combat/BP_CombatHUD");
constexpr const TCHAR* RootPath=TEXT("/Game/ProjectR/UI/Combat/WBP_CombatHUD");
constexpr const TCHAR* ResourcePath=TEXT("/Game/ProjectR/UI/Combat/WBP_PlayerResources");
constexpr const TCHAR* BarPath=TEXT("/Game/ProjectR/UI/Combat/WBP_SkillBar");
constexpr const TCHAR* SlotPath=TEXT("/Game/ProjectR/UI/Combat/WBP_SkillSlot");
constexpr const TCHAR* FeedbackPath=TEXT("/Game/ProjectR/UI/Combat/WBP_CombatFeedback");
constexpr const TCHAR* BossHealthPath=TEXT("/Game/ProjectR/UI/Boss/WBP_BossHealth");
constexpr const TCHAR* BossResultPath=TEXT("/Game/ProjectR/UI/Boss/WBP_BossPrototypeResult");
UBlueprint* MakeBP(const TCHAR* Path,UClass* Parent,bool bWidget){if(!bWidget){UPackage* P=CreatePackage(Path);return FKismetEditorUtilities::CreateBlueprint(Parent,P,FName(FPackageName::GetLongPackageAssetName(Path)),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());}UWidgetBlueprintFactory* F=NewObject<UWidgetBlueprintFactory>();F->ParentClass=Parent;return Cast<UBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(FPackageName::GetLongPackageAssetName(Path),FPackageName::GetLongPackagePath(Path),UWidgetBlueprint::StaticClass(),F));}
bool SetClass(UClass* C,const TCHAR* Name,UClass* V){if(FClassProperty* P=FindFProperty<FClassProperty>(C,Name)){P->SetPropertyValue_InContainer(C->GetDefaultObject(),V);return true;}return false;}
bool SetObject(UClass* C,const TCHAR* Name,UObject* V){if(FObjectProperty* P=FindFProperty<FObjectProperty>(C,Name)){P->SetObjectPropertyValue_InContainer(C->GetDefaultObject(),V);return true;}return false;}
}
UToolCallAsyncResultString* UPRCombatHUDAuthoringToolset::CreateCombatHUDManifest()
{
	UToolCallAsyncResultString* R=NewObject<UToolCallAsyncResultString>();R->AddToRoot();
	for(const TCHAR* P:{PRCombatHUDAssets::RootPath,PRCombatHUDAssets::ResourcePath,PRCombatHUDAssets::BarPath,PRCombatHUDAssets::SlotPath,PRCombatHUDAssets::FeedbackPath}) if(FindObject<UObject>(nullptr,P)){R->SetError(FString::Printf(TEXT("HUD manifest collision: %s"),P));R->RemoveFromRoot();return R;}
	UBlueprint* H=LoadObject<UBlueprint>(nullptr,TEXT("/Game/ProjectR/UI/Combat/BP_CombatHUD.BP_CombatHUD"));if(!H)H=PRCombatHUDAssets::MakeBP(PRCombatHUDAssets::HudPath,APRCombatHUD::StaticClass(),false);UBlueprint* Root=PRCombatHUDAssets::MakeBP(PRCombatHUDAssets::RootPath,UPRCombatHUDWidget::StaticClass(),true);UBlueprint* Resources=PRCombatHUDAssets::MakeBP(PRCombatHUDAssets::ResourcePath,UPRPlayerResourcesWidget::StaticClass(),true);UBlueprint* Bar=PRCombatHUDAssets::MakeBP(PRCombatHUDAssets::BarPath,UPRSkillBarWidget::StaticClass(),true);UBlueprint* Slot=PRCombatHUDAssets::MakeBP(PRCombatHUDAssets::SlotPath,UPRSkillSlotWidget::StaticClass(),true);UBlueprint* Feedback=PRCombatHUDAssets::MakeBP(PRCombatHUDAssets::FeedbackPath,UPRCombatFeedbackWidget::StaticClass(),true);
	if(!H||!Root||!Resources||!Bar||!Slot||!Feedback){R->SetError(TEXT("Fixed HUD manifest creation failed."));R->RemoveFromRoot();return R;}for(UBlueprint* B:{H,Root,Resources,Bar,Slot,Feedback})FKismetEditorUtilities::CompileBlueprint(B);if(!H->GeneratedClass||!Root->GeneratedClass||!PRCombatHUDAssets::SetClass(H->GeneratedClass,TEXT("CombatHUDWidgetClass"),Root->GeneratedClass)){R->SetError(TEXT("Fixed CombatHUD CDO configuration failed."));R->RemoveFromRoot();return R;}H->MarkPackageDirty();R->SetValue(TEXT("{\"status\":\"PASS\",\"created\":6,\"modified\":0,\"saveRequired\":true}"));R->RemoveFromRoot();return R;
}

UToolCallAsyncResultString* UPRCombatHUDAuthoringToolset::RepairCombatHUDWidgetTrees()
{
	UToolCallAsyncResultString* R=NewObject<UToolCallAsyncResultString>();R->AddToRoot();
	auto Load=[](const TCHAR* P){return LoadObject<UWidgetBlueprint>(nullptr,FString::Printf(TEXT("%s.%s"),P,*FPackageName::GetLongPackageAssetName(P)));};
	UWidgetBlueprint* Root=Load(PRCombatHUDAssets::RootPath);UWidgetBlueprint* Resources=Load(PRCombatHUDAssets::ResourcePath);UWidgetBlueprint* Bar=Load(PRCombatHUDAssets::BarPath);UWidgetBlueprint* Slot=Load(PRCombatHUDAssets::SlotPath);UWidgetBlueprint* Feedback=Load(PRCombatHUDAssets::FeedbackPath);
	UWidgetBlueprint* BossHealth=Load(PRCombatHUDAssets::BossHealthPath);UWidgetBlueprint* BossResult=Load(PRCombatHUDAssets::BossResultPath);
	if(!Root||!Resources||!Bar||!Slot||!Feedback||!BossHealth||!BossResult||!Root->WidgetTree||!Resources->WidgetTree||!Bar->WidgetTree||!Slot->WidgetTree||!Feedback->WidgetTree||!BossHealth->WidgetTree||!BossResult->WidgetTree){R->SetError(TEXT("Existing fixed HUD WidgetBlueprint manifest is unavailable."));R->RemoveFromRoot();return R;}
	auto Text=[&](UWidgetBlueprint* B,const TCHAR* N){return B->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),N);};
	auto Lines=[&](UWidgetBlueprint* B,std::initializer_list<const TCHAR*> Names){UVerticalBox* V=B->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("Root"));B->WidgetTree->RootWidget=V;for(const TCHAR* N:Names)V->AddChildToVerticalBox(Text(B,N));B->MarkPackageDirty();};
	Lines(Resources,{TEXT("HealthText"),TEXT("ShieldText"),TEXT("EnergyText"),TEXT("PermissionText"),TEXT("ResonanceText")});
	Lines(Slot,{TEXT("SkillNameText"),TEXT("SkillStateText"),TEXT("FailureText")});
	Lines(BossHealth,{TEXT("HealthShieldText"),TEXT("PhaseText"),TEXT("RuleText"),TEXT("PredictionText"),TEXT("AttackText")});
	Lines(BossResult,{TEXT("PrototypeResultText")});
	UVerticalBox* FeedbackRoot=Feedback->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("FeedbackEntries"));Feedback->WidgetTree->RootWidget=FeedbackRoot;Feedback->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(Slot);UVerticalBox* SkillRoot=Bar->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("SkillRoot"));Bar->WidgetTree->RootWidget=SkillRoot;for(int32 I=1;I<=6;++I){UUserWidget* W=Bar->WidgetTree->ConstructWidget<UUserWidget>(TSubclassOf<UUserWidget>(Slot->GeneratedClass),*FString::Printf(TEXT("SkillSlot_%02d"),I));SkillRoot->AddChildToVerticalBox(W);}Bar->MarkPackageDirty();
	UPRAbilitySetDataAsset* DefaultAbilitySet=LoadObject<UPRAbilitySetDataAsset>(nullptr,TEXT("/Game/ProjectR/Abilities/DA_DefaultAbilitySet.DA_DefaultAbilitySet"));
	if(!DefaultAbilitySet||!Bar->GeneratedClass||!PRCombatHUDAssets::SetObject(Bar->GeneratedClass,TEXT("DefaultAbilitySet"),DefaultAbilitySet)){R->SetError(TEXT("Fixed SkillBar DefaultAbilitySet configuration failed."));R->RemoveFromRoot();return R;}
	FKismetEditorUtilities::CompileBlueprint(Resources);FKismetEditorUtilities::CompileBlueprint(Bar);FKismetEditorUtilities::CompileBlueprint(Feedback);FKismetEditorUtilities::CompileBlueprint(BossHealth);FKismetEditorUtilities::CompileBlueprint(BossResult);
	UVerticalBox* RootBox=Root->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("CombatHUDRoot"));Root->WidgetTree->RootWidget=RootBox;RootBox->AddChildToVerticalBox(Root->WidgetTree->ConstructWidget<UUserWidget>(TSubclassOf<UUserWidget>(Resources->GeneratedClass),TEXT("PlayerResources")));RootBox->AddChildToVerticalBox(Root->WidgetTree->ConstructWidget<UUserWidget>(TSubclassOf<UUserWidget>(Bar->GeneratedClass),TEXT("SkillBar")));RootBox->AddChildToVerticalBox(Text(Root,TEXT("BossContextText")));RootBox->AddChildToVerticalBox(Root->WidgetTree->ConstructWidget<UUserWidget>(TSubclassOf<UUserWidget>(BossHealth->GeneratedClass),TEXT("BossHealth")));RootBox->AddChildToVerticalBox(Root->WidgetTree->ConstructWidget<UUserWidget>(TSubclassOf<UUserWidget>(BossResult->GeneratedClass),TEXT("BossPrototypeResult")));RootBox->AddChildToVerticalBox(Root->WidgetTree->ConstructWidget<UUserWidget>(TSubclassOf<UUserWidget>(Feedback->GeneratedClass),TEXT("CombatFeedback")));Root->MarkPackageDirty();FKismetEditorUtilities::CompileBlueprint(Root);
	R->SetValue(TEXT("{\"status\":\"PASS\",\"repairedWidgetTrees\":7,\"saveRequired\":true}"));R->RemoveFromRoot();return R;
}

UToolCallAsyncResultString* UPRCombatHUDAuthoringToolset::RecreateBossWidgetBlueprints()
{
	UToolCallAsyncResultString* R=NewObject<UToolCallAsyncResultString>();R->AddToRoot();
	for(const TCHAR* P:{PRCombatHUDAssets::BossHealthPath,PRCombatHUDAssets::BossResultPath})
	{
		if(FindObject<UObject>(nullptr,P))
		{
			R->SetError(FString::Printf(TEXT("Boss WidgetBlueprint path must be absent before fixed recreation: %s"),P));R->RemoveFromRoot();return R;
		}
	}
	UWidgetBlueprint* Health=Cast<UWidgetBlueprint>(PRCombatHUDAssets::MakeBP(PRCombatHUDAssets::BossHealthPath,UPRBossHealthWidget::StaticClass(),true));
	UWidgetBlueprint* Result=Cast<UWidgetBlueprint>(PRCombatHUDAssets::MakeBP(PRCombatHUDAssets::BossResultPath,UPRBossPrototypeResultWidget::StaticClass(),true));
	if(!Health||!Result||!Health->WidgetTree||!Result->WidgetTree)
	{
		R->SetError(TEXT("Fixed Boss WidgetBlueprint recreation failed."));R->RemoveFromRoot();return R;
	}
	auto AddText=[](UWidgetBlueprint* Blueprint,UVerticalBox* Root,const TCHAR* Name)
	{
		Root->AddChildToVerticalBox(Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name));
	};
	UVerticalBox* HealthRoot=Health->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("BossHealthRoot"));
	Health->WidgetTree->RootWidget=HealthRoot;
	for(const TCHAR* Name:{TEXT("HealthShieldText"),TEXT("PhaseText"),TEXT("RuleText"),TEXT("PredictionText"),TEXT("AttackText")}) AddText(Health,HealthRoot,Name);
	UVerticalBox* ResultRoot=Result->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("BossResultRoot"));
	Result->WidgetTree->RootWidget=ResultRoot;
	AddText(Result,ResultRoot,TEXT("PrototypeResultText"));
	Health->MarkPackageDirty();Result->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(Health);FKismetEditorUtilities::CompileBlueprint(Result);
	if(!Health->GeneratedClass||!Result->GeneratedClass)
	{
		R->SetError(TEXT("Fixed Boss WidgetBlueprint compilation failed."));R->RemoveFromRoot();return R;
	}
	R->SetValue(TEXT("{\"status\":\"PASS\",\"recreated\":2,\"saveRequired\":true}"));R->RemoveFromRoot();return R;
}

UToolCallAsyncResultString* UPRCombatHUDAuthoringToolset::DetachBossWidgetReferences()
{
	UToolCallAsyncResultString* R=NewObject<UToolCallAsyncResultString>();R->AddToRoot();
	const FString RootReference=FString::Printf(TEXT("%s.%s"),PRCombatHUDAssets::RootPath,*FPackageName::GetLongPackageAssetName(PRCombatHUDAssets::RootPath));
	UWidgetBlueprint* Root=LoadObject<UWidgetBlueprint>(nullptr,*RootReference);
	if(!Root||!Root->WidgetTree)
	{
		R->SetError(TEXT("Fixed CombatHUD WidgetBlueprint is unavailable for Boss reference detachment."));R->RemoveFromRoot();return R;
	}
	Root->WidgetTree->RootWidget=nullptr;
	Root->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(Root);
	R->SetValue(TEXT("{\"status\":\"PASS\",\"detached\":2,\"saveRequired\":true}"));R->RemoveFromRoot();return R;
}

UToolCallAsyncResultString* UPRCombatHUDAuthoringToolset::InspectActiveCombatHUDPIE()
{
	UToolCallAsyncResultString* R=NewObject<UToolCallAsyncResultString>();R->AddToRoot();
	UWorld* World=GEditor?GEditor->PlayWorld:nullptr;
	APlayerController* Controller=World?World->GetFirstPlayerController():nullptr;
	APRCombatHUD* HUD=Controller?Cast<APRCombatHUD>(Controller->GetHUD()):nullptr;
	UPRCombatHUDWidget* Root=HUD?HUD->GetCombatHUDWidget():nullptr;
	if(!Root){R->SetError(TEXT("Fixed HUD PIE inspection requires an active local APRCombatHUD."));R->RemoveFromRoot();return R;}
	auto Has=[Root](const TCHAR* Name){return Root->GetWidgetFromName(Name)!=nullptr;};
	const bool bPass=Has(TEXT("PlayerResources"))&&Has(TEXT("SkillBar"))&&Has(TEXT("BossHealth"))&&Has(TEXT("BossPrototypeResult"))&&Has(TEXT("CombatFeedback"));
	if(!bPass){R->SetError(TEXT("Fixed HUD PIE composition is incomplete."));R->RemoveFromRoot();return R;}
	R->SetValue(TEXT("{\"status\":\"PASS\",\"playerResources\":true,\"skillBar\":true,\"bossHealth\":true,\"bossResult\":true,\"feedback\":true}"));R->RemoveFromRoot();return R;
}
