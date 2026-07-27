// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRRoguelikeAuthoringToolset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/PanelWidget.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Roguelike/PREncounterDataAsset.h"
#include "Roguelike/PRRewardDataAsset.h"
#include "Roguelike/PRRewardPolicyDataAsset.h"
#include "Roguelike/PRRoguelikeContentRegistryDataAsset.h"
#include "Roguelike/PRRoomDataAsset.h"
#include "Roguelike/PRRoomEventDataAsset.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRRewardSelectionWidget.h"
#include "UI/PRRoomEventWidget.h"
#include "UI/PRRoomFlowWidget.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/UserWidget.h"

namespace PRRoguelikeAuthoring
{
struct FRoot { explicit FRoot(UToolCallAsyncResultString* In) : Value(In) { Value->AddToRoot(); } ~FRoot() { Value->RemoveFromRoot(); } UToolCallAsyncResultString* Value; };
FGameplayTag Tag(const TCHAR* Name) { return FGameplayTag::RequestGameplayTag(FName(Name), false); }
bool Occupied(const FString& Path) { return FPackageName::DoesPackageExist(Path); }
template <typename TAsset> TAsset* Create(const FString& Path)
{
	const FString ObjectPath = Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path);
	if (TAsset* Existing = LoadObject<TAsset>(nullptr, *ObjectPath)) { Existing->Modify(); return Existing; }
	UPackage* Package = CreatePackage(*Path); if (!Package) return nullptr;
	TAsset* Asset = NewObject<TAsset>(Package, FName(FPackageName::GetLongPackageAssetName(Path)), RF_Public | RF_Standalone);
	if (Asset) { FAssetRegistryModule::AssetCreated(Asset); Asset->MarkPackageDirty(); }
	return Asset;
}

UWidgetBlueprint* CreateWidget(const TCHAR* Path, UClass* Parent)
{
	const FString ObjectPath = FString(Path) + TEXT(".") + FPackageName::GetLongPackageAssetName(Path);
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath);
	if (Blueprint) Blueprint->Modify();
	else { UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>(); Factory->ParentClass = Parent; Blueprint = Cast<UWidgetBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(FPackageName::GetLongPackageAssetName(Path), FPackageName::GetLongPackagePath(Path), UWidgetBlueprint::StaticClass(), Factory)); }
	if (!Blueprint || !Blueprint->WidgetTree) return nullptr;
	UVerticalBox* Root = Cast<UVerticalBox>(Blueprint->WidgetTree->RootWidget); if (!Root) { Root = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root")); Blueprint->WidgetTree->RootWidget = Root; }
	if (!Blueprint->WidgetTree->FindWidget(TEXT("StatusText"))) Root->AddChildToVerticalBox(Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText")));
	for (int32 Index = 0; Index < 3; ++Index) if (!Blueprint->WidgetTree->FindWidget(*FString::Printf(TEXT("Choice%d"), Index))) { UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("Choice%d"), Index)); Button->AddChild(Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("ChoiceText%d"), Index))); Root->AddChildToVerticalBox(Button); }
	Blueprint->MarkPackageDirty();
	return Blueprint;
}

bool AttachToCombatHUD(UWidgetBlueprint* RoomFlow, UWidgetBlueprint* RewardSelection, UWidgetBlueprint* RoomEvent)
{
	UWidgetBlueprint* HUD = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/ProjectR/UI/Combat/WBP_CombatHUD.WBP_CombatHUD"));
	UPanelWidget* Root = HUD && HUD->WidgetTree ? Cast<UPanelWidget>(HUD->WidgetTree->RootWidget) : nullptr;
	if (!Root || !RoomFlow || !RewardSelection || !RoomEvent || !RoomFlow->GeneratedClass || !RewardSelection->GeneratedClass || !RoomEvent->GeneratedClass) return false;
	for (const TCHAR* Name : { TEXT("RoguelikeRoomFlow"), TEXT("RoguelikeRewardSelection"), TEXT("RoguelikeRoomEvent") }) if (HUD->WidgetTree->FindWidget(Name)) { HUD->MarkPackageDirty(); return true; }
	for (const TPair<const TCHAR*, UWidgetBlueprint*>& Entry : { TPair<const TCHAR*, UWidgetBlueprint*>(TEXT("RoguelikeRoomFlow"), RoomFlow), TPair<const TCHAR*, UWidgetBlueprint*>(TEXT("RoguelikeRewardSelection"), RewardSelection), TPair<const TCHAR*, UWidgetBlueprint*>(TEXT("RoguelikeRoomEvent"), RoomEvent) })
	{
		UUserWidget* Widget = HUD->WidgetTree->ConstructWidget<UUserWidget>(TSubclassOf<UUserWidget>(Entry.Value->GeneratedClass), Entry.Key); if (!Widget) return false; Root->AddChild(Widget);
	}
	HUD->MarkPackageDirty(); return true;
}

FPRRewardEffectSpec RewardEffect(const int32 Family, const int32 Tier)
{
	static const EPRRewardAttribute Attributes[] = { EPRRewardAttribute::MaxHealth, EPRRewardAttribute::MaxShield, EPRRewardAttribute::MaxEnergy, EPRRewardAttribute::AttackPower, EPRRewardAttribute::MoveSpeed, EPRRewardAttribute::CritChance, EPRRewardAttribute::Health, EPRRewardAttribute::Shield, EPRRewardAttribute::Energy, EPRRewardAttribute::Resonance };
	static const float Values[][3] = { {10,20,30},{5,10,15},{5,10,15},{1,2,3},{15,30,45},{.01f,.02f,.03f},{20,35,50},{10,20,30},{15,25,40},{10,20,30} };
	FPRRewardEffectSpec Spec; Spec.Attribute = Attributes[Family]; Spec.Magnitude = Values[Family][Tier]; Spec.Duration = Family < 6 ? EPRRewardEffectDuration::Session : EPRRewardEffectDuration::Instant; return Spec;
}
}

UToolCallAsyncResultString* UPRRoguelikeAuthoringToolset::CreateAndConfigureFixedRoguelikeManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>(); PRRoguelikeAuthoring::FRoot Root(Result); using namespace PRRoguelikeAuthoring;
	const TCHAR* RootPath = TEXT("/Game/ProjectR/Roguelike");
	const TCHAR* RoomNames[] = { TEXT("CombatStandard"),TEXT("EliteAudit"),TEXT("BossAuditor"),TEXT("SafeCache"),TEXT("EventEthics"),TEXT("EventMemoryCorridor"),TEXT("EventCurseProtocol"),TEXT("EventCommission") };
	const TCHAR* EncounterNames[] = { TEXT("CombatStandard"),TEXT("EliteAudit"),TEXT("BossAuditor") };
	const TCHAR* EventNames[] = { TEXT("Ethics"),TEXT("MemoryCorridor"),TEXT("CurseProtocol"),TEXT("Commission") };
	const TCHAR* PolicyNames[] = { TEXT("Combat"),TEXT("Elite"),TEXT("Boss"),TEXT("Event") };
	const TCHAR* Families[] = { TEXT("VitalityCache"),TEXT("ShieldCache"),TEXT("EnergyCache"),TEXT("AttackCache"),TEXT("MobilityCache"),TEXT("PrecisionCache"),TEXT("HealthRepair"),TEXT("ShieldRepair"),TEXT("EnergyRefill"),TEXT("ResonancePulse") };
	const TCHAR* Tiers[] = { TEXT("Common"),TEXT("Rare"),TEXT("Epic") };
	TArray<FString> Paths = { FString(RootPath) + TEXT("/DA_RoguelikeContentRegistry") };
	for (const TCHAR* Name : RoomNames) Paths.Add(FString(RootPath) + TEXT("/Rooms/DA_Room_") + Name);
	for (const TCHAR* Name : EncounterNames) Paths.Add(FString(RootPath) + TEXT("/Encounters/DA_Encounter_") + Name);
	for (const TCHAR* Name : EventNames) Paths.Add(FString(RootPath) + TEXT("/Events/DA_RoomEvent_") + Name);
	for (const TCHAR* Name : PolicyNames) Paths.Add(FString(RootPath) + TEXT("/Rewards/Policies/DA_RewardPolicy_") + Name);
	for (const TCHAR* Family : Families) for (const TCHAR* Tier : Tiers) Paths.Add(FString(RootPath) + TEXT("/Rewards/Data/DA_Reward_") + Family + TEXT("_") + Tier);
	Paths.Add(TEXT("/Game/ProjectR/UI/Roguelike/WBP_RoomFlow")); Paths.Add(TEXT("/Game/ProjectR/UI/Roguelike/WBP_RewardSelection")); Paths.Add(TEXT("/Game/ProjectR/UI/Roguelike/WBP_RoomEvent"));

	UPRRoguelikeContentRegistryDataAsset* Registry = Create<UPRRoguelikeContentRegistryDataAsset>(Paths[0]); if (!Registry) { Result->SetError(TEXT("Registry creation failed.")); return Result; }
	Registry->Rooms.Reset(); Registry->Encounters.Reset(); Registry->Events.Reset(); Registry->RewardPolicies.Reset(); Registry->Rewards.Reset(); Registry->EventRoomBindings.Reset(); Registry->DirectorRoomWeightAdjustments.Reset();
	const auto AddRoomWeight = [Registry](const TCHAR* Rule, const TCHAR* RoomType, const int32 Delta) { FPRDirectorRoomWeightAdjustment& Adjustment = Registry->DirectorRoomWeightAdjustments.AddDefaulted_GetRef(); Adjustment.RuleId = Tag(Rule); Adjustment.RoomType = Tag(RoomType); Adjustment.WeightDelta = Delta; };
	AddRoomWeight(TEXT("Rule.CooperationAudit"), TEXT("Room.Type.Event"), 2);
	AddRoomWeight(TEXT("Rule.OptimalPath"), TEXT("Room.Type.Safe"), 3);
	AddRoomWeight(TEXT("Rule.RiskReward"), TEXT("Room.Type.Elite"), 4);
	TArray<UPREncounterDataAsset*> Encounters;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(EncounterNames); ++Index)
	{
		UPREncounterDataAsset* Asset = Create<UPREncounterDataAsset>(FString(RootPath) + TEXT("/Encounters/DA_Encounter_") + EncounterNames[Index]); if (!Asset) { Result->SetError(TEXT("Encounter creation failed.")); return Result; }
		Asset->SpawnDefinitions.Reset(); Asset->ExpectedBossId = FGameplayTag(); Asset->EncounterId = Asset->GetPrimaryAssetId(); Asset->Kind = Index == 0 ? EPRRoomEncounterKind::Combat : Index == 1 ? EPRRoomEncounterKind::Elite : EPRRoomEncounterKind::Boss;
		if (Asset->Kind == EPRRoomEncounterKind::Boss) Asset->ExpectedBossId = Tag(TEXT("Enemy.Type.AuditorBoss"));
		else { const TCHAR* Enemy = Asset->Kind == EPRRoomEncounterKind::Elite ? TEXT("Enemy.Type.EliteAuditGuard") : TEXT("Enemy.Type.MeleeMinion"); for (const FVector Offset : { FVector(170,0,0), FVector(380,0,0), FVector(520,0,0) }) { FPREncounterSpawnDefinition& Spawn = Asset->SpawnDefinitions.AddDefaulted_GetRef(); Spawn.PrototypeTag = Tag(Enemy); Spawn.RelativeLocation = Offset; } }
		Encounters.Add(Asset); Registry->Encounters.Add(Asset);
	}
	TArray<UPRRewardDataAsset*> Rewards;
	for (int32 Family = 0; Family < UE_ARRAY_COUNT(Families); ++Family) for (int32 Tier = 0; Tier < 3; ++Tier)
	{
		UPRRewardDataAsset* Asset = Create<UPRRewardDataAsset>(FString(RootPath) + TEXT("/Rewards/Data/DA_Reward_") + Families[Family] + TEXT("_") + Tiers[Tier]); if (!Asset) { Result->SetError(TEXT("Reward creation failed.")); return Result; }
		Asset->MutualExclusionTags.Reset(); Asset->WeightConditions.Reset(); Asset->RewardId = Asset->GetPrimaryAssetId(); Asset->RarityTag = Tag(*FString::Printf(TEXT("Reward.Rarity.%s"), Tiers[Tier])); Asset->RewardTypeTag = Tag(Family < 6 ? TEXT("Reward.Type.SkillPlugin") : TEXT("Reward.Type.Resource")); Asset->FamilyId = FName(Families[Family]); Asset->ApplicationId = FName(*FString::Printf(TEXT("%s_%s"), Families[Family], Tiers[Tier])); Asset->Tier = Tier + 1; Asset->EffectSpec = RewardEffect(Family, Tier); Asset->DisplayName = FText::FromString(FString::Printf(TEXT("%s %s"), Families[Family], Tiers[Tier])); Asset->EffectText = FText::FromString(TEXT("Bounded session reward")); Asset->CostText = FText::GetEmpty(); Rewards.Add(Asset); Registry->Rewards.Add(Asset);
	}
	TArray<UPRRewardPolicyDataAsset*> Policies;
	const int32 Weights[][3] = { {70,25,5},{40,45,15},{20,40,40},{60,30,10} };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PolicyNames); ++Index)
	{
		UPRRewardPolicyDataAsset* Asset = Create<UPRRewardPolicyDataAsset>(FString(RootPath) + TEXT("/Rewards/Policies/DA_RewardPolicy_") + PolicyNames[Index]); if (!Asset) { Result->SetError(TEXT("Policy creation failed.")); return Result; }
		Asset->RewardIds.Reset(); Asset->PolicyId = FName(*FString::Printf(TEXT("RewardPolicy.%s"), PolicyNames[Index])); Asset->CommonWeight=Weights[Index][0]; Asset->RareWeight=Weights[Index][1]; Asset->EpicWeight=Weights[Index][2]; for (UPRRewardDataAsset* Reward : Rewards) Asset->RewardIds.Add(Reward->GetPrimaryAssetId()); Policies.Add(Asset); Registry->RewardPolicies.Add(Asset);
	}
	TArray<UPRRoomEventDataAsset*> Events;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(EventNames); ++Index)
	{
		UPRRoomEventDataAsset* Asset = Create<UPRRoomEventDataAsset>(FString(RootPath) + TEXT("/Events/DA_RoomEvent_") + EventNames[Index]); if (!Asset) { Result->SetError(TEXT("Event creation failed.")); return Result; }
		Asset->Choices.Reset(); Asset->EventId = Asset->GetPrimaryAssetId(); Asset->DisplayName=FText::FromString(EventNames[Index]); Asset->Description=FText::FromString(TEXT("A bounded room event."));
		const auto AddChoice = [Asset](const TCHAR* Id, int32 Trust, int32 Affection, int32 Evaluation, int32 Overload, bool bQTE, bool bEpic) { FPRRoomEventChoice& Choice=Asset->Choices.AddDefaulted_GetRef(); Choice.ChoiceId=FName(Id); Choice.DisplayName=FText::FromString(Id); Choice.Description=FText::FromString(TEXT("Fixed room event choice.")); Choice.RelationshipDelta.TrustDelta=Trust; Choice.RelationshipDelta.AffectionDelta=Affection; Choice.RelationshipDelta.EvaluationDelta=Evaluation; Choice.RelationshipDelta.OverloadDelta=Overload; Choice.bRequiresQTESuccess=bQTE; Choice.bBoostEpicWeight=bEpic; };
		switch (Index) { case 0: AddChoice(TEXT("Observe"),0,0,0,0,false,false); AddChoice(TEXT("Cooperate"),2,0,1,0,false,false); AddChoice(TEXT("Defy"),0,2,-1,0,false,false); break; case 1: AddChoice(TEXT("Pass"),0,0,0,0,false,false); AddChoice(TEXT("Preserve"),0,2,0,0,false,false); AddChoice(TEXT("Archive"),0,0,2,-2,false,false); break; case 2: AddChoice(TEXT("Refuse"),0,0,0,0,false,false); AddChoice(TEXT("Accept"),0,0,0,5,false,true); break; default: AddChoice(TEXT("Decline"),0,0,0,0,false,false); AddChoice(TEXT("Fulfill"),2,0,2,0,true,false); break; } Events.Add(Asset); Registry->Events.Add(Asset);
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(RoomNames); ++Index)
	{
		UPRRoomDataAsset* Asset = Create<UPRRoomDataAsset>(FString(RootPath) + TEXT("/Rooms/DA_Room_") + RoomNames[Index]); if (!Asset) { Result->SetError(TEXT("Room creation failed.")); return Result; }
		Asset->EntryConditions.Reset(); Asset->ExitConditions.Reset(); Asset->ContentTags.Reset(); Asset->RoomId=Asset->GetPrimaryAssetId(); const bool bBoss=Index==2,bElite=Index==1,bSafe=Index==3,bEvent=Index>=4; Asset->TypeTag=Tag(bBoss?TEXT("Room.Type.Boss"):bElite?TEXT("Room.Type.Elite"):bSafe?TEXT("Room.Type.Safe"):bEvent?TEXT("Room.Type.Event"):TEXT("Room.Type.Combat")); Asset->LevelAsset=TSoftObjectPtr<UWorld>(FSoftObjectPath(bBoss?TEXT("/Game/ProjectR/Maps/L_BossGym.L_BossGym"):TEXT("/Game/ProjectR/Maps/L_CombatGym.L_CombatGym"))); Asset->EncounterId=bBoss?Encounters[2]->GetPrimaryAssetId():bElite?Encounters[1]->GetPrimaryAssetId():Encounters[0]->GetPrimaryAssetId(); Asset->RewardPolicyId=Policies[bBoss?2:bElite?1:bEvent?3:0]->GetPrimaryAssetId(); Asset->DisplayName=FText::FromString(RoomNames[Index]); Asset->Description=FText::FromString(TEXT("Fixed v0.4.2 room.")); Registry->Rooms.Add(Asset); if (bEvent) { FPRRoomEventBinding& Binding=Registry->EventRoomBindings.AddDefaulted_GetRef(); Binding.RoomId=Asset->GetPrimaryAssetId(); Binding.EventId=Events[Index-4]->GetPrimaryAssetId(); }
	}
	const auto SortReferences = [](auto& References) { References.Sort([](const auto& Left, const auto& Right) { return Left.ToSoftObjectPath().ToString() < Right.ToSoftObjectPath().ToString(); }); };
	SortReferences(Registry->Rooms); SortReferences(Registry->Encounters); SortReferences(Registry->Events); SortReferences(Registry->RewardPolicies); SortReferences(Registry->Rewards);
	UWidgetBlueprint* RoomFlow=CreateWidget(TEXT("/Game/ProjectR/UI/Roguelike/WBP_RoomFlow"), UPRRoomFlowWidget::StaticClass()); UWidgetBlueprint* RewardSelection=CreateWidget(TEXT("/Game/ProjectR/UI/Roguelike/WBP_RewardSelection"), UPRRewardSelectionWidget::StaticClass()); UWidgetBlueprint* RoomEvent=CreateWidget(TEXT("/Game/ProjectR/UI/Roguelike/WBP_RoomEvent"), UPRRoomEventWidget::StaticClass());
	if (!RoomFlow || !RewardSelection || !RoomEvent || !AttachToCombatHUD(RoomFlow, RewardSelection, RoomEvent)) { Result->SetError(TEXT("Roguelike Widget or CombatHUD configuration failed.")); return Result; }
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"created\":53,\"modified\":1,\"saved\":false}")); return Result;
}
