// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmGameplayAbility_Harvest.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/FarmCropData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "Farming/FarmField.h"
#include "Farming/FarmingComponent.h"
#include "Farming/FarmTileState.h"
#include "Inventory/VestigeInventoryComponent.h"

bool UFarmGameplayAbility_Harvest::CanUseOn(const FIntPoint& Tile, FVestigeFeedbackReport* OutFailure) const
{
	const UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	if (!State)
	{
		return FailBecause(OutFailure, VestigeGameplayTags::Feedback_Tool_NoTarget);
	}
	if (State->CanClearWithered())
	{
		return true;
	}
	if (!State->IsRipe() || !State->GetCrop())
	{
		return FailBecause(OutFailure, VestigeGameplayTags::Feedback_Farm_NothingRipe);
	}
	return HasRoomForHarvest(State->GetHarvestItemId(), *State->GetCrop())
		|| FailBecause(OutFailure, VestigeGameplayTags::Feedback_Inventory_Full);
}

bool UFarmGameplayAbility_Harvest::UseOn(const FIntPoint& Tile)
{
	UFarmField* Field = GetField();
	const FFarmTileState* State = Field ? Field->GetTile(Tile) : nullptr;
	const UFarmCropData* Crop = State ? State->GetCrop() : nullptr;

	// A withered crop is cleared, yields nothing and needs no room in the bag.
	if (State && State->CanClearWithered())
	{
		return Field->ClearWithered(Tile);
	}

	if (!Crop || !State->IsRipe())
	{
		return false;
	}

	// Copied before the harvest, which clears the mutation and lowers the level of this same tile.
	const FPrimaryAssetId HarvestItemId = State->GetHarvestItemId();
	const int32 LevelBefore = State->GetContamination();
	const bool bWasMutated = State->IsMutated();

	// Room is asked before harvesting, so a full bag leaves the crop in the ground instead of destroying it.
	if (!HasRoomForHarvest(HarvestItemId, *Crop) || !Field->Harvest(Tile))
	{
		return false;
	}
	GetInventory()->AddItem(HarvestItemId, Crop->GetYield());
	ReportCleaning(LevelBefore, State->GetContamination());
	if (bWasMutated)
	{
		if (UVestigeMilestoneSubsystem* Milestones = UVestigeMilestoneSubsystem::Get(GetWorld()))
		{
			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);
		}
	}
	if (UFarmingComponent* FarmingComponent = GetFarming())
	{
		FarmingComponent->ReportCropHarvested(*Crop, Tile);
	}
	return true;
}

bool UFarmGameplayAbility_Harvest::HasRoomForHarvest(const FPrimaryAssetId& HarvestItemId, const UFarmCropData& Crop) const
{
	const UVestigeInventoryComponent* Inventory = GetInventory();
	return Inventory && Inventory->CanAddItem(HarvestItemId, Crop.GetYield());
}

void UFarmGameplayAbility_Harvest::ReportCleaning(int32 LevelBefore, int32 LevelAfter) const
{
	if (LevelAfter >= LevelBefore)
	{
		return;
	}
	if (UVestigeMilestoneSubsystem* Milestones = UVestigeMilestoneSubsystem::Get(GetWorld()))
	{
		Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Cleaned);
	}
	const FGameplayTag Reason =
		LevelAfter == 0 ? VestigeGameplayTags::Feedback_Farm_Cleaned_Clean : VestigeGameplayTags::Feedback_Farm_Cleaned;
	UVestigeFeedbackSubsystem::Report(GetWorld(), Reason);
}
