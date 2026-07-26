// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRDirectorRuleAuthoringToolset.h"

#include "Abilities/PRAttributeSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/PRTagLibrary.h"
#include "Director/PRDirectorRuleDataAsset.h"
#include "Director/PRDirectorRuleEffectTypes.h"
#include "Director/PRDirectorRuleRegistryDataAsset.h"
#include "Engine/Blueprint.h"
#include "GameplayEffect.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/PRDirectorRulePanelWidget.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/Package.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/UserWidget.h"
#include "AssetToolsModule.h"

namespace PRDirectorRuleAuthoring
{
struct FScopedToolResultRoot
{
	explicit FScopedToolResultRoot(UToolCallAsyncResultString* InResult)
		: Result(InResult)
	{
		check(Result);
		Result->AddToRoot();
	}

	~FScopedToolResultRoot()
	{
		Result->RemoveFromRoot();
	}

	UToolCallAsyncResultString* Result;
};

struct FRuleDefinition
{
	const TCHAR* PackagePath;
	const TCHAR* RuleName;
	const TCHAR* DisplayName;
	const TCHAR* VisibleReason;
	const TCHAR* CounterDescription;
	bool bExisting;
};

constexpr FRuleDefinition Rules[] =
{
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_CompanionIsolation"), TEXT("Rule.CompanionIsolation"), TEXT("队友隔离"), TEXT("分歧使支援变得不稳定。"), TEXT("连续完成两次 QTE 可降低。"), false },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_CooperationAudit"), TEXT("Rule.CooperationAudit"), TEXT("协作审计"), TEXT("QTE 未通过使敌人获得护盾。"), TEXT("连续两次 QTE 成功可降低。"), true },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_DeleteEcho"), TEXT("Rule.DeleteEcho"), TEXT("删除回声"), TEXT("死亡回声使敌人暂时更危险。"), TEXT("QTE 成功可立即清除。"), false },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_DistanceCorrection"), TEXT("Rule.DistanceCorrection"), TEXT("距离校正"), TEXT("过远输出触发距离校正。"), TEXT("近距离击杀可降低。"), true },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_EmotionalInterference"), TEXT("Rule.EmotionalInterference"), TEXT("情绪干扰"), TEXT("低关系使支援间隔延长。"), TEXT("提升关系或完成 QTE 可降低。"), false },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_ObedienceTest"), TEXT("Rule.ObedienceTest"), TEXT("服从测试"), TEXT("以生命与资源换取输出。"), TEXT("拒绝一次 QTE 可降低。"), false },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_OptimalPath"), TEXT("Rule.OptimalPath"), TEXT("最优路径"), TEXT("连续安全远程输出使敌人加速。"), TEXT("近距离击杀可清除。"), false },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_PredictionLock"), TEXT("Rule.PredictionLock"), TEXT("预测锁定"), TEXT("重复使用预测技能将被抑制。"), TEXT("使用三种不同技能或 QTE 成功可降低。"), false },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_RepetitionPenalty"), TEXT("Rule.RepetitionPenalty"), TEXT("重复惩戒"), TEXT("重复技能输出受到抑制。"), TEXT("切换技能或 QTE 成功可解除。"), true },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_ResourceBalance"), TEXT("Rule.ResourceBalance"), TEXT("资源均衡"), TEXT("资源过高时最大能量下调。"), TEXT("消耗能量可降低。"), false },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_RiskReward"), TEXT("Rule.RiskReward"), TEXT("风险奖赏"), TEXT("以最大生命换取输出。"), TEXT("连续击杀且不死可降低。"), false },
	{ TEXT("/Game/ProjectR/Data/Director/Rules/DA_DirectorRule_SurvivalProtocol"), TEXT("Rule.SurvivalProtocol"), TEXT("存续协议"), TEXT("低生命时输出提高。"), TEXT("恢复安全生命或 QTE 成功可暂停。"), true }
};

struct FEffectDefinition
{
	const TCHAR* PackagePath;
	FGameplayAttribute Attribute;
	EGameplayModOp::Type ModifierOp;
};

FGameplayTag MagnitudeTag()
{
	return FGameplayTag::RequestGameplayTag(FName(TEXT("Rule.SurvivalProtocol")), false);
}

bool ConfigureRule(UPRDirectorRuleDataAsset* Rule, const FRuleDefinition& Definition)
{
	if (!Rule) return false;
	Rule->Modify();
	Rule->RuleId = FGameplayTag::RequestGameplayTag(FName(Definition.RuleName), false);
	Rule->MaximumLevel = 3;
	Rule->DisplayName = FText::FromString(Definition.DisplayName);
	Rule->DefaultVisibleReason = FText::FromString(Definition.VisibleReason);
	Rule->CounterDescription = FText::FromString(Definition.CounterDescription);
	Rule->MarkPackageDirty();
	return Rule->IsRuleDefinitionValid();
}

UBlueprint* CreateGameplayEffectBlueprint(const TCHAR* PackagePath)
{
	const FString LongName(PackagePath);
	UPackage* Package = CreatePackage(*LongName);
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(UGameplayEffect::StaticClass(), Package,
		FName(FPackageName::GetLongPackageAssetName(LongName)), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	if (Blueprint) FAssetRegistryModule::AssetCreated(Blueprint);
	return Blueprint;
}

UWidgetBlueprint* CreateRulePanelBlueprint()
{
	const TCHAR* PackagePath = TEXT("/Game/ProjectR/UI/Director/WBP_DirectorRulePanel");
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = UPRDirectorRulePanelWidget::StaticClass();
	return Cast<UWidgetBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(
		FPackageName::GetLongPackageAssetName(PackagePath), FPackageName::GetLongPackagePath(PackagePath), UWidgetBlueprint::StaticClass(), Factory));
}

bool ConfigureRulePanel(UWidgetBlueprint* Panel)
{
	if (!Panel || !Panel->WidgetTree) return false;
	UVerticalBox* Root = Panel->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DirectorRuleRoot"));
	Panel->WidgetTree->RootWidget = Root;
	for (const TCHAR* Name : { TEXT("RuleNameText"), TEXT("RuleReasonText"), TEXT("RuleEffectText"), TEXT("RuleCounterText") })
	{
		Root->AddChildToVerticalBox(Panel->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name));
	}
	Panel->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(Panel);
	return Panel->GeneratedClass != nullptr;
}

bool AttachRulePanelToCombatHUD(UWidgetBlueprint* Panel)
{
	UWidgetBlueprint* CombatHUD = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/ProjectR/UI/Combat/WBP_CombatHUD.WBP_CombatHUD"));
	if (!CombatHUD || !CombatHUD->WidgetTree || !Panel || !Panel->GeneratedClass) return false;
	UVerticalBox* Root = Cast<UVerticalBox>(CombatHUD->WidgetTree->RootWidget);
	if (!Root || CombatHUD->WidgetTree->FindWidget(TEXT("DirectorRulePanel"))) return false;
	UUserWidget* PanelWidget = CombatHUD->WidgetTree->ConstructWidget<UUserWidget>(TSubclassOf<UUserWidget>(Panel->GeneratedClass), TEXT("DirectorRulePanel"));
	if (!PanelWidget) return false;
	Root->AddChildToVerticalBox(PanelWidget);
	CombatHUD->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(CombatHUD);
	return CombatHUD->GeneratedClass != nullptr;
}

bool ConfigureEffect(UBlueprint* Blueprint, const FGameplayAttribute Attribute, const EGameplayModOp::Type ModifierOp)
{
	UGameplayEffect* Effect = Blueprint && Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	if (!Effect) return false;
	Effect->Modify();
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	Effect->Period = FScalableFloat(0.0f);
	Effect->Modifiers.Reset();
	Effect->Executions.Reset();
	Effect->GameplayCues.Reset();
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = ModifierOp;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = MagnitudeTag();
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Blueprint->MarkPackageDirty();
	return true;
}

bool IsPackageOccupied(const TCHAR* PackagePath)
{
	return FindObject<UObject>(nullptr, PackagePath) != nullptr;
}
}

UToolCallAsyncResultString* UPRDirectorRuleAuthoringToolset::CreateAndConfigureFixedDirectorRuleManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	PRDirectorRuleAuthoring::FScopedToolResultRoot Root(Result);
	using namespace PRDirectorRuleAuthoring;
	for (const FRuleDefinition& Rule : Rules)
	{
		if (!Rule.bExisting && IsPackageOccupied(Rule.PackagePath))
		{
			Result->SetError(FString::Printf(TEXT("Director Rule create collision: %s"), Rule.PackagePath));
			return Result;
		}
	}
	const TCHAR* Effects[] = {
		TEXT("/Game/ProjectR/Effects/Director/GE_Director_PlayerAttackPower"),
		TEXT("/Game/ProjectR/Effects/Director/GE_Director_PlayerMaxHealth"),
		TEXT("/Game/ProjectR/Effects/Director/GE_Director_PlayerMaxEnergy"),
		TEXT("/Game/ProjectR/Effects/Director/GE_Director_EnemyAttackPower"),
		TEXT("/Game/ProjectR/Effects/Director/GE_Director_EnemyMoveSpeed"),
		TEXT("/Game/ProjectR/Effects/Director/GE_Director_EnemyArmor") };
	for (const TCHAR* Effect : Effects)
	{
		if (IsPackageOccupied(Effect)) { Result->SetError(FString::Printf(TEXT("Director Effect create collision: %s"), Effect)); return Result; }
	}
	if (IsPackageOccupied(TEXT("/Game/ProjectR/UI/Director/WBP_DirectorRulePanel")))
	{
		Result->SetError(TEXT("Director Rule panel create collision."));
		return Result;
	}

	TArray<TObjectPtr<UPRDirectorRuleDataAsset>> OrderedRules;
	for (const FRuleDefinition& Definition : Rules)
	{
		UPRDirectorRuleDataAsset* Rule = Definition.bExisting
			? LoadObject<UPRDirectorRuleDataAsset>(nullptr, *FString::Printf(TEXT("%s.%s"), Definition.PackagePath, *FPackageName::GetLongPackageAssetName(Definition.PackagePath)))
			: NewObject<UPRDirectorRuleDataAsset>(CreatePackage(Definition.PackagePath), FName(FPackageName::GetLongPackageAssetName(Definition.PackagePath)), RF_Public | RF_Standalone);
		if (!Rule || !ConfigureRule(Rule, Definition)) { Result->SetError(TEXT("Director Rule manifest configuration failed.")); return Result; }
		if (!Definition.bExisting) FAssetRegistryModule::AssetCreated(Rule);
		OrderedRules.Add(Rule);
	}
	UPRDirectorRuleRegistryDataAsset* Registry = LoadObject<UPRDirectorRuleRegistryDataAsset>(nullptr, TEXT("/Game/ProjectR/Data/Director/DA_DirectorRuleRegistry.DA_DirectorRuleRegistry"));
	if (!Registry) { Result->SetError(TEXT("Director Rule Registry is unavailable.")); return Result; }
	Registry->Modify(); Registry->Rules.Reset();
	for (UPRDirectorRuleDataAsset* Rule : OrderedRules) Registry->Rules.Add(Rule);
	Registry->MarkPackageDirty();

	const FEffectDefinition Definitions[] = {
		{ Effects[0], UPRAttributeSet::GetAttackPowerAttribute(), EGameplayModOp::Multiplicitive },
		{ Effects[1], UPRAttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Multiplicitive },
		{ Effects[2], UPRAttributeSet::GetMaxEnergyAttribute(), EGameplayModOp::Multiplicitive },
		{ Effects[3], UPRAttributeSet::GetAttackPowerAttribute(), EGameplayModOp::Multiplicitive },
		{ Effects[4], UPRAttributeSet::GetMoveSpeedAttribute(), EGameplayModOp::Multiplicitive },
		{ Effects[5], UPRAttributeSet::GetShieldAttribute(), EGameplayModOp::Additive } };
	for (const FEffectDefinition& Definition : Definitions)
	{
		UBlueprint* Effect = CreateGameplayEffectBlueprint(Definition.PackagePath);
		if (!ConfigureEffect(Effect, Definition.Attribute, Definition.ModifierOp)) { Result->SetError(TEXT("Director Effect manifest configuration failed.")); return Result; }
	}
	UWidgetBlueprint* Panel = CreateRulePanelBlueprint();
	if (!ConfigureRulePanel(Panel)) { Result->SetError(TEXT("Director Rule panel manifest configuration failed.")); return Result; }
	if (!AttachRulePanelToCombatHUD(Panel)) { Result->SetError(TEXT("Director Rule panel could not be attached to the fixed CombatHUD.")); return Result; }
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":15,\"modified\":6,\"saved\":false}"));
	return Result;
}
