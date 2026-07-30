// Copyright ProjectR. All Rights Reserved.

#include "PRAuditorChapterAuthoringToolset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Chapters/Auditor/PRAuditorChapterBoss.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRAuditorChapterWidget.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace PRAuditorChapterAuthoring
{
	constexpr TCHAR BossPath[] = TEXT("/Game/ProjectR/Chapters/Auditor/Boss/BP_Boss_AuditorChapter");
	constexpr TCHAR OverlayPath[] = TEXT("/Game/ProjectR/Chapters/Auditor/UI/WBP_AuditorChapterOverlay");

	bool DoesPackageExist(const TCHAR* Path)
	{
		return FPackageName::DoesPackageExist(Path)
			|| FindObject<UObject>(nullptr, Path) != nullptr;
	}
}

UToolCallAsyncResultString* UPRAuditorChapterAuthoringToolset::CreateV070AuditorBlueprintManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->AddToRoot();
	auto Finish = [Result](const FString& Value, const bool bError)
	{
		if (bError) Result->SetError(Value); else Result->SetValue(Value);
		Result->RemoveFromRoot();
		return Result;
	};

	using namespace PRAuditorChapterAuthoring;
	if (GEditor && GEditor->PlayWorld)
	{
		return Finish(TEXT("v0.7.0 Auditor Blueprint creation is unavailable while PIE is active."), true);
	}
	if (DoesPackageExist(BossPath) || DoesPackageExist(OverlayPath))
	{
		return Finish(TEXT("v0.7.0 Auditor Blueprint manifest collision; no package was created."), true);
	}

	FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
	UBlueprintFactory* BossFactory = NewObject<UBlueprintFactory>();
	BossFactory->ParentClass = APRAuditorChapterBoss::StaticClass();
	UBlueprint* BossBlueprint = Cast<UBlueprint>(AssetToolsModule.Get().CreateAsset(
		FPackageName::GetLongPackageAssetName(BossPath),
		FPackageName::GetLongPackagePath(BossPath),
		UBlueprint::StaticClass(),
		BossFactory));
	if (!BossBlueprint)
	{
		return Finish(TEXT("v0.7.0 Auditor Boss Blueprint creation failed."), true);
	}

	UWidgetBlueprintFactory* OverlayFactory = NewObject<UWidgetBlueprintFactory>();
	OverlayFactory->ParentClass = UPRAuditorChapterWidget::StaticClass();
	UWidgetBlueprint* OverlayBlueprint = Cast<UWidgetBlueprint>(AssetToolsModule.Get().CreateAsset(
		FPackageName::GetLongPackageAssetName(OverlayPath),
		FPackageName::GetLongPackagePath(OverlayPath),
		UWidgetBlueprint::StaticClass(),
		OverlayFactory));
	if (!OverlayBlueprint || !OverlayBlueprint->WidgetTree)
	{
		return Finish(TEXT("v0.7.0 Auditor Overlay Widget creation failed."), true);
	}

	FKismetEditorUtilities::CompileBlueprint(BossBlueprint);
	FKismetEditorUtilities::CompileBlueprint(OverlayBlueprint);
	BossBlueprint->MarkPackageDirty();
	OverlayBlueprint->MarkPackageDirty();
	return Finish(TEXT("{\"status\":\"PASS\",\"created\":[\"/Game/ProjectR/Chapters/Auditor/Boss/BP_Boss_AuditorChapter\",\"/Game/ProjectR/Chapters/Auditor/UI/WBP_AuditorChapterOverlay\"],\"saved\":false}"), false);
}
