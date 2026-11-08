// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/VestigeGameplayAbility_UseTool.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Content/VestigeGameData.h"
#include "Content/VestigeToolData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTargetingComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Inventory/VestigeInventoryComponent.h"

namespace VestigeUseTool
{
	/** Single-player: there is no other machine to tell when a use ends. */
	constexpr bool bReplicateEndAbility = false;
}

UVestigeGameplayAbility_UseTool::UVestigeGameplayAbility_UseTool()
{
	// One instance per grant, reused for every use and every press, so a use creates only its wait task.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Single-player: the ability runs where it is activated, with nothing predicted or sent to a server.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	SetAssetTags(FGameplayTagContainer(VestigeGameplayTags::Ability_UseTool));

	// One tool use at a time, across every tool: the old tool's ability may still be finishing a use after a switch.
	ActivationOwnedTags.AddTag(VestigeGameplayTags::State_UsingTool);
	ActivationBlockedTags.AddTag(VestigeGameplayTags::State_UsingTool);
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
	const AActor* Avatar = ActorInfo->AvatarActor.Get();
	UFarmingComponent* FarmingComponent = Avatar ? Avatar->FindComponentByClass<UFarmingComponent>() : nullptr;
	UFarmTargetingComponent* TargetingComponent = Avatar ? Avatar->FindComponentByClass<UFarmTargetingComponent>() : nullptr;
	if (!FarmingComponent || !TargetingComponent || !GetTool())
	{
		EndUse();
		return;
	}
	Farming = FarmingComponent;
	Targeting = TargetingComponent;
	TargetChangedHandle =
		TargetingComponent->OnTargetTileChanged.AddUObject(this, &UVestigeGameplayAbility_UseTool::ContinueAfterTargetChange);
	bFirstTry = true;
	TryNextUse();
}

void UVestigeGameplayAbility_UseTool::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Before the base class, which may remove the spec this ability was granted with.
	if (UFarmTargetingComponent* TargetingComponent = Targeting.Get())
	{
		TargetingComponent->OnTargetTileChanged.Remove(TargetChangedHandle);
	}
	TargetChangedHandle.Reset();
	Targeting.Reset();
	Farming.Reset();
	Step = EUseStep::Idle;
	bFirstTry = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UVestigeGameplayAbility_UseTool::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// A use under way finishes its time first, and FinishUse sees the key is no longer held.
	if (Step == EUseStep::WaitingForTarget || Step == EUseStep::WaitingForLanding)
	{
		EndUse();
	}
}

bool UVestigeGameplayAbility_UseTool::CheckCost(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	return true;
}

UGameplayEffect* UVestigeGameplayAbility_UseTool::GetCostGameplayEffect() const
{
	const TSubclassOf<UGameplayEffect> CostEffect = UVestigeGameData::Get().GetStaminaCostEffect();
	return CostEffect ? CostEffect->GetDefaultObject<UGameplayEffect>() : nullptr;
}

void UVestigeGameplayAbility_UseTool::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UVestigeToolData* Tool = GetTool();
	const TSubclassOf<UGameplayEffect> CostEffect = UVestigeGameData::Get().GetStaminaCostEffect();
	if (!Tool || !CostEffect)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec =
		MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CostEffect, GetAbilityLevel(Handle, ActorInfo));
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(VestigeGameplayTags::Data_StaminaCost, -Tool->GetStaminaCost());
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
}

void UVestigeGameplayAbility_UseTool::TryNextUse()
{
	const bool bFirst = bFirstTry;
	bFirstTry = false;

	if (IsInTheAir())
	{
		if (!IsKeyHeld())
		{
			EndUse();
			return;
		}
		Step = EUseStep::WaitingForLanding;
		UAbilityTask_WaitMovementModeChange* Landing =
			UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
		Landing->OnChange.AddDynamic(this, &UVestigeGameplayAbility_UseTool::ContinueAfterLanding);
		Landing->ReadyForActivation();
		return;
	}

	const UFarmTargetingComponent* TargetingComponent = Targeting.Get();
	const TOptional<FIntPoint> Target = TargetingComponent ? TargetingComponent->GetTargetTile() : TOptional<FIntPoint>();
	if (!Target || !GetField())
	{
		if (bFirst && !ReportFeedback(VestigeGameplayTags::Feedback_Tool_NoTarget))
		{
			return;
		}
		WaitForTargetOrEnd();
		return;
	}

	// Only the press says why the tool can't change the target; a repeat waits quietly, so a held key doesn't report on
	// every tile.
	FGameplayTag FailureReason;
	if (!CanUseOn(*Target, bFirst ? &FailureReason : nullptr))
	{
		if (bFirst)
		{
			ensureMsgf(FailureReason.IsValid(), TEXT("%s::CanUseOn returned false without a reason."), *GetClass()->GetName());
			if (!ReportFeedback(FailureReason))
			{
				return;
			}
		}
		WaitForTargetOrEnd();
		return;
	}

	// Timed before the cost is paid, so the use that empties the stamina still runs at full speed.
	const UVestigeToolData* Tool = GetTool();
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	const float Seconds = Tool && AbilitySystem ? GetUseSeconds(*Tool, *AbilitySystem) : 0.f;

	// UseOn and the reports run other code (the field's change event, Blueprint handlers) that may end this ability.
	Step = EUseStep::Using;
	const bool bChanged = UseOn(*Target);
	if (!IsActive())
	{
		return;
	}
	if (!bChanged)
	{
		// Nothing runs between CanUseOn and UseOn, so they disagree: a bug in the subclass.
		VESTIGE_LOG(this, LogFarm, Warning, "{Ability} changed nothing on tile {Tile}, though its CanUseOn said it would.",
			GetClass()->GetName(), Target->ToString());
		WaitForTargetOrEnd();
		return;
	}

	CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	if (UFarmingComponent* Reporter = Farming.Get(); Reporter && Tool)
	{
		Reporter->ReportToolUsed(*Tool, *Target);
	}
	if (!IsActive())
	{
		return;
	}

	Step = EUseStep::WaitingOutUse;
	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, Seconds);
	Wait->OnFinish.AddDynamic(this, &UVestigeGameplayAbility_UseTool::FinishUse);
	Wait->ReadyForActivation();
}

void UVestigeGameplayAbility_UseTool::WaitForTargetOrEnd()
{
	if (IsKeyHeld())
	{
		Step = EUseStep::WaitingForTarget;
		return;
	}
	EndUse();
}

bool UVestigeGameplayAbility_UseTool::IsKeyHeld() const
{
	// The spec may be gone if the ability is being removed, so it is looked up each time rather than kept.
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	return Spec && Spec->InputPressed;
}

bool UVestigeGameplayAbility_UseTool::IsInTheAir() const
{
	const UCharacterMovementComponent* Movement =
		CurrentActorInfo ? Cast<UCharacterMovementComponent>(CurrentActorInfo->MovementComponent.Get()) : nullptr;
	return Movement && Movement->IsFalling();
}

void UVestigeGameplayAbility_UseTool::FinishUse()
{
	if (Step != EUseStep::WaitingOutUse)
	{
		return;
	}
	if (!IsKeyHeld())
	{
		EndUse();
		return;
	}
	TryNextUse();
}

void UVestigeGameplayAbility_UseTool::ContinueAfterLanding(EMovementMode NewMovementMode)
{
	if (Step == EUseStep::WaitingForLanding)
	{
		TryNextUse();
	}
}

void UVestigeGameplayAbility_UseTool::ContinueAfterTargetChange(const TOptional<FIntPoint>& Tile)
{
	// Only while waiting for one: during a use the next try reads the target anyway.
	if (Step == EUseStep::WaitingForTarget)
	{
		TryNextUse();
	}
}

void UVestigeGameplayAbility_UseTool::EndUse()
{
	constexpr bool bWasCancelled = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, VestigeUseTool::bReplicateEndAbility, bWasCancelled);
}

const UVestigeToolData* UVestigeGameplayAbility_UseTool::GetTool() const
{
	return Cast<UVestigeToolData>(GetCurrentSourceObject());
}

UFarmField* UVestigeGameplayAbility_UseTool::GetField() const
{
	const UFarmTargetingComponent* TargetingComponent = Targeting.Get();
	const AFarmGrid* Grid = TargetingComponent ? TargetingComponent->GetGrid() : nullptr;
	return Grid ? Grid->GetField() : nullptr;
}

UVestigeInventoryComponent* UVestigeGameplayAbility_UseTool::GetInventory() const
{
	const AActor* Owner = CurrentActorInfo ? CurrentActorInfo->OwnerActor.Get() : nullptr;
	return Owner ? Owner->FindComponentByClass<UVestigeInventoryComponent>() : nullptr;
}

bool UVestigeGameplayAbility_UseTool::FailBecause(FGameplayTag* OutFailureReason, const FGameplayTag& Reason)
{
	if (OutFailureReason)
	{
		*OutFailureReason = Reason;
	}
	return false;
}

bool UVestigeGameplayAbility_UseTool::ReportFeedback(const FGameplayTag& Reason)
{
	// The report runs other code (the HUD, Blueprint listeners later) that may end this ability, as UseOn may.
	Step = EUseStep::Using;
	if (Reason.IsValid())
	{
		UVestigeFeedbackSubsystem::Report(GetWorld(), Reason);
	}
	return IsActive();
}
