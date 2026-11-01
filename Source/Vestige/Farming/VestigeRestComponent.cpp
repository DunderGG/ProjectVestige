// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/VestigeRestComponent.h"

#include "AbilitySystem/VestigeStaminaRefillEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Engine/World.h"
#include "Time/VestigeDayCycle.h"

#define LOCTEXT_NAMESPACE "VestigeRestComponent"

namespace VestigeRest
{
	/** Stamina effects have no levels, so they are applied at the ability system's default level. */
	constexpr float EffectLevel = 1.f;
}

UVestigeRestComponent::UVestigeRestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVestigeRestComponent::BeginPlay()
{
	Super::BeginPlay();

	DayCycle = AVestigeDayCycle::Get(GetWorld());
	if (DayCycle)
	{
		DayCycle->OnDayEndReached.AddUObject(this, &UVestigeRestComponent::PassOut);
	}
}

void UVestigeRestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DayCycle)
	{
		DayCycle->OnDayEndReached.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UVestigeRestComponent::Sleep()
{
	StartNextDay(LOCTEXT("Slept", "You slept. Day {Day} begins."));
}

void UVestigeRestComponent::PassOut(int32 EndedDay)
{
	StartNextDay(LOCTEXT("PassedOut", "You stayed up too late and passed out. Day {Day} begins."));
}

void UVestigeRestComponent::StartNextDay(const FText& MessageFormat)
{
	if (!DayCycle)
	{
		return;
	}

	// Raised first, so messages from anything reacting to the new day, like saving, follow it.
	UVestigeFeedbackSubsystem::Notify(GetWorld(), FText::FormatNamed(MessageFormat, TEXT("Day"), DayCycle->GetCurrentDay() + 1));

	// Before the new day starts, so the save made at its start holds the refilled stamina.
	RefillStamina();
	DayCycle->StartNextDay();
}

void UVestigeRestComponent::RefillStamina() const
{
	UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!AbilitySystem)
	{
		return;
	}
	AbilitySystem->ApplyGameplayEffectToSelf(
		GetDefault<UVestigeStaminaRefillEffect>(), VestigeRest::EffectLevel, AbilitySystem->MakeEffectContext());
}

#undef LOCTEXT_NAMESPACE
