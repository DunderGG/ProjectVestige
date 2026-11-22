// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Plant.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/FarmCropData.h"
#include "Content/FarmSeedFragment.h"
#include "Content/VestigeCalendar.h"
#include "Content/VestigeContentCatalog.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Farming/FarmField.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTileState.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Time/VestigeClockSubsystem.h"

void UFarmGameplayAbility_Plant::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	bWarnedThisPress = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UFarmGameplayAbility_Plant::CanUseOn(const FIntPoint& Tile, FVestigeFeedbackReport* OutFailure) const
{
	// The seed first: without one, the tile doesn't matter.
	const UFarmCropData* Crop = FindCropInHand();
	if (!Crop)
	{
		return FailBecause(OutFailure, VestigeGameplayTags::Feedback_Farm_NothingToPlant);
	}

	// Without a clock (an editor world) every crop fits.
	const UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(GetWorld());
	const FFarmSeasonFit Fit = Clock ? Crop->GetSeasonFit(Clock->GetCalendar(), Clock->GetCurrentDay()) : FFarmSeasonFit();
	const auto FailBecauseOfSeason = [OutFailure, Clock, &Fit](const FGameplayTag& Reason)
	{
		// Only the press, which reports them, builds the values; a repeat stays quiet.
		return FailBecause(OutFailure, Reason,
			OutFailure && Clock ? Fit.GetReportValues(Clock->GetCalendar(), Clock->GetCurrentDay()) : FFormatNamedArguments());
	};

	// The season before the tile: no tile would take these seeds, so tilling first would be wasted work.
	if (Fit.Fit == EFarmSeasonFit::OutOfSeason)
	{
		return FailBecauseOfSeason(VestigeGameplayTags::Feedback_Farm_OutOfSeason);
	}
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	if (!State)
	{
		return FailBecause(OutFailure, VestigeGameplayTags::Feedback_Tool_NoTarget);
	}
	if (State->IsWithered())
	{
		return FailBecause(OutFailure, VestigeGameplayTags::Feedback_Farm_ClearFirst);
	}
	if (!State->CanPlant())
	{
		return FailBecause(OutFailure, VestigeGameplayTags::Feedback_Farm_CantPlant);
	}

	// Too late comes last, so the warning shows only when a seed would really go in.
	if (Fit.Fit == EFarmSeasonFit::TooLate && !IsTooLatePlantingConfirmed(*Crop, Clock->GetCurrentDay()))
	{
		return FailBecauseOfSeason(
			Fit.DaysToDeadline == 0 ? VestigeGameplayTags::Feedback_Farm_TooLate_LastDay : VestigeGameplayTags::Feedback_Farm_TooLate);
	}
	return true;
}

bool UFarmGameplayAbility_Plant::UseOn(const FIntPoint& Tile)
{
	const UFarmCropData* Crop = FindCropInHand();
	UFarmField* Field = GetField();
	if (!Crop || !Field || !Field->Plant(Tile, Crop))
	{
		return false;
	}

	// From the stack in hand, so another stack of the same seed stays as it is. This can empty the hand, which takes this
	// ability back once its use has finished.
	UVestigeInventoryComponent* Inventory = GetInventory();
	Inventory->RemoveItemFromSlot(Inventory->GetSelectedHotbarSlot(), 1);
	return true;
}

void UFarmGameplayAbility_Plant::OnPressRefused(const FIntPoint& Tile, const FVestigeFeedbackReport& Refusal)
{
	if (!Refusal.Tag.MatchesTag(VestigeGameplayTags::Feedback_Farm_TooLate))
	{
		return;
	}
	UFarmingComponent* FarmingComponent = GetFarming();
	const UFarmCropData* Crop = FindCropInHand();
	const UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(GetWorld());
	if (!FarmingComponent || !Crop || !Clock)
	{
		return;
	}

	// Either form of the warning is remembered under Farm.TooLate, so the next press that day plants.
	FarmingComponent->RememberWarning(VestigeGameplayTags::Feedback_Farm_TooLate, Crop->GetPrimaryAssetId(), Clock->GetCurrentDay());
	bWarnedThisPress = true;
}

const UFarmCropData* UFarmGameplayAbility_Plant::FindCropInHand() const
{
	const UVestigeInventoryComponent* Inventory = GetInventory();
	return UFarmSeedFragment::FindCropOf(Inventory ? Inventory->GetItemInHand() : nullptr, UVestigeContentCatalog::Get(GetWorld()));
}

bool UFarmGameplayAbility_Plant::IsTooLatePlantingConfirmed(const UFarmCropData& Crop, int32 Day) const
{
	const UFarmingComponent* FarmingComponent = GetFarming();
	return !bWarnedThisPress && FarmingComponent
		&& FarmingComponent->WasWarned(VestigeGameplayTags::Feedback_Farm_TooLate, Crop.GetPrimaryAssetId(), Day);
}
