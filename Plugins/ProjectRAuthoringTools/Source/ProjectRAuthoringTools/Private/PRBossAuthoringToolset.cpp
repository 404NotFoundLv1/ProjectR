// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRBossAuthoringToolset.h"

#include "Abilities/PRAbilitySetDataAsset.h"
#include "Abilities/PRAbilitySystemComponent.h"
#include "Abilities/PRAttributeSet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Enemies/Bosses/PRAuditorBossDataAsset.h"
#include "Enemies/Bosses/PRBossAuditor.h"
#include "Enemies/Bosses/PRAuditorBossComponent.h"
#include "Enemies/Bosses/PRBossPrototypeGameMode.h"
#include "Enemies/Bosses/PRBossSubsystem.h"
#include "Enemies/PREnemyBrainComponent.h"
#include "Combat/PRCombatSubsystem.h"
#include "Combat/PRCombatTypes.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Enemies/PREnemyAttackDataAsset.h"
#include "Enemies/PREnemyAttackGameplayAbility.h"
#include "Enemies/PREnemyProjectile.h"
#include "Enemies/PREnemyPrototypeDataAsset.h"
#include "Engine/World.h"
#include "Factories/SoundFactory.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#include "Sound/SoundWave.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UI/PRBossHealthWidget.h"
#include "UI/PRBossPrototypeResultWidget.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "StateTree.h"

namespace PRBossAuthoring
{
constexpr const TCHAR* Paths[] = {
	TEXT("/Game/ProjectR/Enemies/Bosses/Blueprints/BP_Boss_Auditor"), TEXT("/Game/ProjectR/Enemies/Bosses/Prototypes/DA_Boss_Auditor"), TEXT("/Game/ProjectR/Enemies/Bosses/Abilities/DA_EnemyAbilitySet_AuditorBoss"),
	TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorSweep"), TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorLockShot"), TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorCounter"),
	TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/GA_Enemy_AuditorSweep"), TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/GA_Enemy_AuditorLockShot"), TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/GA_Enemy_AuditorCounter"),
	TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorSweep_Cooldown"), TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorLockShot_Cooldown"), TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorCounter_Cooldown"), TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_Boss_Auditor_PredictionShield"),
	TEXT("/Game/ProjectR/Enemies/Bosses/Blueprints/BP_EnemyProjectile_AuditorLockShot"),
	TEXT("/Game/ProjectR/Enemies/Bosses/VFX/VFX_Boss_AuditorSweep"), TEXT("/Game/ProjectR/Enemies/Bosses/VFX/VFX_Boss_AuditorLockShot"), TEXT("/Game/ProjectR/Enemies/Bosses/VFX/VFX_Boss_AuditorCounter"),
	TEXT("/Game/ProjectR/Enemies/Bosses/Audio/SFX_Boss_AuditorSweep"), TEXT("/Game/ProjectR/Enemies/Bosses/Audio/SFX_Boss_AuditorLockShot"), TEXT("/Game/ProjectR/Enemies/Bosses/Audio/SFX_Boss_AuditorCounter"),
	TEXT("/Game/ProjectR/UI/Boss/WBP_BossHealth"), TEXT("/Game/ProjectR/UI/Boss/WBP_BossPrototypeResult"), TEXT("/Game/ProjectR/GameFramework/BP_BossPrototypeGameMode")};

template <typename T> T* CreateAsset(const TCHAR* Path) { UPackage* P=CreatePackage(Path); T* A=NewObject<T>(P,*FPackageName::GetLongPackageAssetName(Path),RF_Public|RF_Standalone); FAssetRegistryModule::AssetCreated(A); P->MarkPackageDirty(); return A; }
UBlueprint* CreateBP(const TCHAR* Path,UClass* Parent) { UPackage* P=CreatePackage(Path); UBlueprint* B=FKismetEditorUtilities::CreateBlueprint(Parent,P,FName(FPackageName::GetLongPackageAssetName(Path)),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass()); if(B) FAssetRegistryModule::AssetCreated(B); return B; }
bool Save(UObject* A,FString& E) { FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.Error=GError; const FString Extension=A->IsA<UWorld>()?FPackageName::GetMapPackageExtension():FPackageName::GetAssetPackageExtension(); const FString F=FPackageName::LongPackageNameToFilename(A->GetOutermost()->GetName(),Extension); if(!UPackage::SavePackage(A->GetOutermost(),A,*F,Args)){E=FString::Printf(TEXT("Exact Boss manifest save failed: %s"),*A->GetPathName());return false;}return true; }
void SetTag(UObject* O,const TCHAR* Name,const TCHAR* Value) { if(FStructProperty* P=FindFProperty<FStructProperty>(O->GetClass(),Name)){*P->ContainerPtrToValuePtr<FGameplayTag>(O)=FGameplayTag::RequestGameplayTag(Value);} }
void ConfigureAttack(UPREnemyAttackDataAsset* A,const TCHAR* Id,const TCHAR* Tag,EPREnemyAttackKind Kind,float Min,float Max,float Base,float Scale,float W,float Active,float R,float CD){A->AttackId=Id;A->AttackTag=FGameplayTag::RequestGameplayTag(Tag);A->Kind=Kind;A->MinRange=Min;A->MaxRange=Max;A->BaseDamage=Base;A->AttackPowerScale=Scale;A->Windup=W;A->ActiveWindow=Active;A->Recovery=R;A->Cooldown=CD;A->SweepRadius=Max;A->SweepHalfAngleDegrees=60.0f;}
void ConfigureCooldown(UGameplayEffect* Effect, const FGameplayTag CooldownTag, const float Duration)
{
	Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
	Effect->Period = FScalableFloat(0.0f);
	Effect->Modifiers.Reset();
	Effect->Executions.Reset();
	Effect->GameplayCues.Reset();
	FInheritedTagContainer Tags;
	Tags.AddTag(CooldownTag);
	Effect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(Tags);
}

void AddOverrideModifier(UGameplayEffect* Effect, const FGameplayAttribute Attribute, const float Value)
{
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Override;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
}

bool SetClassProperty(UClass* Class, const TCHAR* PropertyName, UClass* Value, FString& OutError)
{
	if (FClassProperty* Property = FindFProperty<FClassProperty>(Class, PropertyName))
	{
		Property->SetPropertyValue_InContainer(Class->GetDefaultObject(), Value);
		return true;
	}
	OutError = FString::Printf(TEXT("Fixed Boss manifest property is unavailable: %s.%s"), *Class->GetName(), PropertyName);
	return false;
}

bool SetObjectProperty(UClass* Class, const TCHAR* PropertyName, UObject* Value, FString& OutError)
{
	if (FObjectProperty* Property = FindFProperty<FObjectProperty>(Class, PropertyName))
	{
		Property->SetObjectPropertyValue_InContainer(Class->GetDefaultObject(), Value);
		return true;
	}
	OutError = FString::Printf(TEXT("Fixed Boss manifest property is unavailable: %s.%s"), *Class->GetName(), PropertyName);
	return false;
}

bool SetAbilityTag(UClass* Class, const FGameplayTag Tag, FString& OutError)
{
	if (FStructProperty* Property = FindFProperty<FStructProperty>(Class, TEXT("AbilityTag")))
	{
		*Property->ContainerPtrToValuePtr<FGameplayTag>(Class->GetDefaultObject()) = Tag;
		return true;
	}
	OutError = FString::Printf(TEXT("Fixed Boss attack ability is missing AbilityTag: %s"), *Class->GetName());
	return false;
}
USoundWave* Sound(const TCHAR* P){const uint8 B[]={'R','I','F','F',0x24,0,0,0,'W','A','V','E','f','m','t',' ',0x10,0,0,0,1,0,1,0,0x22,0x56,0,0,0x44,0xac,0,0,2,0,0x10,0,'d','a','t','a',0,0,0,0};const uint8* Buffer=B;UPackage* K=CreatePackage(P);USoundFactory* F=NewObject<USoundFactory>();USoundWave* S=Cast<USoundWave>(F->FactoryCreateBinary(USoundWave::StaticClass(),K,*FPackageName::GetLongPackageAssetName(P),RF_Public|RF_Standalone,nullptr,TEXT("wav"),Buffer,B+UE_ARRAY_COUNT(B),GWarn));if(S){K->MarkPackageDirty();FAssetRegistryModule::AssetCreated(S);}return S;}
}

UToolCallAsyncResultString* UPRBossAuthoringToolset::CreateAndSaveAuditorBossManifest()
{
	UToolCallAsyncResultString* R=NewObject<UToolCallAsyncResultString>(); R->AddToRoot();
	for(const TCHAR* P:PRBossAuthoring::Paths) if(FindObject<UObject>(nullptr,P)){R->SetError(FString::Printf(TEXT("Boss manifest collision: %s"),P));R->RemoveFromRoot();return R;}
	UPREnemyPrototypeRegistryDataAsset* Registry=LoadObject<UPREnemyPrototypeRegistryDataAsset>(nullptr,TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	if(!Registry||Registry->Entries.Num()!=4){R->SetError(TEXT("Boss manifest requires the unchanged four-entry Enemy Registry."));R->RemoveFromRoot();return R;}
	UBlueprint* Boss=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[0],APRBossAuditor::StaticClass());
	UPRAuditorBossDataAsset* Proto=PRBossAuthoring::CreateAsset<UPRAuditorBossDataAsset>(PRBossAuthoring::Paths[1]);
	UPRAbilitySetDataAsset* Set=PRBossAuthoring::CreateAsset<UPRAbilitySetDataAsset>(PRBossAuthoring::Paths[2]);
	UPREnemyAttackDataAsset* Sweep=PRBossAuthoring::CreateAsset<UPREnemyAttackDataAsset>(PRBossAuthoring::Paths[3]); UPREnemyAttackDataAsset* Shot=PRBossAuthoring::CreateAsset<UPREnemyAttackDataAsset>(PRBossAuthoring::Paths[4]); UPREnemyAttackDataAsset* Counter=PRBossAuthoring::CreateAsset<UPREnemyAttackDataAsset>(PRBossAuthoring::Paths[5]);
	UBlueprint* GASweep=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[6],UPREnemyAttackGameplayAbility::StaticClass());UBlueprint* GAShot=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[7],UPREnemyAttackGameplayAbility::StaticClass());UBlueprint* GACounter=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[8],UPREnemyAttackGameplayAbility::StaticClass());
	UBlueprint* CDSweep=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[9],UGameplayEffect::StaticClass());UBlueprint* CDShot=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[10],UGameplayEffect::StaticClass());UBlueprint* CDCounter=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[11],UGameplayEffect::StaticClass());UBlueprint* Shield=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[12],UGameplayEffect::StaticClass());
	UBlueprint* Projectile=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[13],APREnemyProjectile::StaticClass());
	UNiagaraSystemFactoryNew* NF=NewObject<UNiagaraSystemFactoryNew>(); auto Niagara=[&](const TCHAR* P){return Cast<UNiagaraSystem>(FAssetToolsModule::GetModule().Get().CreateAsset(FPackageName::GetLongPackageAssetName(P),FPackageName::GetLongPackagePath(P),UNiagaraSystem::StaticClass(),NF));}; UNiagaraSystem* V1=Niagara(PRBossAuthoring::Paths[14]);UNiagaraSystem* V2=Niagara(PRBossAuthoring::Paths[15]);UNiagaraSystem* V3=Niagara(PRBossAuthoring::Paths[16]);USoundWave* S1=PRBossAuthoring::Sound(PRBossAuthoring::Paths[17]);USoundWave* S2=PRBossAuthoring::Sound(PRBossAuthoring::Paths[18]);USoundWave* S3=PRBossAuthoring::Sound(PRBossAuthoring::Paths[19]);
	UBlueprint* WH=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[20],UPRBossHealthWidget::StaticClass());UBlueprint* WR=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[21],UPRBossPrototypeResultWidget::StaticClass());UBlueprint* GM=PRBossAuthoring::CreateBP(PRBossAuthoring::Paths[22],APRBossPrototypeGameMode::StaticClass());
	if(!Boss||!Proto||!Set||!Sweep||!Shot||!Counter||!GASweep||!GAShot||!GACounter||!CDSweep||!CDShot||!CDCounter||!Shield||!Projectile||!V1||!V2||!V3||!S1||!S2||!S3||!WH||!WR||!GM){R->SetError(TEXT("Boss fixed manifest create failed."));R->RemoveFromRoot();return R;}
	FKismetEditorUtilities::CompileBlueprint(GASweep);FKismetEditorUtilities::CompileBlueprint(GAShot);FKismetEditorUtilities::CompileBlueprint(GACounter);FKismetEditorUtilities::CompileBlueprint(CDSweep);FKismetEditorUtilities::CompileBlueprint(CDShot);FKismetEditorUtilities::CompileBlueprint(CDCounter);FKismetEditorUtilities::CompileBlueprint(Shield);FKismetEditorUtilities::CompileBlueprint(Boss);FKismetEditorUtilities::CompileBlueprint(Projectile);FKismetEditorUtilities::CompileBlueprint(WH);FKismetEditorUtilities::CompileBlueprint(WR);FKismetEditorUtilities::CompileBlueprint(GM);
	PRBossAuthoring::ConfigureAttack(Sweep,TEXT("AuditorSweep"),TEXT("Enemy.Attack.AuditorSweep"),EPREnemyAttackKind::MeleeSweep,0,220,16,.8f,.45f,.18f,.60f,1.60f);PRBossAuthoring::ConfigureAttack(Shot,TEXT("AuditorLockShot"),TEXT("Enemy.Attack.AuditorLockShot"),EPREnemyAttackKind::Projectile,200,900,10,.6f,.65f,.12f,.45f,2.0f);PRBossAuthoring::ConfigureAttack(Counter,TEXT("AuditorCounter"),TEXT("Enemy.Attack.AuditorCounter"),EPREnemyAttackKind::MeleeSweep,0,300,20,.9f,.80f,.18f,.80f,3.0f);
	Proto->EnemyId=TEXT("Auditor");Proto->PrototypeTag=FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.AuditorBoss"));Proto->Mobility=EPRAbilityTargetMobility::Anchored;Proto->Attributes={900,900,120,120,0,0,18,320,0,0,0};Proto->Perception={.10f,1600,1900,180,650,60,120};Proto->AttackDefinitions={Sweep,Shot,Counter};Proto->InitialAbilitySet=Set;Proto->PredictionShieldEffect=Shield->GeneratedClass;
	for(auto Pair:TArray<TPair<UBlueprint*,const TCHAR*>>{{GASweep,TEXT("Enemy.Attack.AuditorSweep")},{GAShot,TEXT("Enemy.Attack.AuditorLockShot")},{GACounter,TEXT("Enemy.Attack.AuditorCounter")}})PRBossAuthoring::SetTag(Pair.Key->GeneratedClass->GetDefaultObject(),TEXT("AbilityTag"),Pair.Value);
	auto AddAbility=[&](UBlueprint* Ability){FPRAbilitySetEntry& Entry=Set->AbilityEntries.AddDefaulted_GetRef();Entry.AbilityClass=Ability->GeneratedClass;Entry.AbilityLevel=1;Entry.bGrantOnInitialization=true;};AddAbility(GASweep);AddAbility(GAShot);AddAbility(GACounter);
	FPREnemyPrototypeRegistryEntry E;E.PrototypeTag=Proto->PrototypeTag;E.Prototype=Proto;E.EnemyClass=Boss->GeneratedClass;Registry->Entries.Add(E);Registry->MarkPackageDirty();
	UWorld* BossGym=LoadObject<UWorld>(nullptr,TEXT("/Game/ProjectR/Maps/L_BossGym.L_BossGym"));if(!BossGym||!BossGym->GetWorldSettings()||!GM->GeneratedClass){R->SetError(TEXT("Boss manifest requires L_BossGym and Boss Prototype GameMode."));R->RemoveFromRoot();return R;}BossGym->GetWorldSettings()->DefaultGameMode=GM->GeneratedClass;BossGym->MarkPackageDirty();
	TArray<UObject*> ToSave={Boss,Proto,Set,Sweep,Shot,Counter,GASweep,GAShot,GACounter,CDSweep,CDShot,CDCounter,Shield,Projectile,V1,V2,V3,S1,S2,S3,WH,WR,GM,Registry,BossGym};FString Error;for(UObject* A:ToSave)if(!PRBossAuthoring::Save(A,Error)){R->SetError(Error);R->RemoveFromRoot();return R;}R->SetValue(TEXT("{\\\"status\\\":\\\"PASS\\\",\\\"created\\\":23,\\\"modified\\\":2,\\\"saved\\\":25}"));R->RemoveFromRoot();return R;
}

UToolCallAsyncResultString* UPRBossAuthoringToolset::RepairAndSaveAuditorBossManifest()
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->AddToRoot();

	UPREnemyPrototypeRegistryDataAsset* Registry = LoadObject<UPREnemyPrototypeRegistryDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/DA_EnemyPrototypeRegistry.DA_EnemyPrototypeRegistry"));
	UBlueprint* BossBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Blueprints/BP_Boss_Auditor.BP_Boss_Auditor"));
	UPRAuditorBossDataAsset* Prototype = LoadObject<UPRAuditorBossDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Prototypes/DA_Boss_Auditor.DA_Boss_Auditor"));
	UPRAbilitySetDataAsset* AbilitySet = LoadObject<UPRAbilitySetDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Abilities/DA_EnemyAbilitySet_AuditorBoss.DA_EnemyAbilitySet_AuditorBoss"));
	UPREnemyAttackDataAsset* Sweep = LoadObject<UPREnemyAttackDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorSweep.DA_EnemyAttack_AuditorSweep"));
	UPREnemyAttackDataAsset* LockShot = LoadObject<UPREnemyAttackDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorLockShot.DA_EnemyAttack_AuditorLockShot"));
	UPREnemyAttackDataAsset* Counter = LoadObject<UPREnemyAttackDataAsset>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/DA_EnemyAttack_AuditorCounter.DA_EnemyAttack_AuditorCounter"));
	UBlueprint* SweepAbility = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/GA_Enemy_AuditorSweep.GA_Enemy_AuditorSweep"));
	UBlueprint* LockShotAbility = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/GA_Enemy_AuditorLockShot.GA_Enemy_AuditorLockShot"));
	UBlueprint* CounterAbility = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Attacks/GA_Enemy_AuditorCounter.GA_Enemy_AuditorCounter"));
	UBlueprint* SweepCooldown = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorSweep_Cooldown.GE_EnemyAttack_AuditorSweep_Cooldown"));
	UBlueprint* LockShotCooldown = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorLockShot_Cooldown.GE_EnemyAttack_AuditorLockShot_Cooldown"));
	UBlueprint* CounterCooldown = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_EnemyAttack_AuditorCounter_Cooldown.GE_EnemyAttack_AuditorCounter_Cooldown"));
	UBlueprint* PredictionShield = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Effects/GE_Boss_Auditor_PredictionShield.GE_Boss_Auditor_PredictionShield"));
	UBlueprint* Projectile = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Blueprints/BP_EnemyProjectile_AuditorLockShot.BP_EnemyProjectile_AuditorLockShot"));
	UNiagaraSystem* SweepVFX = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/VFX/VFX_Boss_AuditorSweep.VFX_Boss_AuditorSweep"));
	UNiagaraSystem* LockShotVFX = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/VFX/VFX_Boss_AuditorLockShot.VFX_Boss_AuditorLockShot"));
	UNiagaraSystem* CounterVFX = LoadObject<UNiagaraSystem>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/VFX/VFX_Boss_AuditorCounter.VFX_Boss_AuditorCounter"));
	USoundWave* SweepSFX = LoadObject<USoundWave>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Audio/SFX_Boss_AuditorSweep.SFX_Boss_AuditorSweep"));
	USoundWave* LockShotSFX = LoadObject<USoundWave>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Audio/SFX_Boss_AuditorLockShot.SFX_Boss_AuditorLockShot"));
	USoundWave* CounterSFX = LoadObject<USoundWave>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Bosses/Audio/SFX_Boss_AuditorCounter.SFX_Boss_AuditorCounter"));
	UBlueprint* HealthWidget = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/UI/Boss/WBP_BossHealth.WBP_BossHealth"));
	UBlueprint* ResultWidget = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/UI/Boss/WBP_BossPrototypeResult.WBP_BossPrototypeResult"));
	UBlueprint* GameMode = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/GameFramework/BP_BossPrototypeGameMode.BP_BossPrototypeGameMode"));
	UBlueprint* PlayerController = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerController.BP_PlayerController"));
	UBlueprint* PlayerCharacter = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerCharacter.BP_PlayerCharacter"));
	UBlueprint* PlayerState = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Blueprints/Player/BP_PlayerState.BP_PlayerState"));
	UWorld* BossGym = LoadObject<UWorld>(nullptr, TEXT("/Game/ProjectR/Maps/L_BossGym.L_BossGym"));
	UBlueprint* DefaultAttributes = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes.GE_Enemy_DefaultAttributes"));
	UStateTree* EnemyStateTree = LoadObject<UStateTree>(nullptr,
		TEXT("/Game/ProjectR/Enemies/AI/ST_Enemy_Base.ST_Enemy_Base"));
	UClass* DamageEffectClass = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/ProjectR/Effects/GE_Damage.GE_Damage_C"));

	if (!Registry || !BossBlueprint || !Prototype || !AbilitySet || !Sweep || !LockShot || !Counter
		|| !SweepAbility || !LockShotAbility || !CounterAbility || !SweepCooldown || !LockShotCooldown || !CounterCooldown
		|| !PredictionShield || !Projectile || !SweepVFX || !LockShotVFX || !CounterVFX || !SweepSFX || !LockShotSFX || !CounterSFX
		|| !HealthWidget || !ResultWidget || !GameMode || !PlayerController || !PlayerCharacter || !PlayerState || !BossGym || !DefaultAttributes || !DefaultAttributes->GeneratedClass
		|| !EnemyStateTree || !DamageEffectClass)
	{
		Result->SetError(TEXT("Boss repair requires every fixed v0.2.2 manifest Package and the frozen Enemy dependencies."));
		Result->RemoveFromRoot();
		return Result;
	}
	if (!BossBlueprint->GeneratedClass || !BossBlueprint->GeneratedClass->IsChildOf(APRBossAuditor::StaticClass())
		|| !Projectile->GeneratedClass || !Projectile->GeneratedClass->IsChildOf(APREnemyProjectile::StaticClass())
		|| !HealthWidget->GeneratedClass || !HealthWidget->GeneratedClass->IsChildOf(UPRBossHealthWidget::StaticClass())
		|| !ResultWidget->GeneratedClass || !ResultWidget->GeneratedClass->IsChildOf(UPRBossPrototypeResultWidget::StaticClass())
		|| !GameMode->GeneratedClass || !GameMode->GeneratedClass->IsChildOf(APRBossPrototypeGameMode::StaticClass()))
	{
		Result->SetError(TEXT("Boss repair rejected an incompatible fixed Blueprint parent class."));
		Result->RemoveFromRoot();
		return Result;
	}

	const FGameplayTag BossTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.AuditorBoss"));
	const FGameplayTag SweepTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.AuditorSweep"));
	const FGameplayTag LockShotTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.AuditorLockShot"));
	const FGameplayTag CounterTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.AuditorCounter"));
	const FGameplayTag SweepCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Enemy.AuditorSweep"));
	const FGameplayTag LockShotCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Enemy.AuditorLockShot"));
	const FGameplayTag CounterCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Enemy.AuditorCounter"));
	const TArray<FGameplayTag> RequiredRegistryTags = {
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.MeleeMinion")),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.RangedMinion")),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.ShieldMinion")),
		FGameplayTag::RequestGameplayTag(TEXT("Enemy.Type.EliteAuditGuard")), BossTag};
	if (Registry->Entries.Num() != RequiredRegistryTags.Num())
	{
		Result->SetError(TEXT("Boss repair requires the exact four frozen Registry entries plus the existing fifth Auditor entry."));
		Result->RemoveFromRoot();
		return Result;
	}
	for (int32 Index = 0; Index < RequiredRegistryTags.Num(); ++Index)
	{
		if (Registry->Entries[Index].PrototypeTag != RequiredRegistryTags[Index])
		{
			Result->SetError(TEXT("Boss repair rejected a changed Registry order or identity."));
			Result->RemoveFromRoot();
			return Result;
		}
	}

	FString Error;
	if (!PRBossAuthoring::SetObjectProperty(BossBlueprint->GeneratedClass, TEXT("EnemyStateTree"), EnemyStateTree, Error)
		|| !PRBossAuthoring::SetClassProperty(BossBlueprint->GeneratedClass, TEXT("DefaultAttributesEffect"), DefaultAttributes->GeneratedClass, Error)
		|| !PRBossAuthoring::SetClassProperty(BossBlueprint->GeneratedClass, TEXT("DamageEffect"), DamageEffectClass, Error))
	{
		Result->SetError(Error);
		Result->RemoveFromRoot();
		return Result;
	}
	BossBlueprint->MarkPackageDirty();

	Prototype->EnemyId = TEXT("Auditor");
	Prototype->PrototypeTag = BossTag;
	Prototype->Mobility = EPRAbilityTargetMobility::Anchored;
	Prototype->Attributes = {900.0f, 900.0f, 120.0f, 120.0f, 0.0f, 0.0f, 18.0f, 320.0f, 0.0f, 0.0f, 0.0f};
	Prototype->Perception = {0.10f, 1600.0f, 1900.0f, 180.0f, 650.0f, 60.0f, 120.0f};
	Prototype->RuleAuditHealthRatio = 0.66f;
	Prototype->PredictionShieldHealthRatio = 0.33f;
	Prototype->PredictionShieldValue = 240.0f;
	Prototype->CounterproofFragmentsAwarded = 1;
	Prototype->AttackDefinitions = {Sweep, LockShot, Counter};
	Prototype->InitialAbilitySet = AbilitySet;
	Prototype->PredictionShieldEffect = PredictionShield->GeneratedClass;
	Prototype->MarkPackageDirty();

	PRBossAuthoring::ConfigureAttack(Sweep, TEXT("AuditorSweep"), TEXT("Enemy.Attack.AuditorSweep"), EPREnemyAttackKind::MeleeSweep,
		0.0f, 220.0f, 16.0f, 0.8f, 0.45f, 0.18f, 0.60f, 1.60f);
	Sweep->ProjectileClass.Reset();
	Sweep->ProjectileSpeed = 0.0f;
	Sweep->ProjectileLifetime = 0.0f;
	Sweep->DamageTags.Reset();
	Sweep->VFX = SweepVFX;
	Sweep->SFX = SweepSFX;
	Sweep->MarkPackageDirty();
	PRBossAuthoring::ConfigureAttack(LockShot, TEXT("AuditorLockShot"), TEXT("Enemy.Attack.AuditorLockShot"), EPREnemyAttackKind::Projectile,
		200.0f, 900.0f, 10.0f, 0.6f, 0.65f, 0.12f, 0.45f, 2.00f);
	LockShot->SweepRadius = 0.0f;
	LockShot->SweepHalfAngleDegrees = 0.0f;
	LockShot->ProjectileClass = Projectile->GeneratedClass;
	LockShot->ProjectileSpeed = 1200.0f;
	LockShot->ProjectileLifetime = 1.5f;
	LockShot->DamageTags.Reset();
	LockShot->VFX = LockShotVFX;
	LockShot->SFX = LockShotSFX;
	LockShot->MarkPackageDirty();
	PRBossAuthoring::ConfigureAttack(Counter, TEXT("AuditorCounter"), TEXT("Enemy.Attack.AuditorCounter"), EPREnemyAttackKind::MeleeSweep,
		0.0f, 300.0f, 20.0f, 0.9f, 0.80f, 0.18f, 0.80f, 3.00f);
	Counter->ProjectileClass.Reset();
	Counter->ProjectileSpeed = 0.0f;
	Counter->ProjectileLifetime = 0.0f;
	Counter->DamageTags.Reset();
	Counter->VFX = CounterVFX;
	Counter->SFX = CounterSFX;
	Counter->MarkPackageDirty();

	AbilitySet->AbilityEntries.Reset();
	auto AddAttackAbility = [AbilitySet](UBlueprint* Ability, UPREnemyAttackDataAsset* Attack)
	{
		FPRAbilitySetEntry& Entry = AbilitySet->AbilityEntries.AddDefaulted_GetRef();
		Entry.AbilityClass = Ability->GeneratedClass;
		Entry.AbilityData = Attack;
		Entry.AbilityLevel = 1;
		Entry.bGrantOnInitialization = true;
	};
	AddAttackAbility(SweepAbility, Sweep);
	AddAttackAbility(LockShotAbility, LockShot);
	AddAttackAbility(CounterAbility, Counter);
	AbilitySet->MarkPackageDirty();

	UGameplayEffect* SweepCooldownCDO = SweepCooldown->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	UGameplayEffect* LockShotCooldownCDO = LockShotCooldown->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	UGameplayEffect* CounterCooldownCDO = CounterCooldown->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	UGameplayEffect* PredictionShieldCDO = PredictionShield->GeneratedClass->GetDefaultObject<UGameplayEffect>();
	if (!SweepCooldownCDO || !LockShotCooldownCDO || !CounterCooldownCDO || !PredictionShieldCDO)
	{
		Result->SetError(TEXT("Boss repair could not access a fixed GameplayEffect CDO."));
		Result->RemoveFromRoot();
		return Result;
	}
	PRBossAuthoring::ConfigureCooldown(SweepCooldownCDO, SweepCooldownTag, 1.60f);
	PRBossAuthoring::ConfigureCooldown(LockShotCooldownCDO, LockShotCooldownTag, 2.00f);
	PRBossAuthoring::ConfigureCooldown(CounterCooldownCDO, CounterCooldownTag, 3.00f);
	PredictionShieldCDO->DurationPolicy = EGameplayEffectDurationType::Infinite;
	PredictionShieldCDO->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.0f));
	PredictionShieldCDO->Period = FScalableFloat(0.0f);
	PredictionShieldCDO->Modifiers.Reset();
	PredictionShieldCDO->Executions.Reset();
	PredictionShieldCDO->GameplayCues.Reset();
	PRBossAuthoring::AddOverrideModifier(PredictionShieldCDO, UPRAttributeSet::GetMaxShieldAttribute(), 240.0f);
	SweepCooldown->MarkPackageDirty();
	LockShotCooldown->MarkPackageDirty();
	CounterCooldown->MarkPackageDirty();
	PredictionShield->MarkPackageDirty();

	if (!PRBossAuthoring::SetAbilityTag(SweepAbility->GeneratedClass, SweepTag, Error)
		|| !PRBossAuthoring::SetClassProperty(SweepAbility->GeneratedClass, TEXT("CooldownGameplayEffectClass"), SweepCooldown->GeneratedClass, Error)
		|| !PRBossAuthoring::SetAbilityTag(LockShotAbility->GeneratedClass, LockShotTag, Error)
		|| !PRBossAuthoring::SetClassProperty(LockShotAbility->GeneratedClass, TEXT("CooldownGameplayEffectClass"), LockShotCooldown->GeneratedClass, Error)
		|| !PRBossAuthoring::SetAbilityTag(CounterAbility->GeneratedClass, CounterTag, Error)
		|| !PRBossAuthoring::SetClassProperty(CounterAbility->GeneratedClass, TEXT("CooldownGameplayEffectClass"), CounterCooldown->GeneratedClass, Error))
	{
		Result->SetError(Error);
		Result->RemoveFromRoot();
		return Result;
	}
	SweepAbility->MarkPackageDirty();
	LockShotAbility->MarkPackageDirty();
	CounterAbility->MarkPackageDirty();
	if (!PlayerController->GeneratedClass || !PlayerCharacter->GeneratedClass || !PlayerState->GeneratedClass
		|| !PRBossAuthoring::SetClassProperty(GameMode->GeneratedClass, TEXT("PlayerControllerClass"), PlayerController->GeneratedClass, Error)
		|| !PRBossAuthoring::SetClassProperty(GameMode->GeneratedClass, TEXT("DefaultPawnClass"), PlayerCharacter->GeneratedClass, Error)
		|| !PRBossAuthoring::SetClassProperty(GameMode->GeneratedClass, TEXT("PlayerStateClass"), PlayerState->GeneratedClass, Error))
	{
		Result->SetError(Error.IsEmpty() ? TEXT("Boss Prototype GameMode requires the frozen formal Player Blueprint classes.") : Error);
		Result->RemoveFromRoot();
		return Result;
	}
	GameMode->MarkPackageDirty();
	BossGym->GetWorldSettings()->DefaultGameMode = GameMode->GeneratedClass;
	BossGym->MarkPackageDirty();

	for (UBlueprint* Blueprint : {BossBlueprint, SweepAbility, LockShotAbility, CounterAbility, SweepCooldown, LockShotCooldown,
		CounterCooldown, PredictionShield, Projectile, HealthWidget, ResultWidget, GameMode})
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}

	TArray<UObject*> ToSave = {BossBlueprint, Prototype, AbilitySet, Sweep, LockShot, Counter, SweepAbility, LockShotAbility,
		CounterAbility, SweepCooldown, LockShotCooldown, CounterCooldown, PredictionShield, Projectile, SweepVFX, LockShotVFX,
		CounterVFX, SweepSFX, LockShotSFX, CounterSFX, HealthWidget, ResultWidget, GameMode, Registry, BossGym};
	for (UObject* Asset : ToSave)
	{
		if (!PRBossAuthoring::Save(Asset, Error))
		{
			Result->SetError(Error);
			Result->RemoveFromRoot();
			return Result;
		}
	}
	Result->SetValue(TEXT("{\"status\":\"PASS\",\"repaired\":23,\"modified\":2,\"saved\":25,\"fixedInput\":true}"));
	Result->RemoveFromRoot();
	return Result;
}

UToolCallAsyncResultString* UPRBossAuthoringToolset::ConfigureAndSaveBossGymGameMode()
{
	UToolCallAsyncResultString* R=NewObject<UToolCallAsyncResultString>();R->AddToRoot();
	UBlueprint* GM=LoadObject<UBlueprint>(nullptr,TEXT("/Game/ProjectR/GameFramework/BP_BossPrototypeGameMode.BP_BossPrototypeGameMode"));
	UWorld* BossGym=LoadObject<UWorld>(nullptr,TEXT("/Game/ProjectR/Maps/L_BossGym.L_BossGym"));
	if(!GM||!GM->GeneratedClass||!BossGym||!BossGym->GetWorldSettings()){R->SetError(TEXT("BossGym GameMode configuration requires its fixed saved Blueprint and map."));R->RemoveFromRoot();return R;}
	BossGym->GetWorldSettings()->DefaultGameMode=GM->GeneratedClass;BossGym->MarkPackageDirty();FString Error;if(!PRBossAuthoring::Save(BossGym,Error)){R->SetError(Error);R->RemoveFromRoot();return R;}R->SetValue(TEXT("{\\\"status\\\":\\\"PASS\\\",\\\"modified\\\":1,\\\"saved\\\":1}"));R->RemoveFromRoot();return R;
}

namespace PRBossAuthoring
{
class FPIEAuditorBossRunner : public TSharedFromThis<FPIEAuditorBossRunner>
{
public:
	static UToolCallAsyncResultString* Start()
	{
		TSharedRef<FPIEAuditorBossRunner> Runner = MakeShared<FPIEAuditorBossRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!PlayWorld || PlayWorld->GetNetMode() == NM_Client)
		{
			Runner->FinishError(TEXT("RunPIEAuditorBossSmoke requires an active authoritative in-process PIE world."));
			return Runner->Result.Get();
		}
		Runner->World = PlayWorld;
		Runner->StartedAt = PlayWorld->GetTimeSeconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Shared = Runner.ToSharedPtr()](float)
		{
			return Shared->Tick();
		}));
		return Runner->Result.Get();
	}

private:
	enum class EStep : uint8 { Resolve, SeedSampling, AwaitRule, ArmCounter, AwaitCounter, EnterPrediction, VerifyBlock, BreakShield, VerifyUnblocked, Defeat, VerifyCompletion };

	bool Tick()
	{
		UWorld* PlayWorld = World.Get();
		if (!PlayWorld || PlayWorld->GetTimeSeconds() - StartedAt > 25.0)
		{
			FinishError(*FString::Printf(TEXT("Fixed Auditor Boss smoke timed out at step %d (phase %d, rule %s, attack %s, health %.1f, shield %.1f)."),
				static_cast<int32>(Step), static_cast<int32>(LastState.Phase), *LastState.ActiveRuleId.ToString(), *LastState.ActiveAttackTag.ToString(), LastState.Health, LastState.Shield));
			return false;
		}
		if (Step == EStep::Resolve)
		{
			if (!ResolveRuntime()) return true;
			Step = EStep::SeedSampling;
			return true;
		}
		APRBossAuditor* BossActor = Boss.Get();
		UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		if (!BossActor || !Combat || !Player.IsValid())
		{
			FinishError(TEXT("Fixed Auditor Boss smoke lost its Boss, player, or CombatSubsystem."));
			return false;
		}
		const UPRAttributeSet* Attributes = BossActor->GetAttributeSet();
		UPRBossSubsystem* Bosses = PlayWorld->GetSubsystem<UPRBossSubsystem>();
		FPRBossRuntimeState State;
		Bosses ? Bosses->GetActiveBossState(State) : false;
		LastState = State;

		switch (Step)
		{
		case EStep::SeedSampling:
			// Three P1-only samples are recorded before the fourth hit crosses the P2 threshold.
			if (!Apply(TEXT("Skill.ShadowThrust"), 50.0f) || !Apply(TEXT("Skill.ShadowThrust"), 50.0f)
				|| !Apply(TEXT("Skill.ShadowThrust"), 50.0f) || !Apply(TEXT("Skill.ShadowThrust"), 280.0f))
				return FailFalse(TEXT("P1 fixed CombatSubsystem sampling requests were rejected."));
			Step = EStep::AwaitRule;
			return true;
		case EStep::AwaitRule:
			if (State.Phase != EPRAuditorBossPhase::RuleAudit) return true;
			if (!State.ActiveRuleId.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Rule.RepetitionPenalty"))))
				return FailFalse(TEXT("P2 did not deterministically select RepetitionPenalty from three identical Skill samples."));
			Step = EStep::ArmCounter;
			return true;
		case EStep::ArmCounter:
			if (!Apply(TEXT("Skill.ShadowThrust"), 1.0f)) return FailFalse(TEXT("P2 repetition trigger was rejected."));
			Step = EStep::AwaitCounter;
			return true;
		case EStep::AwaitCounter:
			if (const UPREnemyBrainComponent* Brain = BossActor->GetEnemyBrain(); Brain && Brain->GetRuntimeState().ActiveAttackTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Attack.AuditorCounter"))))
			{
				Step = EStep::EnterPrediction;
				return true;
			}
			return true;
		case EStep::EnterPrediction:
			if (!Apply(TEXT("Skill.FireSlash"), 300.0f)) return FailFalse(TEXT("Fixed P3 transition request was rejected."));
			Step = EStep::VerifyBlock;
			return true;
		case EStep::VerifyBlock:
			if (State.Phase != EPRAuditorBossPhase::PredictionShield || !State.PredictedAbilityTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Skill.ShadowThrust"))) || !Attributes || !FMath::IsNearlyEqual(Attributes->GetShield(), 240.0f)) return true;
			BeforeBlockedHealth = Attributes->GetHealth(); BeforeBlockedShield = Attributes->GetShield();
			if (ApplyStatus(TEXT("Skill.ShadowThrust"), 10.0f) != EPRCombatRequestStatus::RejectedBlocked || !FMath::IsNearlyEqual(Attributes->GetHealth(), BeforeBlockedHealth) || !FMath::IsNearlyEqual(Attributes->GetShield(), BeforeBlockedShield) || BlockedEvents != 1)
				return FailFalse(TEXT("Prediction Shield did not emit exactly one blocked DamageRejected without changing attributes."));
			Step = EStep::BreakShield;
			return true;
		case EStep::BreakShield:
			if (!Apply(TEXT("Skill.FireSlash"), 240.0f)) return FailFalse(TEXT("Non-predicted skill could not damage Prediction Shield."));
			Step = EStep::VerifyUnblocked;
			return true;
		case EStep::VerifyUnblocked:
			if (Attributes && Attributes->GetShield() > 0.0f) return true;
			if (!Apply(TEXT("Skill.ShadowThrust"), 10.0f)) return FailFalse(TEXT("Predicted Skill remained blocked after Prediction Shield was broken."));
			Step = EStep::Defeat;
			return true;
		case EStep::Defeat:
			if (!Apply(TEXT("Skill.ThunderDrop"), 1000.0f)) return FailFalse(TEXT("Fixed Boss defeat request was rejected."));
			Step = EStep::VerifyCompletion;
			return true;
		case EStep::VerifyCompletion:
			if (!State.bDefeated || CompletionEvents != 1 || CompletedFragments != 1) return true;
			FinishSuccess();
			return false;
		default: return true;
		}
	}

	bool ResolveRuntime()
	{
		UWorld* PlayWorld = World.Get();
		APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
		APawn* PlayerPawn = Controller ? Controller->GetPawn() : nullptr;
		if (!PlayerPawn) return false;
		for (TActorIterator<APRBossAuditor> It(PlayWorld); It; ++It)
		{
			if ((*It)->IsEnemyInitialized()) { Boss = *It; break; }
		}
		APRBossAuditor* BossActor = Boss.Get();
		if (!BossActor) return false;
		UPRAbilitySystemComponent* ASC = BossActor->GetProjectRAbilitySystemComponent();
		const UPRAttributeSet* Attributes = BossActor->GetAttributeSet();
		if (!ASC || !Attributes || ASC->GetOwnerActor() != BossActor || ASC->GetAvatarActor() != BossActor
			|| !FMath::IsNearlyEqual(Attributes->GetHealth(), 900.0f) || !FMath::IsNearlyEqual(Attributes->GetShield(), 120.0f))
		{
			FinishError(TEXT("Auditor ASC Owner/Avatar or frozen default attributes were invalid."));
			return false;
		}
		Player = PlayerPawn;
		PlayerStart = PlayerPawn->GetActorTransform();
		PlayerPawn->SetActorLocation(FVector(BossActor->GetActorLocation().X + 220.0f, BossActor->GetActorLocation().Y, BossActor->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
		if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>())
		{
			CombatEventHandle = Combat->OnCombatEvent().AddLambda([this](const FPRCombatEvent& Event)
			{
				if (Event.Target.Get() == Boss.Get() && Event.EventTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Combat.Event.DamageRejected")))
					&& Event.ResponseTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Combat.Response.PredictionBlocked")))) ++BlockedEvents;
			});
		}
		if (UPRBossSubsystem* Bosses = PlayWorld->GetSubsystem<UPRBossSubsystem>())
		{
			CompletionHandle = Bosses->OnPrototypeRunCompleted().AddLambda([this](const FPRPrototypeRunResult& Result)
			{
				++CompletionEvents; CompletedFragments = Result.CounterproofFragmentsAwarded;
			});
		}
		return true;
	}

	EPRCombatRequestStatus ApplyStatus(const TCHAR* AbilityTag, const float Damage)
	{
		UWorld* PlayWorld = World.Get(); APawn* PlayerPawn = Player.Get(); APRBossAuditor* BossActor = Boss.Get();
		UPRCombatSubsystem* Combat = PlayWorld ? PlayWorld->GetSubsystem<UPRCombatSubsystem>() : nullptr;
		if (!PlayWorld || !PlayerPawn || !BossActor || !Combat) return EPRCombatRequestStatus::Invalid;
		FPRDamageRequest Request; Request.SourceId = TEXT("AuditorBossFixedSmoke"); Request.DamageSource = PlayerPawn; Request.Instigator = PlayerPawn; Request.Target = BossActor;
		Request.AbilityTag = FGameplayTag::RequestGameplayTag(AbilityTag); Request.RawDamage = Damage; Request.ImpactOrigin = PlayerPawn->GetActorLocation();
		Request.IncomingDirection = (BossActor->GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal();
		return Combat->ApplyDamage(Request);
	}
	bool Apply(const TCHAR* AbilityTag, const float Damage) { return ApplyStatus(AbilityTag, Damage) == EPRCombatRequestStatus::Applied; }
	bool FailFalse(const TCHAR* Message) { FinishError(Message); return false; }
	void Cleanup()
	{
		if (UWorld* PlayWorld = World.Get())
		{
			if (UPRCombatSubsystem* Combat = PlayWorld->GetSubsystem<UPRCombatSubsystem>(); Combat && CombatEventHandle.IsValid()) Combat->OnCombatEvent().Remove(CombatEventHandle);
			if (UPRBossSubsystem* Bosses = PlayWorld->GetSubsystem<UPRBossSubsystem>(); Bosses && CompletionHandle.IsValid()) Bosses->OnPrototypeRunCompleted().Remove(CompletionHandle);
		}
		if (APawn* PlayerPawn = Player.Get()) PlayerPawn->SetActorTransform(PlayerStart, false, nullptr, ETeleportType::TeleportPhysics);
		CombatEventHandle.Reset(); CompletionHandle.Reset();
	}
	void FinishSuccess()
	{
		Cleanup();
		if (Result.IsValid()) { Result->SetValue(TEXT("{\"status\":\"PASS\",\"boss\":\"Auditor\",\"p1Sampling\":true,\"p2Counter\":true,\"p3Prediction\":true,\"completionFragments\":1,\"runtimeClean\":true}")); Result->RemoveFromRoot(); }
	}
	void FinishError(const TCHAR* Message)
	{
		Cleanup(); if (Result.IsValid()) { Result->SetError(Message); Result->RemoveFromRoot(); }
	}

	TWeakObjectPtr<UWorld> World; TWeakObjectPtr<APRBossAuditor> Boss; TWeakObjectPtr<APawn> Player; FTransform PlayerStart;
	TStrongObjectPtr<UToolCallAsyncResultString> Result; FDelegateHandle CombatEventHandle; FDelegateHandle CompletionHandle;
	double StartedAt = 0.0; float BeforeBlockedHealth = 0.0f; float BeforeBlockedShield = 0.0f;
	int32 BlockedEvents = 0; int32 CompletionEvents = 0; int32 CompletedFragments = 0; EStep Step = EStep::Resolve;
	FPRBossRuntimeState LastState;
};
}

UToolCallAsyncResultString* UPRBossAuthoringToolset::RunPIEAuditorBossSmoke()
{
	return PRBossAuthoring::FPIEAuditorBossRunner::Start();
}
