// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "VestigeGameplayAbility_UseTool.generated.h"

class UAbilitySystemComponent;
class UVestigeToolData;

/**
 * Using the tool in hand, as an ability: what it costs and how long it takes. The avatar's UFarmingComponent picks the
 * tool and the tile and applies the tool's rules (UseToolOnTarget); this ability spends the tool's StaminaCost through
 * UVestigeStaminaCostEffect when the use changed the farm, then stays active for the tool's UseSeconds, longer while the
 * player is out of stamina, so the next use can't start before this one is done.
 * Activated by its asset tag, Vestige.Ability.UseTool, and granted by DA_AbilitySet_Player.
 */
UCLASS()
class UVestigeGameplayAbility_UseTool : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UVestigeGameplayAbility_UseTool();

	/**
	 * How long one use of Tool takes for the player AbilitySystem belongs to, in seconds: the tool's UseSeconds,
	 * OutOfStaminaTimeScale times as long while the player is out of stamina.
	 */
	float GetUseSeconds(const UVestigeToolData& Tool, const UAbilitySystemComponent& AbilitySystem) const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Always passes: an action works at any stamina, and the stamina set keeps a cost from taking it below zero. */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	/** Spends the StaminaCost of the tool in use, through the cost effect's set-by-caller magnitude. */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	/** Ends the use once its time is up, so the next one can start. */
	void FinishUse();

	/** Ends a use that changed nothing, at once and without a cost. */
	void CancelUse();

	/** How many times as long a use takes while the player is out of stamina. */
	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float OutOfStaminaTimeScale = 2.f;

	/** The tool of the use under way, for its cost. Set only while the ability is active. */
	UPROPERTY()
	TObjectPtr<const UVestigeToolData> ToolInUse;

	FTimerHandle UseTimer;
};
