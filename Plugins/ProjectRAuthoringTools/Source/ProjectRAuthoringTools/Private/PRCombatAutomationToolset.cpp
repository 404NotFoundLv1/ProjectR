// Copyright Epic Games, Inc. All Rights Reserved.

#include "PRCombatAutomationToolset.h"

#include "Characters/PRPlayerCharacter.h"
#include "Combat/PRCombatSubsystem.h"
#include "Containers/Ticker.h"
#include "Core/PRPlayerController.h"
#include "Core/PRPlayerState.h"
#include "Core/PRTagLibrary.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace PRCombatAutomationToolset
{
enum class ESmokePhase : uint8
{
	ShieldHit,
	WaitShieldReaction,
	SpillOver,
	WaitSpillReaction,
	InvulnerableRejection,
	FatalHit,
	DeadRejection,
	Revive,
	Complete
};

class FPIECombatSmokeRunner : public TSharedFromThis<FPIECombatSmokeRunner>
{
public:
	static UToolCallAsyncResultString* Start(float StepDelaySeconds, float HitReactionToleranceSeconds)
	{
		TSharedRef<FPIECombatSmokeRunner> Runner = MakeShared<FPIECombatSmokeRunner>();
		Runner->Result = TStrongObjectPtr<UToolCallAsyncResultString>(NewObject<UToolCallAsyncResultString>());
		Runner->StepDelay = FMath::Clamp(StepDelaySeconds, 0.20f, 5.0f);
		Runner->HitReactionTolerance = FMath::Clamp(HitReactionToleranceSeconds, 0.0f, 0.10f);
		if (!Runner->Initialize())
		{
			return Runner->Result.Get();
		}

		Runner->PhaseStartTime = Runner->GetPlayWorldTimeSeconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[RunnerPtr = Runner.ToSharedPtr()](float DeltaSeconds) mutable
			{
				const bool bContinue = RunnerPtr->Tick(DeltaSeconds);
				if (!bContinue)
				{
					RunnerPtr.Reset();
				}
				return bContinue;
			}));
		return Runner->Result.Get();
	}

	~FPIECombatSmokeRunner()
	{
		Cleanup();
	}

private:
	bool Initialize()
	{
		UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
		if (!IsValid(PlayWorld) || PlayWorld->GetNetMode() == NM_Client)
		{
			Fail(TEXT("RunPIECombatSmoke requires an active authoritative in-process PIE world."));
			return false;
		}

		Controller = Cast<APRPlayerController>(PlayWorld->GetFirstPlayerController());
		Character = Controller.IsValid() ? Cast<APRPlayerCharacter>(Controller->GetPawn()) : nullptr;
		PlayerState = Character.IsValid() ? Character->GetPlayerState<APRPlayerState>() : nullptr;
		Subsystem = PlayWorld->GetSubsystem<UPRCombatSubsystem>();
		if (!Controller.IsValid() || !Character.IsValid() || !PlayerState.IsValid()
			|| !Subsystem.IsValid())
		{
			Fail(TEXT("PIE must contain one possessed ProjectR Character, PlayerState, ASC, and CombatSubsystem."));
			return false;
		}

		EventHandle = Subsystem->OnCombatEvent().AddLambda(
			[WeakThis = TWeakPtr<FPIECombatSmokeRunner>(AsShared())](const FPRCombatEvent& Event)
			{
				if (TSharedPtr<FPIECombatSmokeRunner> This = WeakThis.Pin())
				{
					This->Events.Add(Event);
				}
			});
		Phase = ESmokePhase::ShieldHit;
		return true;
	}

	bool Tick(float DeltaSeconds)
	{
		if (!Character.IsValid() || !PlayerState.IsValid()
			|| !Subsystem.IsValid() || !GEditor || GEditor->PlayWorld != Character->GetWorld())
		{
			Fail(TEXT("PIE ended or ProjectR combat objects became invalid during combat smoke."));
			return false;
		}

		const double Elapsed = GetPlayWorldTimeSeconds() - PhaseStartTime;
		switch (Phase)
		{
		case ESmokePhase::ShieldHit:
			if (!ApplyAndCheckDamage(20.0f, EPRCombatRequestStatus::Applied, 100.0f, 30.0f))
			{
				return false;
			}
			ReactionStartTime = GetPlayWorldTimeSeconds();
			Advance(ESmokePhase::WaitShieldReaction);
			break;

		case ESmokePhase::WaitShieldReaction:
			if (!MeasureReaction(Elapsed, ShieldReactionSeconds, ESmokePhase::SpillOver))
			{
				return false;
			}
			break;

		case ESmokePhase::SpillOver:
			if (Elapsed >= StepDelay)
			{
				if (!ApplyAndCheckDamage(40.0f, EPRCombatRequestStatus::Applied, 90.0f, 0.0f))
				{
					return false;
				}
				ReactionStartTime = GetPlayWorldTimeSeconds();
				Advance(ESmokePhase::WaitSpillReaction);
			}
			break;

		case ESmokePhase::WaitSpillReaction:
			if (!MeasureReaction(Elapsed, SpillReactionSeconds, ESmokePhase::InvulnerableRejection))
			{
				return false;
			}
			break;

		case ESmokePhase::InvulnerableRejection:
			if (Elapsed >= StepDelay)
			{
				if (!ModifyGameplayTag(true))
				{
					Fail(TEXT("Combat smoke could not add the temporary State.Invulnerable tag."));
					return false;
				}
				bTemporaryInvulnerableApplied = true;
				if (!ApplyAndCheckDamage(10.0f, EPRCombatRequestStatus::RejectedInvulnerable, 90.0f, 0.0f))
				{
					return false;
				}
				RestoreInvulnerable();
				Advance(ESmokePhase::FatalHit);
			}
			break;

		case ESmokePhase::FatalHit:
			if (Elapsed >= StepDelay)
			{
				if (!ApplyAndCheckDamage(200000.0f, EPRCombatRequestStatus::Applied, 0.0f, 0.0f)
					|| Character->GetCharacterMovement()->MovementMode != MOVE_None)
				{
					Fail(TEXT("Fatal stage did not enter the exact Dead movement-locked state."));
					return false;
				}
				Advance(ESmokePhase::DeadRejection);
			}
			break;

		case ESmokePhase::DeadRejection:
			if (Elapsed >= StepDelay)
			{
				if (!ApplyAndCheckDamage(1.0f, EPRCombatRequestStatus::RejectedDead, 0.0f, 0.0f))
				{
					return false;
				}
				Advance(ESmokePhase::Revive);
			}
			break;

		case ESmokePhase::Revive:
			if (Elapsed >= StepDelay)
			{
				FPRReviveRequest Request;
				Request.SourceId = TEXT("MCP.CombatSmoke");
				Request.DamageSource = Character;
				Request.Instigator = Character;
				Request.Target = Character;
				const EPRCombatRequestStatus Status = Subsystem->Revive(Request);
				if (Status != EPRCombatRequestStatus::Applied
					|| Character->GetCharacterMovement()->MovementMode != MOVE_Walking)
				{
					Fail(TEXT("Revive stage did not restore the exact full Alive walking state."));
					return false;
				}
				Advance(ESmokePhase::Complete);
			}
			break;

		case ESmokePhase::Complete:
			return Finish();
		}
		return true;
	}

	bool ApplyAndCheckDamage(
		float RawDamage,
		EPRCombatRequestStatus ExpectedStatus,
		float ExpectedHealth,
		float ExpectedShield)
	{
		const int32 FirstNewEvent = Events.Num();
		FPRDamageRequest Request;
		Request.SourceId = TEXT("MCP.CombatSmoke");
		Request.DamageSource = Character;
		Request.Instigator = Character;
		Request.Target = Character;
		Request.RawDamage = RawDamage;
		const EPRCombatRequestStatus Status = Subsystem->ApplyDamage(Request);
		const FGameplayTag ExpectedEventTag = ExpectedStatus == EPRCombatRequestStatus::Applied
			? UPRTagLibrary::GetCombatEventDamageTag()
			: UPRTagLibrary::GetCombatEventDamageRejectedTag();
		const FPRCombatEvent* ObservedEvent = nullptr;
		for (int32 Index = FirstNewEvent; Index < Events.Num(); ++Index)
		{
			if (Events[Index].EventTag == ExpectedEventTag)
			{
				ObservedEvent = &Events[Index];
				break;
			}
		}
		if (Status != ExpectedStatus
			|| !ObservedEvent
			|| !FMath::IsNearlyEqual(ObservedEvent->RemainingHealth, ExpectedHealth)
			|| !FMath::IsNearlyEqual(ObservedEvent->RemainingShield, ExpectedShield))
		{
			Fail(FString::Printf(
				TEXT("Damage stage %.3f failed: status=%d eventHealth=%.3f eventShield=%.3f."),
				RawDamage,
				static_cast<int32>(Status),
				ObservedEvent ? ObservedEvent->RemainingHealth : -1.0f,
				ObservedEvent ? ObservedEvent->RemainingShield : -1.0f));
			return false;
		}
		return true;
	}

	bool MeasureReaction(double PhaseElapsed, double& OutDuration, ESmokePhase NextPhase)
	{
		if (Character->GetCharacterMovement()->MovementMode != MOVE_None)
		{
			OutDuration = GetPlayWorldTimeSeconds() - ReactionStartTime;
			const double Minimum = 0.10 - HitReactionTolerance;
			const double Maximum = 0.10 + HitReactionTolerance;
			if (OutDuration < Minimum || OutDuration > Maximum)
			{
				Fail(FString::Printf(
					TEXT("Hit reaction duration %.4f was outside [%.4f, %.4f]."),
					OutDuration, Minimum, Maximum));
				return false;
			}
			Advance(NextPhase);
		}
		else if (PhaseElapsed > 0.10 + HitReactionTolerance + 0.50)
		{
			Fail(TEXT("Hit reaction did not restore movement before its timeout."));
			return false;
		}
		return true;
	}

	bool Finish()
	{
		const FGameplayTag ExpectedOrder[] = {
			UPRTagLibrary::GetCombatEventDamageTag(),
			UPRTagLibrary::GetCombatEventDamageTag(),
			UPRTagLibrary::GetCombatEventDamageRejectedTag(),
			UPRTagLibrary::GetCombatEventDamageTag(),
			UPRTagLibrary::GetCombatEventDeathTag(),
			UPRTagLibrary::GetCombatEventDamageRejectedTag(),
			UPRTagLibrary::GetCombatEventReviveTag()};
		bool bEventsValid = Events.Num() == static_cast<int32>(UE_ARRAY_COUNT(ExpectedOrder));
		TSet<FGuid> EventIds;
		TArray<TSharedPtr<FJsonValue>> EventJson;
		for (int32 Index = 0; Index < Events.Num(); ++Index)
		{
			if (Index >= static_cast<int32>(UE_ARRAY_COUNT(ExpectedOrder))
				|| Events[Index].EventTag != ExpectedOrder[Index]
				|| !Events[Index].EventId.IsValid())
			{
				bEventsValid = false;
			}
			EventIds.Add(Events[Index].EventId);
			TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
			Item->SetStringField(TEXT("eventId"), Events[Index].EventId.ToString());
			Item->SetStringField(TEXT("eventTag"), Events[Index].EventTag.ToString());
			Item->SetNumberField(TEXT("rawDamage"), Events[Index].RawDamage);
			Item->SetNumberField(TEXT("shieldAbsorbed"), Events[Index].ShieldAbsorbed);
			Item->SetNumberField(TEXT("healthDamage"), Events[Index].HealthDamage);
			Item->SetBoolField(TEXT("fatal"), Events[Index].bFatal);
			EventJson.Add(MakeShared<FJsonValueObject>(Item));
		}
		bEventsValid = bEventsValid && EventIds.Num() == Events.Num();

		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("status"), bEventsValid ? TEXT("PASS") : TEXT("FAIL"));
		Json->SetStringField(TEXT("sourceId"), TEXT("MCP.CombatSmoke"));
		Json->SetNumberField(TEXT("shieldReactionSeconds"), ShieldReactionSeconds);
		Json->SetNumberField(TEXT("spillReactionSeconds"), SpillReactionSeconds);
		const FPRCombatEvent* FinalEvent = Events.IsEmpty() ? nullptr : &Events.Last();
		Json->SetNumberField(TEXT("health"), FinalEvent ? FinalEvent->RemainingHealth : -1.0f);
		Json->SetNumberField(TEXT("shield"), FinalEvent ? FinalEvent->RemainingShield : -1.0f);
		Json->SetBoolField(TEXT("alive"), FinalEvent && FinalEvent->ResponseTags.HasTagExact(UPRTagLibrary::GetStateAliveTag()));
		Json->SetBoolField(TEXT("dead"), false);
		Json->SetNumberField(TEXT("movementMode"), Character->GetCharacterMovement()->MovementMode);
		Json->SetArrayField(TEXT("events"), EventJson);

		FString JsonString;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
		FJsonSerializer::Serialize(Json, Writer);
		Cleanup();
		if (bEventsValid)
		{
			Result->SetValue(JsonString);
		}
		else
		{
			Result->SetError(JsonString);
		}
		return false;
	}

	bool ModifyGameplayTag(bool bAdd)
	{
		UClass* LibraryClass = LoadObject<UClass>(
			nullptr, TEXT("/Script/GameplayAbilities.AbilitySystemBlueprintLibrary"));
		UObject* LibraryCDO = LibraryClass ? LibraryClass->GetDefaultObject() : nullptr;
		UFunction* Function = LibraryCDO
			? LibraryCDO->FindFunction(bAdd ? TEXT("AddGameplayTags") : TEXT("RemoveGameplayTags"))
			: nullptr;
		if (!Function)
		{
			return false;
		}

		FStructOnScope Parameters(Function);
		FObjectPropertyBase* ActorProperty = FindFProperty<FObjectPropertyBase>(Function, TEXT("Actor"));
		FStructProperty* TagsProperty = FindFProperty<FStructProperty>(Function, TEXT("GameplayTags"));
		FProperty* ReplicationProperty = Function->FindPropertyByName(TEXT("ReplicationRule"));
		FBoolProperty* ReturnProperty = FindFProperty<FBoolProperty>(Function, TEXT("ReturnValue"));
		if (!ActorProperty || !TagsProperty || !ReplicationProperty || !ReturnProperty)
		{
			return false;
		}

		void* Memory = Parameters.GetStructMemory();
		ActorProperty->SetObjectPropertyValue_InContainer(Memory, Character.Get());
		FArrayProperty* TagsArrayProperty = FindFProperty<FArrayProperty>(
			TagsProperty->Struct, TEXT("GameplayTags"));
		FStructProperty* TagProperty = TagsArrayProperty
			? CastField<FStructProperty>(TagsArrayProperty->Inner)
			: nullptr;
		if (!TagsArrayProperty || !TagProperty)
		{
			return false;
		}
		void* TagsMemory = TagsProperty->ContainerPtrToValuePtr<void>(Memory);
		FScriptArrayHelper TagsArray(TagsArrayProperty, TagsMemory);
		const int32 TagIndex = TagsArray.AddValue();
		const FGameplayTag& InvulnerableTag = UPRTagLibrary::GetStateInvulnerableTag();
		TagProperty->CopyCompleteValue(TagsArray.GetRawPtr(TagIndex), &InvulnerableTag);
		if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(ReplicationProperty))
		{
			EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(
				EnumProperty->ContainerPtrToValuePtr<void>(Memory),
				static_cast<int64>(EGameplayTagReplicationState::TagOnly));
		}
		else if (FByteProperty* ByteProperty = CastField<FByteProperty>(ReplicationProperty))
		{
			ByteProperty->SetPropertyValue_InContainer(
				Memory, static_cast<uint8>(EGameplayTagReplicationState::TagOnly));
		}
		else
		{
			return false;
		}

		LibraryCDO->ProcessEvent(Function, Memory);
		return ReturnProperty->GetPropertyValue_InContainer(Memory);
	}

	void Advance(ESmokePhase NextPhase)
	{
		Phase = NextPhase;
		PhaseStartTime = GetPlayWorldTimeSeconds();
	}

	double GetPlayWorldTimeSeconds() const
	{
		const UWorld* PlayWorld = Character.IsValid() ? Character->GetWorld() : nullptr;
		return IsValid(PlayWorld) ? PlayWorld->GetTimeSeconds() : 0.0;
	}

	void RestoreInvulnerable()
	{
		if (bTemporaryInvulnerableApplied && Character.IsValid())
		{
			ModifyGameplayTag(false);
			bTemporaryInvulnerableApplied = false;
		}
	}

	void Cleanup()
	{
		RestoreInvulnerable();
		if (Subsystem.IsValid() && EventHandle.IsValid())
		{
			Subsystem->OnCombatEvent().Remove(EventHandle);
			EventHandle.Reset();
		}
	}

	void Fail(const FString& Message)
	{
		Cleanup();
		if (Result.IsValid())
		{
			Result->SetError(Message);
		}
	}

	TStrongObjectPtr<UToolCallAsyncResultString> Result;
	TWeakObjectPtr<APRPlayerController> Controller;
	TWeakObjectPtr<APRPlayerCharacter> Character;
	TWeakObjectPtr<APRPlayerState> PlayerState;
	TWeakObjectPtr<UPRCombatSubsystem> Subsystem;
	FDelegateHandle EventHandle;
	TArray<FPRCombatEvent> Events;
	ESmokePhase Phase = ESmokePhase::ShieldHit;
	double PhaseStartTime = 0.0;
	double ReactionStartTime = 0.0;
	double ShieldReactionSeconds = 0.0;
	double SpillReactionSeconds = 0.0;
	float StepDelay = 0.35f;
	float HitReactionTolerance = 0.03f;
	bool bTemporaryInvulnerableApplied = false;
};
} // namespace PRCombatAutomationToolset

UToolCallAsyncResultString* UPRCombatAutomationToolset::RunPIECombatSmoke(
	float StepDelaySeconds,
	float HitReactionToleranceSeconds)
{
	return PRCombatAutomationToolset::FPIECombatSmokeRunner::Start(
		StepDelaySeconds,
		HitReactionToleranceSeconds);
}
