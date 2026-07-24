// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDivergenceAuthoringToolset.h"

#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Divergence/PRDivergenceDataAsset.h"
#include "Divergence/PRDivergenceTypes.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRDivergenceCacheWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace PRDivergenceAuthoring
{
constexpr const TCHAR* DataPath = TEXT("/Game/ProjectR/Data/Divergence/DA_DivergenceCache");
constexpr const TCHAR* WidgetPath = TEXT("/Game/ProjectR/UI/Companion/WBP_DivergenceCache");
const TCHAR* Manifest[] = { DataPath, WidgetPath };

bool Exists(const TCHAR* Path) { return FPackageName::DoesPackageExist(Path); }

UPRDivergenceDataAsset* LoadData()
{
	return LoadObject<UPRDivergenceDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Divergence/DA_DivergenceCache.DA_DivergenceCache"));
}

UWidgetBlueprint* LoadWidget()
{
	return LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/ProjectR/UI/Companion/WBP_DivergenceCache.WBP_DivergenceCache"));
}

void ConfigureData(UPRDivergenceDataAsset& Asset, UWidgetBlueprint& Widget)
{
	FPRDivergenceContract::ConfigureFixedDefinition(Asset);
	Asset.WidgetClass = Widget.GeneratedClass;
	Asset.MarkPackageDirty();
}

UWidgetBlueprint* CreateWidget()
{
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = UPRDivergenceCacheWidget::StaticClass();
	IAssetTools& Tools = FAssetToolsModule::GetModule().Get();
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Tools.CreateAsset(
		TEXT("WBP_DivergenceCache"), TEXT("/Game/ProjectR/UI/Companion"), UWidgetBlueprint::StaticClass(), Factory));
	if (!Blueprint->WidgetTree) return nullptr;
	UVerticalBox* Root = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DivergenceRoot"));
	Blueprint->WidgetTree->RootWidget = Root;
	for (const FName Name : { TEXT("SpeakerText"), TEXT("PromptText"), TEXT("RescueText"), TEXT("LeaveText"), TEXT("ChallengeText"), TEXT("InputHintText"), TEXT("CountdownText") })
	{
		Root->AddChildToVerticalBox(Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name));
	}
	FAssetRegistryModule::AssetCreated(Blueprint);
	Blueprint->MarkPackageDirty();
	return Blueprint;
}

UPRDivergenceDataAsset* CreateData(UWidgetBlueprint& Widget)
{
	UPackage* Package = CreatePackage(DataPath);
	UPRDivergenceDataAsset* Asset = NewObject<UPRDivergenceDataAsset>(Package, TEXT("DA_DivergenceCache"), RF_Public | RF_Standalone);
	if (!Asset) return nullptr;
	FAssetRegistryModule::AssetCreated(Asset);
	ConfigureData(*Asset, Widget);
	return Asset;
}

bool ValidateWidget(const UWidgetBlueprint& Widget, FString& Error)
{
	if (!Widget.ParentClass || !Widget.ParentClass->IsChildOf(UPRDivergenceCacheWidget::StaticClass()) || !Widget.WidgetTree)
	{
		Error = TEXT("Divergence widget parent or WidgetTree differs from the v0.3.4 contract.");
		return false;
	}
	for (const FName Name : { TEXT("SpeakerText"), TEXT("PromptText"), TEXT("RescueText"), TEXT("LeaveText"), TEXT("ChallengeText"), TEXT("InputHintText"), TEXT("CountdownText") })
	{
		if (!Widget.WidgetTree->FindWidget(Name))
		{
			Error = FString::Printf(TEXT("Divergence widget lacks required TextBlock %s."), *Name.ToString());
			return false;
		}
	}
	return true;
}

bool OnlyManifestDirty(FString& Error)
{
	TSet<FString> Allowed;
	for (const TCHAR* Path : Manifest) Allowed.Add(Path);
	TArray<UPackage*> Dirty;
	FEditorFileUtils::GetDirtyContentPackages(Dirty);
	for (UPackage* Package : Dirty)
	{
		if (Package && !Allowed.Contains(Package->GetName()))
		{
			Error = FString::Printf(TEXT("Unexpected dirty package: %s"), *Package->GetName());
			return false;
		}
	}
	return true;
}

bool SaveExact(const TCHAR* Path, FString& Error)
{
	UObject* Object = LoadObject<UObject>(nullptr, *FString::Printf(TEXT("%s.%s"), Path, *FPackageName::GetLongPackageAssetName(Path)));
	if (!Object) { Error = FString::Printf(TEXT("Cannot load manifest package: %s"), Path); return false; }
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.Error = GError;
	return UPackage::SavePackage(Object->GetOutermost(), Object, *FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args);
}
}

UToolCallAsyncResultString* UPRDivergenceAuthoringToolset::CreateV034DivergenceManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	for (const TCHAR* Path : PRDivergenceAuthoring::Manifest)
	{
		if (PRDivergenceAuthoring::Exists(Path)) { Result->SetError(FString::Printf(TEXT("v0.3.4 manifest collision: %s"), Path)); return Result; }
	}
	UWidgetBlueprint* Widget = PRDivergenceAuthoring::CreateWidget();
	UPRDivergenceDataAsset* Data = Widget ? PRDivergenceAuthoring::CreateData(*Widget) : nullptr;
	if (!Widget || !Data) { Result->SetError(TEXT("v0.3.4 manifest creation failed; no save was attempted.")); return Result; }
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":2,\"saved\":false,\"mapsSaved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRDivergenceAuthoringToolset::RepairV034DivergenceManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UPRDivergenceDataAsset* Data = PRDivergenceAuthoring::LoadData();
	UWidgetBlueprint* Widget = PRDivergenceAuthoring::LoadWidget();
	if (!Data || !Widget) { Result->SetError(TEXT("v0.3.4 repair requires both exact existing manifest packages.")); return Result; }
	PRDivergenceAuthoring::ConfigureData(*Data, *Widget);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Widget);
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"repaired\":1,\"mapsSaved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRDivergenceAuthoringToolset::ValidateV034DivergenceManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UPRDivergenceDataAsset* Data = PRDivergenceAuthoring::LoadData();
	UWidgetBlueprint* Widget = PRDivergenceAuthoring::LoadWidget();
	FString Error;
	if (!Data || !Widget || !Data->ValidateDefinition(Error) || !PRDivergenceAuthoring::ValidateWidget(*Widget, Error))
	{
		Result->SetError(FString::Printf(TEXT("v0.3.4 manifest validation failed: %s"), *Error));
		return Result;
	}
	if (Data->WidgetClass.LoadSynchronous() != Widget->GeneratedClass)
	{
		Result->SetError(TEXT("Divergence DataAsset WidgetClass does not refer to WBP_DivergenceCache."));
		return Result;
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"dataAsset\":true,\"widget\":true}"));
	return Result;
}

UToolCallAsyncResultString* UPRDivergenceAuthoringToolset::SaveV034DivergenceManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	FString Error;
	if (!PRDivergenceAuthoring::OnlyManifestDirty(Error)) { Result->SetError(Error); return Result; }
	int32 Saved = 0;
	for (const TCHAR* Path : PRDivergenceAuthoring::Manifest)
	{
		UObject* Object = LoadObject<UObject>(nullptr, *FString::Printf(TEXT("%s.%s"), Path, *FPackageName::GetLongPackageAssetName(Path)));
		if (!Object) { Result->SetError(FString::Printf(TEXT("Cannot load manifest package: %s"), Path)); return Result; }
		if (Object->GetOutermost()->IsDirty())
		{
			if (!PRDivergenceAuthoring::SaveExact(Path, Error)) { Result->SetError(Error); return Result; }
			++Saved;
		}
	}
	Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"saved\":%d,\"mapsSaved\":false}"), Saved));
	return Result;
}
