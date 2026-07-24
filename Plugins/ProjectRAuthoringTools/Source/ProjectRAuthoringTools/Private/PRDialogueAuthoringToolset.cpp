// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDialogueAuthoringToolset.h"

#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PRRelationshipTypes.h"
#include "Dialogue/PRCompanionDialogueDataAsset.h"
#include "Dialogue/PRCompanionDialogueRegistryDataAsset.h"
#include "Dialogue/PRDialogueTypes.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRCompanionDialogueWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace PRDialogueAuthoring
{
constexpr const TCHAR* RegistryPath = TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogueRegistry");
constexpr const TCHAR* AxiomPath = TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogue_Axiom");
constexpr const TCHAR* KindlePath = TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogue_Kindle");
constexpr const TCHAR* NullPath = TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogue_Null");
constexpr const TCHAR* WidgetPath = TEXT("/Game/ProjectR/UI/Dialogue/WBP_CompanionDialogue");
const TCHAR* Manifest[] = { RegistryPath, AxiomPath, KindlePath, NullPath, WidgetPath };

struct FLineText { const TCHAR* Id; const TCHAR* Text; };

const TArray<EPRDialogueTriggerKind>& Triggers()
{
	static const TArray<EPRDialogueTriggerKind> Values = {
		EPRDialogueTriggerKind::CriticalLowHealth, EPRDialogueTriggerKind::LowHealth, EPRDialogueTriggerKind::QTESuccess,
		EPRDialogueTriggerKind::QTEFailure, EPRDialogueTriggerKind::SupportApplied, EPRDialogueTriggerKind::SupportRejected,
		EPRDialogueTriggerKind::BossPhaseChanged, EPRDialogueTriggerKind::PredictionBlocked, EPRDialogueTriggerKind::SafeCombatCleared };
	return Values;
}

const TArray<FLineText>& TextsFor(const FGameplayTag Companion)
{
	static const TArray<FLineText> Axiom = {{TEXT("CriticalLowHealth"),TEXT("生命曲线临界，立即脱离。")},{TEXT("LowHealth"),TEXT("生命资源偏低，降低交换频率。")},{TEXT("QTESuccess"),TEXT("协作窗口闭合，结果符合预期。")},{TEXT("QTEFailure"),TEXT("协作窗口失效，重置节奏。")},{TEXT("SupportApplied"),TEXT("支援已生效。")},{TEXT("SupportRejected"),TEXT("支援条件不成立。")},{TEXT("BossPhaseChanged"),TEXT("审计阶段变化，重新评估攻击序列。")},{TEXT("PredictionBlocked"),TEXT("当前模式已被预测，切换技能。")},{TEXT("SafeCombatCleared"),TEXT("区域稳定，开始复盘。")}};
	static const TArray<FLineText> Kindle = {{TEXT("CriticalLowHealth"),TEXT("别硬撑，先拉开！")},{TEXT("LowHealth"),TEXT("血量不妙，别贪刀！")},{TEXT("QTESuccess"),TEXT("漂亮！就是这个节奏！")},{TEXT("QTEFailure"),TEXT("没接上？下一次抢回来！")},{TEXT("SupportApplied"),TEXT("接住，继续上！")},{TEXT("SupportRejected"),TEXT("没接上，重来！")},{TEXT("BossPhaseChanged"),TEXT("它换阶段了，继续压上！")},{TEXT("PredictionBlocked"),TEXT("它读懂你了，换招砸开！")},{TEXT("SafeCombatCleared"),TEXT("清完了，来复盘！")}};
	static const TArray<FLineText> Null = {{TEXT("CriticalLowHealth"),TEXT("再挨一下，我就替你写遗言。")},{TEXT("LowHealth"),TEXT("你的血条比计划还短。")},{TEXT("QTESuccess"),TEXT("行吧，这次配合得像回事。")},{TEXT("QTEFailure"),TEXT("错过了，别假装那是战术。")},{TEXT("SupportApplied"),TEXT("我只是顺手。")},{TEXT("SupportRejected"),TEXT("连系统都不配合你。")},{TEXT("BossPhaseChanged"),TEXT("看吧，它开始改规则了。")},{TEXT("PredictionBlocked"),TEXT("重复答案被判错，换一道题。")},{TEXT("SafeCombatCleared"),TEXT("终于安静，勉强聊两句。")}};
	if (Companion == FPRCompanionContract::AxiomTag()) return Axiom;
	if (Companion == FPRCompanionContract::KindleTag()) return Kindle;
	return Null;
}

const TCHAR* ChoiceTextFor(const FGameplayTag Companion, const bool bAnalyze)
{
	if (Companion == FPRCompanionContract::AxiomTag()) return bAnalyze ? TEXT("分析下一次风险") : TEXT("认可这次协作");
	if (Companion == FPRCompanionContract::KindleTag()) return bAnalyze ? TEXT("说说哪里还能更猛") : TEXT("夸夸这次配合");
	return bAnalyze ? TEXT("挑出这次的问题") : TEXT("承认你有点用");
}

bool ValidateFixedDialogueAsset(const UPRCompanionDialogueDataAsset& Asset, FString& Error)
{
	const TArray<FLineText>& ExpectedLines = TextsFor(Asset.CompanionId);
	if (Asset.Lines.Num() != ExpectedLines.Num()) { Error = TEXT("Dialogue line count differs from the fixed v0.3.3 contract."); return false; }
	for (int32 Index = 0; Index < ExpectedLines.Num(); ++Index)
	{
		if (Asset.Lines[Index].LineId != FName(ExpectedLines[Index].Id) || Asset.Lines[Index].Text.ToString() != ExpectedLines[Index].Text)
		{
			Error = FString::Printf(TEXT("Dialogue line %d does not match the fixed companion voice contract."), Index);
			return false;
		}
	}
	if (Asset.SafeChoices.Num() != 2
		|| Asset.SafeChoices[0].Text.ToString() != ChoiceTextFor(Asset.CompanionId, false)
		|| Asset.SafeChoices[1].Text.ToString() != ChoiceTextFor(Asset.CompanionId, true))
	{
		Error = TEXT("Dialogue safe-choice text does not match the fixed companion voice contract.");
		return false;
	}
	return true;
}

bool Exists(const TCHAR* Path) { return FPackageName::DoesPackageExist(Path); }

void ConfigureDialogueAsset(UPRCompanionDialogueDataAsset& Asset, const FGameplayTag Companion, const TCHAR* Speaker)
{
	Asset.CompanionId = Companion;
	Asset.SpeakerName = FText::FromString(Speaker);
	Asset.Lines.Reset();
	Asset.SafeChoices.Reset();
	const TArray<FLineText>& Texts = TextsFor(Companion);
	const TArray<EPRDialogueTriggerKind>& TriggerValues = Triggers();
	for (int32 Index = 0; Index < TriggerValues.Num(); ++Index)
	{
		FPRDialogueLineDefinition Line;
		Line.LineId = FName(Texts[Index].Id);
		Line.TriggerKind = TriggerValues[Index];
		Line.Priority = FPRDialogueContract::GetTriggerPriority(Line.TriggerKind);
		Line.CooldownSeconds = FPRDialogueContract::GetTriggerCooldown(Line.TriggerKind);
		Line.DisplayDurationSeconds = 2.5f;
		Line.Text = FText::FromString(Texts[Index].Text);
		Asset.Lines.Add(Line);
	}
	for (const bool bAnalyze : { false, true })
	{
		FPRDialogueChoiceDefinition Choice;
		Choice.ChoiceId = bAnalyze ? TEXT("Analyze") : TEXT("Acknowledge");
		Choice.Text = FText::FromString(ChoiceTextFor(Companion, bAnalyze));
		Choice.RelationshipDelta.CompanionId = Companion;
		Choice.RelationshipDelta.SourceId = bAnalyze ? TEXT("Dialogue.Analyze") : TEXT("Dialogue.Acknowledge");
		Choice.RelationshipDelta.TrustDelta = 1;
		Choice.RelationshipDelta.AffectionDelta = bAnalyze ? 0 : 1;
		Choice.RelationshipDelta.EvaluationDelta = bAnalyze ? 1 : 0;
		Asset.SafeChoices.Add(Choice);
	}
	Asset.MarkPackageDirty();
}

UPRCompanionDialogueDataAsset* CreateDialogueAsset(const TCHAR* Path, const FGameplayTag Companion, const TCHAR* Speaker)
{
	UPackage* Package = CreatePackage(Path);
	UPRCompanionDialogueDataAsset* Asset = NewObject<UPRCompanionDialogueDataAsset>(Package, FName(FPackageName::GetLongPackageAssetName(Path)), RF_Public | RF_Standalone);
	FAssetRegistryModule::AssetCreated(Asset);
	if (!Asset) return nullptr;
	ConfigureDialogueAsset(*Asset, Companion, Speaker);
	return Asset;
}

UWidgetBlueprint* CreateWidget()
{
	IAssetTools& Tools = FAssetToolsModule::GetModule().Get();
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = UPRCompanionDialogueWidget::StaticClass();
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Tools.CreateAsset(TEXT("WBP_CompanionDialogue"), TEXT("/Game/ProjectR/UI/Dialogue"), UWidgetBlueprint::StaticClass(), Factory));
	if (!Blueprint || !Blueprint->WidgetTree) return nullptr;
	UVerticalBox* Root = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueRoot"));
	Blueprint->WidgetTree->RootWidget = Root;
	for (const FName Name : { TEXT("SpeakerText"), TEXT("DialogueText"), TEXT("ChoiceAcknowledgeText"), TEXT("ChoiceAnalyzeText"), TEXT("ChoiceHintText") })
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Root->AddChildToVerticalBox(Text);
	}
	Blueprint->MarkPackageDirty();
	return Blueprint;
}

bool OnlyManifestDirty(FString& Error)
{
	TSet<FString> Allowed;
	for (const TCHAR* Path : Manifest) Allowed.Add(Path);
	TArray<UPackage*> Dirty;
	FEditorFileUtils::GetDirtyContentPackages(Dirty);
	for (UPackage* Package : Dirty) if (Package && !Allowed.Contains(Package->GetName())) { Error = FString::Printf(TEXT("Unexpected dirty package: %s"), *Package->GetName()); return false; }
	return true;
}

bool SaveExact(const TCHAR* Path, FString& Error)
{
	UObject* Object = LoadObject<UObject>(nullptr, *FString::Printf(TEXT("%s.%s"), Path, *FPackageName::GetLongPackageAssetName(Path)));
	if (!Object) { Error = FString::Printf(TEXT("Cannot load fixed manifest package: %s"), Path); return false; }
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.Error = GError;
	return UPackage::SavePackage(Object->GetOutermost(), Object, *FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args);
}
}

UToolCallAsyncResultString* UPRDialogueAuthoringToolset::CreateV033DialogueManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	for (const TCHAR* Path : PRDialogueAuthoring::Manifest)
	{
		if (PRDialogueAuthoring::Exists(Path)) { Result->SetError(FString::Printf(TEXT("v0.3.3 manifest collision: %s"), Path)); return Result; }
	}
	UPRCompanionDialogueDataAsset* Axiom = PRDialogueAuthoring::CreateDialogueAsset(PRDialogueAuthoring::AxiomPath, FPRCompanionContract::AxiomTag(), TEXT("Axiom"));
	UPRCompanionDialogueDataAsset* Kindle = PRDialogueAuthoring::CreateDialogueAsset(PRDialogueAuthoring::KindlePath, FPRCompanionContract::KindleTag(), TEXT("Kindle"));
	UPRCompanionDialogueDataAsset* Null = PRDialogueAuthoring::CreateDialogueAsset(PRDialogueAuthoring::NullPath, FPRCompanionContract::NullTag(), TEXT("Null"));
	UWidgetBlueprint* Widget = PRDialogueAuthoring::CreateWidget();
	if (!Axiom || !Kindle || !Null || !Widget) { Result->SetError(TEXT("v0.3.3 fixed manifest creation failed; no save was attempted.")); return Result; }
	UPackage* RegistryPackage = CreatePackage(PRDialogueAuthoring::RegistryPath);
	UPRCompanionDialogueRegistryDataAsset* Registry = NewObject<UPRCompanionDialogueRegistryDataAsset>(RegistryPackage, TEXT("DA_CompanionDialogueRegistry"), RF_Public | RF_Standalone);
	FAssetRegistryModule::AssetCreated(Registry);
	Registry->Axiom = Axiom;
	Registry->Kindle = Kindle;
	Registry->Null = Null;
	Registry->WidgetClass = Widget->GeneratedClass;
	Registry->MarkPackageDirty();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":5,\"saved\":false,\"mapsSaved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRDialogueAuthoringToolset::RepairV033DialogueManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UPRCompanionDialogueDataAsset* Axiom = LoadObject<UPRCompanionDialogueDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogue_Axiom.DA_CompanionDialogue_Axiom"));
	UPRCompanionDialogueDataAsset* Kindle = LoadObject<UPRCompanionDialogueDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogue_Kindle.DA_CompanionDialogue_Kindle"));
	UPRCompanionDialogueDataAsset* Null = LoadObject<UPRCompanionDialogueDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogue_Null.DA_CompanionDialogue_Null"));
	if (!Axiom || !Kindle || !Null)
	{
		Result->SetError(TEXT("v0.3.3 repair requires the exact existing three dialogue DataAssets."));
		return Result;
	}
	PRDialogueAuthoring::ConfigureDialogueAsset(*Axiom, FPRCompanionContract::AxiomTag(), TEXT("Axiom"));
	PRDialogueAuthoring::ConfigureDialogueAsset(*Kindle, FPRCompanionContract::KindleTag(), TEXT("Kindle"));
	PRDialogueAuthoring::ConfigureDialogueAsset(*Null, FPRCompanionContract::NullTag(), TEXT("Null"));
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"repaired\":3,\"mapsSaved\":false}"));
	return Result;
}

UToolCallAsyncResultString* UPRDialogueAuthoringToolset::SaveV033DialogueManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	FString Error;
	if (!PRDialogueAuthoring::OnlyManifestDirty(Error)) { Result->SetError(Error); return Result; }
	int32 SavedCount = 0;
	for (const TCHAR* Path : PRDialogueAuthoring::Manifest)
	{
		UObject* Object = LoadObject<UObject>(nullptr, *FString::Printf(TEXT("%s.%s"), Path, *FPackageName::GetLongPackageAssetName(Path)));
		if (!Object) { Result->SetError(FString::Printf(TEXT("Cannot load fixed manifest package: %s"), Path)); return Result; }
		if (Object->GetOutermost()->IsDirty())
		{
			if (!PRDialogueAuthoring::SaveExact(Path, Error)) { Result->SetError(Error); return Result; }
			++SavedCount;
		}
	}
	Result->SetValue(FString::Printf(TEXT("{\"status\":\"PASS\",\"saved\":%d,\"mapsSaved\":false}"), SavedCount));
	return Result;
}

UToolCallAsyncResultString* UPRDialogueAuthoringToolset::ValidateV033DialogueManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	UPRCompanionDialogueRegistryDataAsset* Registry = LoadObject<UPRCompanionDialogueRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Dialogue/DA_CompanionDialogueRegistry.DA_CompanionDialogueRegistry"));
	FString Error;
	if (!Registry || !Registry->ValidateRegistry(Error)) { Result->SetError(FString::Printf(TEXT("Dialogue registry validation failed: %s"), *Error)); return Result; }
	for (const TSoftObjectPtr<UPRCompanionDialogueDataAsset>& Asset : {Registry->Axiom, Registry->Kindle, Registry->Null})
	{
		UPRCompanionDialogueDataAsset* Loaded = Asset.LoadSynchronous();
		if (!Loaded || !Loaded->ValidateDefinition(Error) || !PRDialogueAuthoring::ValidateFixedDialogueAsset(*Loaded, Error)) { Result->SetError(FString::Printf(TEXT("Dialogue data validation failed: %s"), *Error)); return Result; }
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"registry\":true,\"assets\":3,\"widget\":true}"));
	return Result;
}
