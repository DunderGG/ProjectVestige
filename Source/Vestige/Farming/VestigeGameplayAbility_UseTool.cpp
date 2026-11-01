// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/VestigeGameplayAbility_UseTool.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystem/VestigeStaminaCostEffect.h"
#include "AbilitySystemComponent.h"
#include "Content/VestigeToolData.h"
#include "Engine/World.h"
#include "Farming/FarmingComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

namespace VestigeUseTool
{
	/** Single-player: there is no other machine to tell when a use ends. */
	constexpr bool bReplicateEndAbility = false;
}

UVestigeGameplayAbility_UseTool::UVestigeGameplayAbility_UseTool()
{
	// One instance per player, reused for every use, so using a tool creates no objects. While it is active, the
	// ability system won't start it again, which is what keeps one use from starting before the last has finished.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Single-player: the ability runs where it is activated, with nothing predicted or sent to a server.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	CostGameplayEffectClass = UVestigeStaminaCostEffect::StaticClass();
	SetAssetTags(FGameplayTagContainer(VestigeGameplayTags::Ability_UseTool));
}

float UVestigeGameplayAbility_UseTool::GetUseSeconds(const UVestigeToolData& Tool, const UAbilitySystemComponent& AbilitySystem) const
{
	const bool bOutOfStamina = AbilitySystem.HasMatchingGameplayTag(VestigeGameplayTags::State_OutOfStamina);
	return Tool.GetUseSeconds() * (bOutOfStamina ? OutOfStaminaTimeScale : 1.f);
}

void UVestigeGameplayAbility_UseTool::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// No call to the base class, which only runs a Blueprint's activation graph.
	const UAbilitySystemComponent* AbilitySystem = ActorInfo->AbilitySystemComponent.Get();
	const AActor* Avatar = ActorInfo->AvatarActor.Get();
	UFarmingComponent* Farming = Avatar ? Avatar->FindComponentByClass<UFarmingComponent>() : nullptr;
	ToolInUse = Farming ? Farming->GetCurrentTool() : nullptr;

	// Timed before the cost is paid, so the use that empties the stamina still runs at full speed.
	const float Seconds = ToolInUse && AbilitySystem ? GetUseSeconds(*ToolInUse, *AbilitySystem) : 0.f;
	if (!Farming || !Farming->UseToolOnTarget() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		CancelUse();
		return;
	}

	if (Seconds <= 0.f)
	{
		FinishUse();
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(UseTimer, this, &UVestigeGameplayAbility_UseTool::FinishUse, Seconds);
}

void UVestigeGameplayAbility_UseTool::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UseTimer);
	}
	ToolInUse = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UVestigeGameplayAbility_UseTool::CheckCost(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	return true;
}

void UVestigeGameplayAbility_UseTool::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!ToolInUse || !CostGameplayEffectClass)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec =
		MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CostGameplayEffectClass, GetAbilityLevel(Handle, ActorInfo));
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaCost, -ToolInUse->GetStaminaCost());
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
}

void UVestigeGameplayAbility_UseTool::FinishUse()
{
	constexpr bool bWasCancelled = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, VestigeUseTool::bReplicateEndAbility, bWasCancelled);
}

void UVestigeGameplayAbility_UseTool::CancelUse()
{
	constexpr bool bWasCancelled = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, VestigeUseTool::bReplicateEndAbility, bWasCancelled);
}
