// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRQTEAuthoringToolset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Companions/PRCompanionTypes.h"
#include "Core/PRTagLibrary.h"
#include "EnhancedActionKeyMapping.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "Input/PRInputConfigDataAsset.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "QTE/PRQTEDataAsset.h"
#include "QTE/PRQTERegistryDataAsset.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRQTEPromptWidget.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetToolsModule.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace PRQTEAuthoring
{
constexpr const TCHAR* RegistryPackage = TEXT("/Game/ProjectR/QTE/DA_QTERegistry");
constexpr const TCHAR* RejectActionPackage = TEXT("/Game/ProjectR/Input/Actions/IA_QTE_Reject");
constexpr const TCHAR* PromptPackage = TEXT("/Game/ProjectR/UI/QTE/WBP_QTEPrompt");
constexpr const TCHAR* InputConfigPackage = TEXT("/Game/ProjectR/Input/DA_PlayerInputConfig");
constexpr const TCHAR* InputMappingPackage = TEXT("/Game/ProjectR/Input/IMC_Player");

const TCHAR* ManifestPackages[] = {
	TEXT("/Game/ProjectR/QTE/Axiom/DA_QTE_Axiom_WeaknessAxiom"),
	TEXT("/Game/ProjectR/QTE/Axiom/DA_QTE_Axiom_ProbabilityShield"),
	TEXT("/Game/ProjectR/QTE/Axiom/DA_QTE_Axiom_PhaseCut"),
	TEXT("/Game/ProjectR/QTE/Axiom/DA_QTE_Axiom_CooperativeRefutation"),
	TEXT("/Game/ProjectR/QTE/Kindle/DA_QTE_Kindle_Breakout"),
	TEXT("/Game/ProjectR/QTE/Kindle/DA_QTE_Kindle_BurnoutTornado"),
	TEXT("/Game/ProjectR/QTE/Kindle/DA_QTE_Kindle_DetonationRelay"),
	TEXT("/Game/ProjectR/QTE/Kindle/DA_QTE_Kindle_ReverseBurnRescue"),
	TEXT("/Game/ProjectR/QTE/Null/DA_QTE_Null_VulnerabilityBackstab"),
	TEXT("/Game/ProjectR/QTE/Null/DA_QTE_Null_DecoyJudgment"),
	TEXT("/Game/ProjectR/QTE/Null/DA_QTE_Null_GarbageCollection"),
	TEXT("/Game/ProjectR/QTE/Null/DA_QTE_Null_VerdictTamper"),
	RegistryPackage, RejectActionPackage, PromptPackage, InputConfigPackage, InputMappingPackage};

struct FDefinition
{
	const TCHAR* Package;
	FName Id;
	FGameplayTag Companion;
	FGameplayTag Type;
	EPRQTETriggerSource Source;
	EPRQTETriggerKind Trigger;
	FGameplayTag InputA;
	FGameplayTag InputB;
	float Window;
	float Cooldown;
	EPRQTEEffectKind Effect;
	float Magnitude;
	float Range;
	int32 MaxTargets;
	int32 Pulses;
	float PulseInterval;
	FName FutureSample;
	int32 Trust;
	int32 Affection;
	int32 Evaluation;
	int32 Overload;
};

FGameplayTag Tag(const TCHAR* Name) { return FGameplayTag::RequestGameplayTag(FName(Name)); }

const TArray<FDefinition>& Definitions()
{
	static const TArray<FDefinition> Values = {
		{ManifestPackages[0], TEXT("Axiom_WeaknessAxiom"), FPRCompanionContract::AxiomTag(), Tag(TEXT("QTE.Type.Attack")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::ShieldBroken, UPRTagLibrary::GetInputSkillShadowThrustTag(), FGameplayTag(), 1.60f,24.f,EPRQTEEffectKind::Damage,40.f,0.f,1,0,0.f,NAME_None,1,0,2,-1},
		{ManifestPackages[1], TEXT("Axiom_ProbabilityShield"), FPRCompanionContract::AxiomTag(), Tag(TEXT("QTE.Type.Defense")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::LowHealthDamage, UPRTagLibrary::GetInputSkillCounterProofWallTag(),UPRTagLibrary::GetInputDodgeTag(),1.80f,30.f,EPRQTEEffectKind::ShieldAndInvulnerable,20.f,0.f,1,0,0.f,NAME_None,2,0,1,-2},
		{ManifestPackages[2], TEXT("Axiom_PhaseCut"), FPRCompanionContract::AxiomTag(), Tag(TEXT("QTE.Type.Control")), EPRQTETriggerSource::CompanionSupportEvent, EPRQTETriggerKind::SupportDuringAuditorWindup, UPRTagLibrary::GetInputSkillThunderDropTag(),FGameplayTag(),1.50f,30.f,EPRQTEEffectKind::Stun,.75f,0.f,1,0,0.f,NAME_None,1,0,2,-1},
		{ManifestPackages[3], TEXT("Axiom_CooperativeRefutation"), FPRCompanionContract::AxiomTag(), Tag(TEXT("QTE.Type.RuleCounter")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::PredictionBlocked, UPRTagLibrary::GetInputInteractTag(),FGameplayTag(),1.80f,45.f,EPRQTEEffectKind::FutureSample,0.f,0.f,0,0,0.f,TEXT("Future.Director.CooperationCounterSample"),2,0,2,-2},
		{ManifestPackages[4], TEXT("Kindle_Breakout"), FPRCompanionContract::KindleTag(), Tag(TEXT("QTE.Type.Control")), EPRQTETriggerSource::CompanionSupportEvent, EPRQTETriggerKind::KindleCrowdSupport, UPRTagLibrary::GetInputSkillFireSlashTag(),FGameplayTag(),1.f,22.f,EPRQTEEffectKind::Stun,.75f,260.f,3,0,0.f,NAME_None,1,2,0,1},
		{ManifestPackages[5], TEXT("Kindle_BurnoutTornado"), FPRCompanionContract::KindleTag(), Tag(TEXT("QTE.Type.Attack")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::FireSlashThreeHits, UPRTagLibrary::GetInputExecuteTag(),FGameplayTag(),.90f,30.f,EPRQTEEffectKind::PulseDamage,8.f,0.f,3,3,.25f,NAME_None,1,2,1,2},
		{ManifestPackages[6], TEXT("Kindle_DetonationRelay"), FPRCompanionContract::KindleTag(), Tag(TEXT("QTE.Type.Attack")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::ShadowThrustHit, UPRTagLibrary::GetInputAttackTag(),FGameplayTag(),.80f,18.f,EPRQTEEffectKind::Damage,35.f,0.f,1,0,0.f,NAME_None,1,2,1,1},
		{ManifestPackages[7], TEXT("Kindle_ReverseBurnRescue"), FPRCompanionContract::KindleTag(), Tag(TEXT("QTE.Type.Rescue")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::LowHealthDamage, UPRTagLibrary::GetInputSkillCounterProofWallTag(),UPRTagLibrary::GetInputDodgeTag(),.75f,45.f,EPRQTEEffectKind::ShieldAndInvulnerable,20.f,0.f,1,0,0.f,NAME_None,2,2,0,5},
		{ManifestPackages[8], TEXT("Null_VulnerabilityBackstab"), FPRCompanionContract::NullTag(), Tag(TEXT("QTE.Type.Attack")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::DecoyConsumed, UPRTagLibrary::GetInputSkillShadowThrustTag(),FGameplayTag(),1.20f,20.f,EPRQTEEffectKind::Damage,25.f,450.f,1,0,0.f,NAME_None,1,1,2,-1},
		{ManifestPackages[9], TEXT("Null_DecoyJudgment"), FPRCompanionContract::NullTag(), Tag(TEXT("QTE.Type.Control")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::PerfectDecoyConsumed, UPRTagLibrary::GetInputExecuteTag(),FGameplayTag(),1.10f,24.f,EPRQTEEffectKind::Stun,.75f,450.f,1,0,0.f,NAME_None,1,1,2,-1},
		{ManifestPackages[10], TEXT("Null_GarbageCollection"), FPRCompanionContract::NullTag(), Tag(TEXT("QTE.Type.Attack")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::NormalLowHealth, UPRTagLibrary::GetInputAttackTag(),FGameplayTag(),1.40f,18.f,EPRQTEEffectKind::ConfirmBasicAttackKill,.60f,0.f,1,0,0.f,TEXT("Future.Reward.GarbageCollected"),1,1,2,-1},
		{ManifestPackages[11], TEXT("Null_VerdictTamper"), FPRCompanionContract::NullTag(), Tag(TEXT("QTE.Type.RuleCounter")), EPRQTETriggerSource::CombatEvent, EPRQTETriggerKind::PredictionBlocked, UPRTagLibrary::GetInputInteractTag(),FGameplayTag(),1.20f,45.f,EPRQTEEffectKind::FutureSample,0.f,0.f,0,0,0.f,TEXT("Future.Director.VerdictMutationSample"),1,1,3,-2}};
	return Values;
}

bool PackageExists(const TCHAR* Path) { return FPackageName::DoesPackageExist(Path); }

bool VerifyCreatePreflight(FString& Error)
{
	for (int32 Index = 0; Index < 15; ++Index)
	{
		if (PackageExists(ManifestPackages[Index])) { Error = FString::Printf(TEXT("v0.3.2 manifest collision: %s"), ManifestPackages[Index]); return false; }
	}
	UPRInputConfigDataAsset* Config=LoadObject<UPRInputConfigDataAsset>(nullptr,TEXT("/Game/ProjectR/Input/DA_PlayerInputConfig.DA_PlayerInputConfig"));
	if (!Config) { Error=TEXT("QTE Reject input preflight could not load InputConfig."); return false; }
	const FGameplayTag RejectTag=UPRTagLibrary::GetInputQTERejectTag();
	if (Config->TaggedInputActions.ContainsByPredicate([RejectTag](const FPRTaggedInputAction& Entry) { return Entry.InputTag.MatchesTagExact(RejectTag); }))
	{
		Error=TEXT("QTE Reject input is already configured.");
		return false;
	}
	return true;
}

UPRQTEDataAsset* CreateDataAsset(const TCHAR* PackagePath)
{
	UPackage* Package=CreatePackage(PackagePath);
	UPRQTEDataAsset* Asset=NewObject<UPRQTEDataAsset>(Package,FName(FPackageName::GetLongPackageAssetName(PackagePath)),RF_Public|RF_Standalone);
	FAssetRegistryModule::AssetCreated(Asset);
	return Asset;
}

void SetDelta(FPRRelationshipDelta& Delta,const FDefinition& D,int32 T,int32 A,int32 E,int32 O)
{
	Delta.CompanionId=D.Companion; Delta.TrustDelta=T; Delta.AffectionDelta=A; Delta.EvaluationDelta=E; Delta.OverloadDelta=O; Delta.SourceId=D.Id;
}

void ConfigureQTE(UPRQTEDataAsset& Asset,const FDefinition& D)
{
	Asset.QTEId=D.Id; Asset.CompanionId=D.Companion; Asset.QTEType=D.Type; Asset.DisplayName=FText::FromName(D.Id); Asset.PromptText=FText::FromString(TEXT("Respond now"));
	Asset.AcceptedInputTags.Reset(); Asset.AcceptedInputTags.AddTag(D.InputA); if(D.InputB.IsValid()) Asset.AcceptedInputTags.AddTag(D.InputB);
	Asset.WindowSeconds=D.Window; Asset.PerfectWindowFraction=.35f; Asset.BaseCooldownSeconds=D.Cooldown; Asset.MinimumTrust=40; Asset.MaximumOverload=90; Asset.Priority=0; Asset.TriggerSource=D.Source; Asset.TriggerKind=D.Trigger;
	Asset.RequiredAbilityTag=FGameplayTag(); Asset.RequiredResponseTag=FGameplayTag(); Asset.TriggerTargetScope=EPRQTETargetScope::AnyEnemy; Asset.TriggerHealthFraction=0.0f; Asset.RequiredDistinctTargetCount=0; Asset.bRequireFirstTargetShieldBreak=false;
	if(D.Trigger==EPRQTETriggerKind::PredictionBlocked) Asset.RequiredResponseTag=UPRTagLibrary::GetCombatResponsePredictionBlockedTag();
	if(D.Trigger==EPRQTETriggerKind::ShieldBroken) Asset.RequiredResponseTag=UPRTagLibrary::GetCombatResponseShieldBrokenTag();
	if(D.Trigger==EPRQTETriggerKind::ShadowThrustHit) Asset.RequiredAbilityTag=Tag(TEXT("Skill.ShadowThrust"));
	Asset.SuccessEffects.Reset(); FPRQTEEffectDefinition& Effect=Asset.SuccessEffects.AddDefaulted_GetRef(); Effect.EffectKind=D.Effect; Effect.Magnitude=D.Magnitude; Effect.Range=D.Range; Effect.MaxTargets=D.MaxTargets; Effect.PulseCount=D.Pulses; Effect.PulseIntervalSeconds=D.PulseInterval; Effect.FutureSampleId=D.FutureSample; Effect.TargetScope=EPRQTETargetScope::AnyEnemy;
	if (D.Id == TEXT("Axiom_WeaknessAxiom")) { Asset.TriggerTargetScope=EPRQTETargetScope::EliteOrBoss; Asset.bRequireFirstTargetShieldBreak=true; Effect.TargetScope=EPRQTETargetScope::EliteOrBoss; }
	if (D.Id == TEXT("Axiom_ProbabilityShield")) Asset.TriggerHealthFraction=.35f;
	if (D.Id == TEXT("Axiom_PhaseCut")) Effect.TargetScope=EPRQTETargetScope::EliteOrBoss;
	if (D.Id == TEXT("Kindle_Breakout")) Effect.TargetScope=EPRQTETargetScope::LightEnemy;
	if (D.Id == TEXT("Kindle_BurnoutTornado")) Asset.RequiredDistinctTargetCount=3;
	if (D.Id == TEXT("Kindle_ReverseBurnRescue")) Asset.TriggerHealthFraction=.20f;
	if (D.Id == TEXT("Null_DecoyJudgment")) Effect.TargetScope=EPRQTETargetScope::NonBossEnemy;
	if (D.Id == TEXT("Null_GarbageCollection")) Asset.TriggerHealthFraction=.20f;
	SetDelta(Asset.RelationshipDeltas.Success,D,D.Trust,D.Affection,D.Evaluation,D.Overload); SetDelta(Asset.RelationshipDeltas.Failure,D,0,0,-1,2); SetDelta(Asset.RelationshipDeltas.Rejected,D,-1,0,0,1); SetDelta(Asset.RelationshipDeltas.Timeout,D,0,0,-1,1);
	Asset.MarkPackageDirty();
}

UWidgetBlueprint* CreatePromptBlueprint()
{
	UWidgetBlueprintFactory* Factory=NewObject<UWidgetBlueprintFactory>(); Factory->ParentClass=UPRQTEPromptWidget::StaticClass();
	UWidgetBlueprint* Blueprint=Cast<UWidgetBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(TEXT("WBP_QTEPrompt"),TEXT("/Game/ProjectR/UI/QTE"),UWidgetBlueprint::StaticClass(),Factory));
	if(!Blueprint || !Blueprint->WidgetTree) return nullptr;
	UVerticalBox* Root=Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("QTEPromptRoot")); Blueprint->WidgetTree->RootWidget=Root;
	for(const TCHAR* Name:{TEXT("QTENameText"),TEXT("PromptText"),TEXT("InputText"),TEXT("TimeText")}) Root->AddChildToVerticalBox(Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name));
	// The official Blueprint tool compiles this asset after this synchronous MCP
	// writer returns. Compiling here can collect the MCP return object before
	// SetValue(), leaving the request blocked even though construction succeeded.
	Blueprint->MarkPackageDirty();
	return Blueprint;
}

bool OnlyManifestDirty(FString& Error)
{
	TSet<FString> Allowed; for(const TCHAR* Path:ManifestPackages) Allowed.Add(Path);
	TArray<UPackage*> Dirty; FEditorFileUtils::GetDirtyContentPackages(Dirty); for(UPackage* Package:Dirty) if(Package && !Allowed.Contains(Package->GetName())) { Error=FString::Printf(TEXT("Unexpected dirty package: %s"),*Package->GetName()); return false; }
	return true;
}

bool OnlyQTEDataDirty(FString& Error)
{
	TSet<FString> Allowed; for(int32 Index=0;Index<12;++Index) Allowed.Add(ManifestPackages[Index]);
	TArray<UPackage*> Dirty; FEditorFileUtils::GetDirtyContentPackages(Dirty); for(UPackage* Package:Dirty) if(Package && !Allowed.Contains(Package->GetName())) { Error=FString::Printf(TEXT("Unexpected dirty package: %s"),*Package->GetName()); return false; }
	return true;
}

bool SavePackage(const TCHAR* PackagePath,FString& Error)
{
	const FString Name(PackagePath); UObject* Object=LoadObject<UObject>(nullptr,*FString::Printf(TEXT("%s.%s"),PackagePath,*FPackageName::GetLongPackageAssetName(Name)));
	if(!Object){Error=FString::Printf(TEXT("Cannot load fixed manifest package: %s"),PackagePath);return false;}
	FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.Error=GError;
	return UPackage::SavePackage(Object->GetOutermost(),Object,*FPackageName::LongPackageNameToFilename(Name,FPackageName::GetAssetPackageExtension()),Args);
}
}

UToolCallAsyncResultString* UPRQTEAuthoringToolset::CreateV032QTEManifest()
{
	UToolCallAsyncResultString* Result=NewObject<UToolCallAsyncResultString>(); FString Error;
	if(!PRQTEAuthoring::VerifyCreatePreflight(Error)){Result->SetError(Error);return Result;}
	TArray<UPRQTEDataAsset*> Assets;
	for(const PRQTEAuthoring::FDefinition& Definition:PRQTEAuthoring::Definitions()){UPRQTEDataAsset* Asset=PRQTEAuthoring::CreateDataAsset(Definition.Package);if(!Asset){Result->SetError(TEXT("Could not create fixed QTE DataAsset."));return Result;}PRQTEAuthoring::ConfigureQTE(*Asset,Definition);Assets.Add(Asset);}
	UPackage* RegistryPackage=CreatePackage(PRQTEAuthoring::RegistryPackage); UPRQTERegistryDataAsset* Registry=NewObject<UPRQTERegistryDataAsset>(RegistryPackage,TEXT("DA_QTERegistry"),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Registry);
	UWidgetBlueprint* Prompt=PRQTEAuthoring::CreatePromptBlueprint(); if(!Prompt||!Prompt->GeneratedClass){Result->SetError(TEXT("Could not create fixed QTE prompt WidgetBlueprint."));return Result;}
	for(UPRQTEDataAsset* Asset:Assets) Registry->Entries.Add(Asset); Registry->PromptWidgetClass=Prompt->GeneratedClass; Registry->MarkPackageDirty();
	UPackage* ActionPackage=CreatePackage(PRQTEAuthoring::RejectActionPackage); UInputAction* Reject=NewObject<UInputAction>(ActionPackage,TEXT("IA_QTE_Reject"),RF_Public|RF_Standalone); Reject->ValueType=EInputActionValueType::Boolean;FAssetRegistryModule::AssetCreated(Reject);Reject->MarkPackageDirty();
	UPRInputConfigDataAsset* Config=LoadObject<UPRInputConfigDataAsset>(nullptr,TEXT("/Game/ProjectR/Input/DA_PlayerInputConfig.DA_PlayerInputConfig")); UInputMappingContext* Mapping=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/ProjectR/Input/IMC_Player.IMC_Player"));
	if(!Config||!Mapping){Result->SetError(TEXT("Frozen InputConfig or IMC_Player is unavailable."));return Result;}
	FPRTaggedInputAction Tagged; Tagged.InputTag=UPRTagLibrary::GetInputQTERejectTag();Tagged.InputAction=Reject;Config->TaggedInputActions.Add(Tagged);Config->MarkPackageDirty();Mapping->MapKey(Reject,EKeys::R);Mapping->MapKey(Reject,EKeys::Gamepad_DPad_Left);Mapping->MarkPackageDirty();
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":15,\"modified\":2,\"saved\":false,\"mapsSaved\":false}"));return Result;
}

UToolCallAsyncResultString* UPRQTEAuthoringToolset::SaveV032QTEManifest()
{
	UToolCallAsyncResultString* Result=NewObject<UToolCallAsyncResultString>(); FString Error;if(!PRQTEAuthoring::OnlyManifestDirty(Error)){Result->SetError(Error);return Result;}
	for(const TCHAR* Path:PRQTEAuthoring::ManifestPackages) if(!PRQTEAuthoring::SavePackage(Path,Error)){Result->SetError(Error.IsEmpty()?FString::Printf(TEXT("Could not save %s"),Path):Error);return Result;}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"saved\":17,\"mapsSaved\":false}"));return Result;
}

UToolCallAsyncResultString* UPRQTEAuthoringToolset::ReconfigureV032QTEContracts()
{
	UToolCallAsyncResultString* Result=NewObject<UToolCallAsyncResultString>();
	for(const PRQTEAuthoring::FDefinition& Definition:PRQTEAuthoring::Definitions())
	{
		UPRQTEDataAsset* Asset=LoadObject<UPRQTEDataAsset>(nullptr,*FString::Printf(TEXT("%s.%s"),Definition.Package,*FPackageName::GetLongPackageAssetName(Definition.Package)));
		if(!Asset){Result->SetError(FString::Printf(TEXT("Cannot load fixed QTE package: %s"),Definition.Package));return Result;}
		PRQTEAuthoring::ConfigureQTE(*Asset,Definition);
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"reconfigured\":12,\"saved\":false,\"mapsSaved\":false}"));return Result;
}

UToolCallAsyncResultString* UPRQTEAuthoringToolset::SaveReconfiguredV032QTEContracts()
{
	UToolCallAsyncResultString* Result=NewObject<UToolCallAsyncResultString>(); FString Error;if(!PRQTEAuthoring::OnlyQTEDataDirty(Error)){Result->SetError(Error);return Result;}
	for(int32 Index=0;Index<12;++Index) if(!PRQTEAuthoring::SavePackage(PRQTEAuthoring::ManifestPackages[Index],Error)){Result->SetError(Error.IsEmpty()?FString::Printf(TEXT("Could not save %s"),PRQTEAuthoring::ManifestPackages[Index]):Error);return Result;}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"saved\":12,\"mapsSaved\":false}"));return Result;
}
