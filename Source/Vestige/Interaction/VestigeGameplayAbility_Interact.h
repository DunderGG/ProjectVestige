// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "VestigeGameplayAbility_Interact.generated.h"

/**
 * Using the focused interactable, as an ability, so tags can block it later (in a menu, in dialogue). The avatar's
 * UVestigeInteractionComponent finds the interactable and uses its first option; this ability has no cost and ends at
 * once. An interaction that takes time later stays active for that time, as the use-tool ability does.
 * Activated by its asset tag, Vestige.Ability.Interact, and granted by DA_AbilitySet_Player.
 */
UCLASS()
class UVestigeGameplayAbility_Interact : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UVestigeGameplayAbility_Interact();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
