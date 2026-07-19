// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemies/PREnemyAttackDataAsset.h"

#include "Enemies/PREnemyProjectile.h"
#include "Misc/DataValidation.h"

FPrimaryAssetId UPREnemyAttackDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("ProjectREnemyAttack")), GetFName());
}

#if WITH_EDITOR
EDataValidationResult UPREnemyAttackDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const bool bFinite = FMath::IsFinite(MinRange) && FMath::IsFinite(MaxRange)
		&& FMath::IsFinite(BaseDamage) && FMath::IsFinite(AttackPowerScale)
		&& FMath::IsFinite(Windup) && FMath::IsFinite(ActiveWindow) && FMath::IsFinite(Recovery)
		&& FMath::IsFinite(Cooldown) && FMath::IsFinite(SweepRadius)
		&& FMath::IsFinite(SweepHalfAngleDegrees) && FMath::IsFinite(ProjectileSpeed)
		&& FMath::IsFinite(ProjectileLifetime);
	if (AttackId.IsNone() || !AttackTag.IsValid() || !AttackTag.ToString().StartsWith(TEXT("Enemy.Attack."))
		|| !bFinite || MinRange < 0.0f || MaxRange <= MinRange || BaseDamage < 0.0f
		|| AttackPowerScale < 0.0f || Windup < 0.0f || ActiveWindow < 0.0f || Recovery < 0.0f
		|| Cooldown < 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("Enemy attack has invalid identity, range, damage, or phase values.")));
		Result = EDataValidationResult::Invalid;
	}
	if (Kind == EPREnemyAttackKind::MeleeSweep
		&& (SweepRadius <= 0.0f || SweepHalfAngleDegrees <= 0.0f || SweepHalfAngleDegrees > 180.0f))
	{
		Context.AddError(FText::FromString(TEXT("Melee attacks require a positive sweep radius and half angle in (0, 180].")));
		Result = EDataValidationResult::Invalid;
	}
	if (Kind == EPREnemyAttackKind::Projectile
		&& (ProjectileClass.IsNull() || ProjectileSpeed <= 0.0f || ProjectileLifetime <= 0.0f))
	{
		Context.AddError(FText::FromString(TEXT("Projectile attacks require a class, speed, and lifetime.")));
		Result = EDataValidationResult::Invalid;
	}
	if (Kind == EPREnemyAttackKind::Projectile && !ProjectileClass.IsNull())
	{
		const UClass* LoadedProjectileClass = ProjectileClass.LoadSynchronous();
		if (!LoadedProjectileClass || !LoadedProjectileClass->IsChildOf(APREnemyProjectile::StaticClass()))
		{
			Context.AddError(FText::FromString(TEXT("Projectile attacks require an APREnemyProjectile class.")));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
