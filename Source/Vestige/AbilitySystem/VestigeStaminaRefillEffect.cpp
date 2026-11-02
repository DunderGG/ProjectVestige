// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeStaminaRefillEffect.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaSet.h"

UVestigeStaminaRefillEffect::UVestigeStaminaRefillEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat Amount;
	Amount.DataTag = VestigeGameplayTags::Data_StaminaRefill;

	FGameplayModifierInfo Refill;
	Refill.Attribute = UVestigeStaminaSet::GetStaminaAttribute();
	Refill.ModifierOp = EGameplayModOp::Override;
	Refill.ModifierMagnitude = FGameplayEffectModifierMagnitude(Amount);
	Modifiers.Add(Refill);
}
