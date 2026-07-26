// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/** Stable, data-driven Director identifiers; execution never accepts arbitrary tags. */
namespace FPRDirectorRuleEffectContract
{
	enum class EEffectTarget : uint8
	{
		None,
		PlayerAttackPower,
		PlayerMaxHealth,
		PlayerMaxEnergy,
		EnemyAttackPower,
		EnemyMoveSpeed,
		EnemyArmor,
		CompanionSupport
	};

	inline FGameplayTag GetRuleTag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name), false);
	}
	inline TArray<FGameplayTag> GetRequiredRuleIds()
	{
		static const TCHAR* const Names[] =
		{
			TEXT("Rule.CompanionIsolation"),
			TEXT("Rule.CooperationAudit"),
			TEXT("Rule.DeleteEcho"),
			TEXT("Rule.DistanceCorrection"),
			TEXT("Rule.EmotionalInterference"),
			TEXT("Rule.ObedienceTest"),
			TEXT("Rule.OptimalPath"),
			TEXT("Rule.PredictionLock"),
			TEXT("Rule.RepetitionPenalty"),
			TEXT("Rule.ResourceBalance"),
			TEXT("Rule.RiskReward"),
			TEXT("Rule.SurvivalProtocol")
		};

		TArray<FGameplayTag> Result;
		Result.Reserve(UE_ARRAY_COUNT(Names));
		for (const TCHAR* Name : Names)
		{
			Result.Add(FGameplayTag::RequestGameplayTag(FName(Name), false));
		}
		return Result;
	}

	inline bool IsRequiredRuleId(const FGameplayTag RuleId)
	{
		return GetRequiredRuleIds().Contains(RuleId);
	}

	inline TArray<EEffectTarget> GetEffectTargets(const FGameplayTag RuleId)
	{
		const FString Name = RuleId.ToString();
		if (Name == TEXT("Rule.RepetitionPenalty") || Name == TEXT("Rule.SurvivalProtocol")
			|| Name == TEXT("Rule.DistanceCorrection") || Name == TEXT("Rule.PredictionLock"))
		{
			return { EEffectTarget::PlayerAttackPower };
		}
		if (Name == TEXT("Rule.CooperationAudit")) return { EEffectTarget::EnemyArmor };
		if (Name == TEXT("Rule.DeleteEcho")) return { EEffectTarget::EnemyAttackPower };
		if (Name == TEXT("Rule.OptimalPath")) return { EEffectTarget::EnemyMoveSpeed };
		if (Name == TEXT("Rule.EmotionalInterference") || Name == TEXT("Rule.CompanionIsolation")) return { EEffectTarget::CompanionSupport };
		if (Name == TEXT("Rule.ResourceBalance")) return { EEffectTarget::PlayerMaxEnergy };
		if (Name == TEXT("Rule.RiskReward")) return { EEffectTarget::PlayerAttackPower, EEffectTarget::PlayerMaxHealth };
		if (Name == TEXT("Rule.ObedienceTest")) return { EEffectTarget::PlayerAttackPower, EEffectTarget::PlayerMaxEnergy };
		return {};
	}

	inline const TCHAR* GetEffectAssetPath(const EEffectTarget Target)
	{
		switch (Target)
		{
		case EEffectTarget::PlayerAttackPower: return TEXT("/Game/ProjectR/Effects/Director/GE_Director_PlayerAttackPower.GE_Director_PlayerAttackPower_C");
		case EEffectTarget::PlayerMaxHealth: return TEXT("/Game/ProjectR/Effects/Director/GE_Director_PlayerMaxHealth.GE_Director_PlayerMaxHealth_C");
		case EEffectTarget::PlayerMaxEnergy: return TEXT("/Game/ProjectR/Effects/Director/GE_Director_PlayerMaxEnergy.GE_Director_PlayerMaxEnergy_C");
		case EEffectTarget::EnemyAttackPower: return TEXT("/Game/ProjectR/Effects/Director/GE_Director_EnemyAttackPower.GE_Director_EnemyAttackPower_C");
		case EEffectTarget::EnemyMoveSpeed: return TEXT("/Game/ProjectR/Effects/Director/GE_Director_EnemyMoveSpeed.GE_Director_EnemyMoveSpeed_C");
		case EEffectTarget::EnemyArmor: return TEXT("/Game/ProjectR/Effects/Director/GE_Director_EnemyArmor.GE_Director_EnemyArmor_C");
		default: return TEXT("");
		}
	}

	inline int32 GetCounterTarget(const FGameplayTag RuleId)
	{
		const FString Name = RuleId.ToString();
		if (Name == TEXT("Rule.CooperationAudit") || Name == TEXT("Rule.EmotionalInterference") || Name == TEXT("Rule.CompanionIsolation")) return 2;
		if (Name == TEXT("Rule.DistanceCorrection") || Name == TEXT("Rule.PredictionLock") || Name == TEXT("Rule.RiskReward")) return 3;
		return 1;
	}

	inline FText GetEffectDescription(const FGameplayTag RuleId, const int32 Level)
	{
		const FString Name = RuleId.ToString();
		if (Name == TEXT("Rule.RepetitionPenalty")) return FText::FromString(TEXT("Repeated P0 skill damage is reduced."));
		if (Name == TEXT("Rule.CooperationAudit")) return FText::FromString(TEXT("Enemies receive a temporary shield buffer after failed QTE results."));
		if (Name == TEXT("Rule.SurvivalProtocol")) return FText::FromString(TEXT("Low health raises player attack power."));
		if (Name == TEXT("Rule.DistanceCorrection")) return FText::FromString(TEXT("Long-range damage is corrected downward."));
		if (Name == TEXT("Rule.DeleteEcho")) return FText::FromString(TEXT("A death echo temporarily raises enemy attack power."));
		if (Name == TEXT("Rule.OptimalPath")) return FText::FromString(TEXT("Safe long-range play raises enemy movement speed."));
		if (Name == TEXT("Rule.EmotionalInterference")) return FText::FromString(TEXT("Low relationship slows companion support."));
		if (Name == TEXT("Rule.CompanionIsolation")) return FText::FromString(TEXT("Companion support attempts are deterministically suppressed."));
		if (Name == TEXT("Rule.PredictionLock")) return FText::FromString(TEXT("The predicted P0 skill is weakened when repeated."));
		if (Name == TEXT("Rule.ResourceBalance")) return FText::FromString(TEXT("Sustained high energy reduces maximum energy."));
		if (Name == TEXT("Rule.RiskReward")) return FText::FromString(TEXT("Attack power rises while maximum health is reduced."));
		if (Name == TEXT("Rule.ObedienceTest")) return FText::FromString(TEXT("Attack power rises while maximum energy is reduced."));
		return FText::FromString(FString::Printf(TEXT("Director rule level %d."), Level));
	}
}
