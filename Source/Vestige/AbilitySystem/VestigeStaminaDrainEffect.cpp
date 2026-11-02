// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeStaminaDrainEffect.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"

UVestigeStaminaDrainEffect::UVestigeStaminaDrainEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(PeriodSeconds);

	// The first drain comes one period after midnight, not at the stroke of it.
	bExecutePeriodicEffectOnApplication = false;

	FSetByCallerFloat Amount;
	Amount.DataTag = VestigeGameplayTags::Data_StaminaDrain;

	FGameplayModifierInfo Drain;
	Drain.Attribute = UVestigeStaminaSet::GetStaminaAttribute();
	Drain.ModifierOp = EGameplayModOp::Additive;
	Drain.ModifierMagnitude = FGameplayEffectModifierMagnitude(Amount);
	Modifiers.Add(Drain);
}
