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
#include "GameplayEffect.h"
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
		Clock->OnNewDay.AddUObject(this, &UVestigeRestComponent::WakeUp);

		// The game data is loaded at startup and doesn't change during play, so it is checked once.
		if (HasUsableLateNightDrain())
		{
			Clock->OnMinuteStarted.AddUObject(this, &UVestigeRestComponent::DrainLateMinute);
		}
	}
	UVestigeSaveRegistry::RegisterWorldOwner(GetWorld(), *this);
}

void UVestigeRestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UVestigeSaveRegistry::UnregisterWorldOwner(GetWorld(), *this);

	if (Clock)
	{
		Clock->OnHourStarted.RemoveAll(this);
		Clock->OnMinuteStarted.RemoveAll(this);
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
	}
}

void UVestigeRestComponent::DrainLateMinute(int32 Minute)
{
	// Minute ends the game minute just passed, so the first late one ends a minute after LateNightHour. A minute later than
	// the clock is the old day's, heard after another listener of it ended the day and the next one started.
	if (!Clock || Minute <= LateNightHour * VestigeClock::MinutesPerHour || Minute > VestigeClock::ToWholeMinutes(Clock->GetTimeOfDay()))
	{
		return;
	}
	if (UAbilitySystemComponent* AbilitySystem = GetAbilitySystem())
	{
		ApplyLateNightDrain(*AbilitySystem);
	}
}

void UVestigeRestComponent::WakeUp(int32 NewDay)
{
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

bool UVestigeRestComponent::HasUsableLateNightDrain() const
{
	const TSubclassOf<UGameplayEffect> Drain = UVestigeGameData::Get().GetLateNightDrainEffect();
	if (!Drain)
	{
		return false;
	}

	// The clock's minutes are the drain's period, so the effect must be instant: an infinite one would add another
	// active effect every minute.
	if (Drain.GetDefaultObject()->DurationPolicy != EGameplayEffectDurationType::Instant)
	{
		VESTIGE_LOG(this, LogFarm, Error,
			"The late-night drain effect {Effect} isn't instant, so the night doesn't drain stamina. Make its Duration Policy "
			"Instant.",
			GetNameSafe(Drain.Get()));
		return false;
	}
	return true;
}

void UVestigeRestComponent::ApplyLateNightDrain(UAbilitySystemComponent& AbilitySystem) const
{
	const FGameplayEffectSpecHandle Spec = AbilitySystem.MakeOutgoingSpec(
		UVestigeGameData::Get().GetLateNightDrainEffect(), VestigeRest::EffectLevel, AbilitySystem.MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaDrain, -LateNightDrainPerGameHour / VestigeClock::MinutesPerHour);
	AbilitySystem.ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

UAbilitySystemComponent* UVestigeRestComponent::GetAbilitySystem() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}
