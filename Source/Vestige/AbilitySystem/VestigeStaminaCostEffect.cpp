// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeStaminaCostEffect.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"

UVestigeStaminaCostEffect::UVestigeStaminaCostEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat Cost;
	Cost.DataTag = VestigeGameplayTags::Data_StaminaCost;

	FGameplayModifierInfo Spend;
	Spend.Attribute = UVestigeStaminaSet::GetStaminaAttribute();
	Spend.ModifierOp = EGameplayModOp::Additive;
	Spend.ModifierMagnitude = FGameplayEffectModifierMagnitude(Cost);
	Modifiers.Add(Spend);
}
