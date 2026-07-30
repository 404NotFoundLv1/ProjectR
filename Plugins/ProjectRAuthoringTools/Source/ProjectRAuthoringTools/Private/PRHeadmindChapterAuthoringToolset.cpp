// Copyright ProjectR. All Rights Reserved.

#include "PRHeadmindChapterAuthoringToolset.h"

#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Chapters/Headmind/PRHeadmindProjectionBoss.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "GameplayEffect.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "StateTree.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRHeadmindChapterWidget.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace PRHeadmindChapterAuthoring
{
	constexpr TCHAR BossPath[] = TEXT("/Game/ProjectR/Chapters/Headmind/Boss/BP_Boss_HeadmindProjection");
	constexpr TCHAR OverlayPath[] = TEXT("/Game/ProjectR/Chapters/Headmind/UI/WBP_HeadmindChapterOverlay");

	bool DoesPackageExist(const TCHAR* Path)
	{
		return FPackageName::DoesPackageExist(Path) || FindObject<UObject>(nullptr, Path) != nullptr;
	}
}

UToolCallAsyncResultString* UPRHeadmindChapterAuthoringToolset::CreateV071HeadmindBlueprintManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->AddToRoot();
	auto Finish = [Result](const FString& Value, const bool bError)
	{
		if (bError) Result->SetError(Value); else Result->SetValue(Value);
		Result->RemoveFromRoot();
		return Result;
	};

	using namespace PRHeadmindChapterAuthoring;
	if (GEditor && GEditor->PlayWorld) return Finish(TEXT("v0.7.1 Headmind Blueprint creation is unavailable while PIE is active."), true);
	if (DoesPackageExist(BossPath) || DoesPackageExist(OverlayPath)) return Finish(TEXT("v0.7.1 Headmind Blueprint manifest collision; no package was created."), true);

	FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
	UBlueprintFactory* BossFactory = NewObject<UBlueprintFactory>();
	BossFactory->ParentClass = APRHeadmindProjectionBoss::StaticClass();
	UBlueprint* BossBlueprint = Cast<UBlueprint>(AssetToolsModule.Get().CreateAsset(
		FPackageName::GetLongPackageAssetName(BossPath), FPackageName::GetLongPackagePath(BossPath), UBlueprint::StaticClass(), BossFactory));
	if (!BossBlueprint) return Finish(TEXT("v0.7.1 Headmind Boss Blueprint creation failed."), true);

	UWidgetBlueprintFactory* OverlayFactory = NewObject<UWidgetBlueprintFactory>();
	OverlayFactory->ParentClass = UPRHeadmindChapterWidget::StaticClass();
	UWidgetBlueprint* OverlayBlueprint = Cast<UWidgetBlueprint>(AssetToolsModule.Get().CreateAsset(
		FPackageName::GetLongPackageAssetName(OverlayPath), FPackageName::GetLongPackagePath(OverlayPath), UWidgetBlueprint::StaticClass(), OverlayFactory));
	if (!OverlayBlueprint || !OverlayBlueprint->WidgetTree) return Finish(TEXT("v0.7.1 Headmind overlay Widget creation failed."), true);

	FKismetEditorUtilities::CompileBlueprint(BossBlueprint);
	FKismetEditorUtilities::CompileBlueprint(OverlayBlueprint);
	BossBlueprint->MarkPackageDirty();
	OverlayBlueprint->MarkPackageDirty();
	return Finish(TEXT("{\"status\":\"PASS\",\"created\":[\"/Game/ProjectR/Chapters/Headmind/Boss/BP_Boss_HeadmindProjection\",\"/Game/ProjectR/Chapters/Headmind/UI/WBP_HeadmindChapterOverlay\"],\"saved\":false}"), false);
}

UToolCallAsyncResultString* UPRHeadmindChapterAuthoringToolset::ConfigureV071HeadmindBossDefaults()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->AddToRoot();
	auto Finish = [Result](const FString& Value, const bool bError)
	{
		if (bError) Result->SetError(Value); else Result->SetValue(Value);
		Result->RemoveFromRoot();
		return Result;
	};

	using namespace PRHeadmindChapterAuthoring;
	if (GEditor && GEditor->PlayWorld) return Finish(TEXT("Headmind Boss default configuration is unavailable while PIE is active."), true);
	UBlueprint* BossBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ProjectR/Chapters/Headmind/Boss/BP_Boss_HeadmindProjection.BP_Boss_HeadmindProjection"));
	UStateTree* EnemyStateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/ProjectR/Enemies/AI/ST_Enemy_Base.ST_Enemy_Base"));
	UBlueprint* DefaultAttributes = LoadObject<UBlueprint>(nullptr, TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"));
	UClass* DamageEffect = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/ProjectR/Effects/GE_Damage.GE_Damage_C"));
	if (!BossBlueprint || !BossBlueprint->GeneratedClass || !BossBlueprint->GeneratedClass->IsChildOf(APRHeadmindProjectionBoss::StaticClass())
		|| !EnemyStateTree || !DefaultAttributes || !DefaultAttributes->GeneratedClass || !DamageEffect)
	{
		return Finish(TEXT("Headmind Boss configuration rejected an unavailable or incompatible fixed dependency."), true);
	}
	FObjectProperty* StateTreeProperty = FindFProperty<FObjectProperty>(BossBlueprint->GeneratedClass, TEXT("EnemyStateTree"));
	FClassProperty* DefaultEffectProperty = FindFProperty<FClassProperty>(BossBlueprint->GeneratedClass, TEXT("DefaultAttributesEffect"));
	FClassProperty* DamageEffectProperty = FindFProperty<FClassProperty>(BossBlueprint->GeneratedClass, TEXT("DamageEffect"));
	if (!StateTreeProperty || !DefaultEffectProperty || !DamageEffectProperty)
	{
		return Finish(TEXT("Headmind Boss configuration could not find a required frozen Enemy CDO property."), true);
	}
	UObject* CDO = BossBlueprint->GeneratedClass->GetDefaultObject();
	StateTreeProperty->SetObjectPropertyValue_InContainer(CDO, EnemyStateTree);
	DefaultEffectProperty->SetPropertyValue_InContainer(CDO, DefaultAttributes->GeneratedClass);
	DamageEffectProperty->SetPropertyValue_InContainer(CDO, DamageEffect);
	BossBlueprint->MarkPackageDirty();
	return Finish(TEXT("{\"status\":\"PASS\",\"configured\":\"/Game/ProjectR/Chapters/Headmind/Boss/BP_Boss_HeadmindProjection\",\"saved\":false}"), false);
}
