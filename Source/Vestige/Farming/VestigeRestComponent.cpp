// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/VestigeRestComponent.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Content/VestigeGameData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeSaveRegistry.h"
#include "Engine/World.h"
#include "Farming/FarmLog.h"
#include "Farming/VestigeRestSaveData.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Time/VestigeClockSubsystem.h"

namespace VestigeRest
{
	/** Stamina effects have no levels, so they are applied at the ability system's default level. */
	constexpr float EffectLevel = 1.f;

	/** A full refill, after a night in bed before the late night starts. */
	constexpr float FullRefillFraction = 1.f;
}

UVestigeRestComponent::UVestigeRestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UVestigeRestComponent* UVestigeRestComponent::Find(const AActor* PlayerActor)
{
	if (!PlayerActor)
	{
		return nullptr;
	}
	if (UVestigeRestComponent* Own = PlayerActor->FindComponentByClass<UVestigeRestComponent>())
	{
		return Own;
	}
	const APawn* Pawn = Cast<APawn>(PlayerActor);
	const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
	return PlayerState ? PlayerState->FindComponentByClass<UVestigeRestComponent>() : nullptr;
}

void UVestigeRestComponent::BeginPlay()
{
	Super::BeginPlay();

	Clock = UVestigeClockSubsystem::Get(GetWorld());
	if (Clock)
	{
		Clock->OnHourStarted.AddUObject(this, &UVestigeRestComponent::ReactToHour);
		Clock->OnDayEnded.AddUObject(this, &UVestigeRestComponent::EndNight);
		Clock->OnNewDay.AddUObject(this, &UVestigeRestComponent::WakeUp);

		// A map opened after midnight never hears the hour the drain starts at, so it starts here.
		if (Clock->GetTimeOfDay() >= LateNightHour)
		{
			StartLateNightDrain();
		}
	}
	UVestigeSaveRegistry::RegisterWorldOwner(GetWorld(), *this);
}

void UVestigeRestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UVestigeSaveRegistry::UnregisterWorldOwner(GetWorld(), *this);

	// The drain runs on the ability system, which may outlive this component.
	StopLateNightDrain();
	if (Clock)
	{
		Clock->OnHourStarted.RemoveAll(this);
		Clock->OnDayEnded.RemoveAll(this);
		Clock->OnNewDay.RemoveAll(this);
		Clock = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UVestigeRestComponent::Sleep()
{
	if (Clock)
	{
		Clock->EndDay();
	}
}

bool UVestigeRestComponent::IsExhausted() const
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(VestigeGameplayTags::State_Exhausted);
}

FVestigeRestSaveData UVestigeRestComponent::GetSaveData() const
{
	FVestigeRestSaveData Data;
	Data.bExhausted = IsExhausted();
	return Data;
}

void UVestigeRestComponent::RestoreFromSaveData(const FVestigeRestSaveData& Data)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	if (!Data.bExhausted || !AbilitySystem || IsExhausted())
	{
		return;
	}
	ApplyExhaustion(*AbilitySystem);
}

FName UVestigeRestComponent::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Rest"));
	return Key;
}

int32 UVestigeRestComponent::GetSaveVersion() const
{
	return FVestigeRestSaveData::Version;
}

FVestigeSaveRecord UVestigeRestComponent::WriteSaveRecord() const
{
	return FVestigeSaveRecord::Write(GetSaveData());
}

bool UVestigeRestComponent::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FVestigeRestSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}
	RestoreFromSaveData(Data);
	return true;
}

void UVestigeRestComponent::StartNewGame()
{
	if (UAbilitySystemComponent* AbilitySystem = GetAbilitySystem())
	{
		AbilitySystem->RemoveActiveGameplayEffectBySourceEffect(UVestigeGameData::Get().GetExhaustedEffect(), nullptr);
	}
}

void UVestigeRestComponent::ReactToHour(int32 Hour)
{
	if (Hour == LateWarningHour)
	{
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Rest_GettingLate);
	}
	if (Hour == LateNightHour)
	{
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Rest_PastMidnight);
		StartLateNightDrain();
	}
}

void UVestigeRestComponent::EndNight(const FVestigeDayEnd& DayEnd)
{
	StopLateNightDrain();
}

void UVestigeRestComponent::WakeUp(int32 NewDay)
{
	// Also stopped here, for a day started without ending first.
	StopLateNightDrain();
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	if (!Clock || !AbilitySystem)
	{
		return;
	}
	const bool bLateNight = Clock->GetLastDayEnd().TimeOfDay >= LateNightHour;
	if (bLateNight)
	{
		WakeExhausted(*AbilitySystem);
	}
	else
	{
		WakeRested(*AbilitySystem);
	}
}

void UVestigeRestComponent::WakeExhausted(UAbilitySystemComponent& AbilitySystem)
{
	// One level only: another late night keeps the exhaustion the player already has.
	if (!IsExhausted())
	{
		ApplyExhaustion(AbilitySystem);
	}
	RefillStamina(AbilitySystem, ExhaustedRefillFraction);
	UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Rest_WokeExhausted);
}

void UVestigeRestComponent::WakeRested(UAbilitySystemComponent& AbilitySystem)
{
	RefillStamina(AbilitySystem, VestigeRest::FullRefillFraction);
	if (!IsExhausted())
	{
		return;
	}
	AbilitySystem.RemoveActiveGameplayEffectBySourceEffect(UVestigeGameData::Get().GetExhaustedEffect(), nullptr);
	UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Rest_WokeRested);
}

void UVestigeRestComponent::ApplyExhaustion(UAbilitySystemComponent& AbilitySystem) const
{
	if (const TSubclassOf<UGameplayEffect> Exhausted = UVestigeGameData::Get().GetExhaustedEffect())
	{
		AbilitySystem.ApplyGameplayEffectToSelf(Exhausted.GetDefaultObject(), VestigeRest::EffectLevel, AbilitySystem.MakeEffectContext());
	}
}

void UVestigeRestComponent::RefillStamina(UAbilitySystemComponent& AbilitySystem, float Fraction) const
{
	const FGameplayEffectSpecHandle Spec = AbilitySystem.MakeOutgoingSpec(
		UVestigeGameData::Get().GetStaminaRefillEffect(), VestigeRest::EffectLevel, AbilitySystem.MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}
	const float MaxStamina = AbilitySystem.GetNumericAttribute(UVestigeStaminaSet::GetMaxStaminaAttribute());
	Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaRefill, MaxStamina * Fraction);
	AbilitySystem.ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

void UVestigeRestComponent::StartLateNightDrain()
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	if (!AbilitySystem || !Clock || IsDrainingStamina())
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(
		UVestigeGameData::Get().GetLateNightDrainEffect(), VestigeRest::EffectLevel, AbilitySystem->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}

	// This component removes the drain when the day ends, so it must last until then; and it drains once per period of
	// real time, so the amount per period follows the clock's speed.
	const float PeriodSeconds = Spec.Data->GetPeriod();
	if (Spec.Data->Def->DurationPolicy != EGameplayEffectDurationType::Infinite || PeriodSeconds <= UGameplayEffect::NO_PERIOD)
	{
		VESTIGE_LOG(this, LogFarm, Error,
			"The late-night drain effect {Effect} isn't infinite with a period, so the night doesn't drain stamina. Make it infinite and "
			"give it a period.",
			GetNameSafe(Spec.Data->Def.Get()));
		return;
	}
	const float GameHoursPerPeriod = PeriodSeconds / Clock->GetRealSecondsPerGameHour();
	Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaDrain, -LateNightDrainPerGameHour * GameHoursPerPeriod);
	LateNightDrain = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	DrainedAbilitySystem = AbilitySystem;
}

void UVestigeRestComponent::StopLateNightDrain()
{
	if (UAbilitySystemComponent* AbilitySystem = DrainedAbilitySystem.Get())
	{
		AbilitySystem->RemoveActiveGameplayEffect(LateNightDrain);
	}
	LateNightDrain.Invalidate();
	DrainedAbilitySystem.Reset();
}

UAbilitySystemComponent* UVestigeRestComponent::GetAbilitySystem() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}
