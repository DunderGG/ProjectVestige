// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Interaction/VestigeGameplayAbility_Interact.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "GameFramework/Actor.h"
#include "Interaction/VestigeInteractionComponent.h"

namespace VestigeInteract
{
	/** Single-player: there is no other machine to tell when an interaction ends. */
	constexpr bool bReplicateEndAbility = false;
}

UVestigeGameplayAbility_Interact::UVestigeGameplayAbility_Interact()
{
	// One instance per player, reused for every interaction, so interacting creates no objects.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Single-player: the ability runs where it is activated, with nothing predicted or sent to a server.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	SetAssetTags(FGameplayTagContainer(VestigeGameplayTags::Ability_Interact));
}

void UVestigeGameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// No call to the base class, which only runs a Blueprint's activation graph.
	const AActor* Avatar = ActorInfo->AvatarActor.Get();
	UVestigeInteractionComponent* Interaction = Avatar ? Avatar->FindComponentByClass<UVestigeInteractionComponent>() : nullptr;
	const bool bCommitted = Interaction && CommitAbility(Handle, ActorInfo, ActivationInfo);
	const bool bInteracted = bCommitted && Interaction->InteractWithFocus();

	const bool bWasCancelled = !bInteracted;
	EndAbility(Handle, ActorInfo, ActivationInfo, VestigeInteract::bReplicateEndAbility, bWasCancelled);
}
