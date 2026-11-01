// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "AbilitySystem/VestigeStaminaRefillEffect.h"

#include "AbilitySystem/VestigeStaminaSet.h"

UVestigeStaminaRefillEffect::UVestigeStaminaRefillEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// The coefficient stays at its default of 1: a full refill.
	FAttributeBasedFloat FullStamina;
	FullStamina.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UVestigeStaminaSet::GetMaxStaminaAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);

	FGameplayModifierInfo Refill;
	Refill.Attribute = UVestigeStaminaSet::GetStaminaAttribute();
	Refill.ModifierOp = EGameplayModOp::Override;
	Refill.ModifierMagnitude = FGameplayEffectModifierMagnitude(FullStamina);
	Modifiers.Add(Refill);
}
