// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmWell.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmWaterSubsystem.h"
#include "Farming/FarmWateringCan.h"
#include "Inventory/VestigeInventoryComponent.h"

#define LOCTEXT_NAMESPACE "FarmWell"

namespace FarmWellInteraction
{
	/**
	 * Half the size of the box the well is used from, and where it is: up to the well's 1.3 m. Sized for SM_Well's 12-sided
	 * ring hull (corners 45 cm out, faces 43.5 cm out), so the box is in front of a farmer standing against the ring from any
	 * side, and he can't stand inside it (Systems.md, section 24). A change to the ring re-checks these numbers.
	 */
	const FVector Extent(55.f, 55.f, 65.f);
	const FVector Location(0.f, 0.f, 65.f);

	/** Whether the inventory holds an item that is a water container. */
	bool HasCan(const UVestigeInventoryComponent& Inventory)
	{
		return FarmWateringCan::FindCanToFill(Inventory) != INDEX_NONE;
	}
}

const FName AFarmWell::FillAction(TEXT("Fill"));

AFarmWell::AFarmWell()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Keeps its default blocking collision, so the farmer walks around the well rather than through it.
	WellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WellMesh"));
	WellMesh->SetupAttachment(RootComponent);
	WellMesh->SetGenerateOverlapEvents(false);

	// Only found by the interaction component's query, which needs no overlap events.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(FarmWellInteraction::Extent);
	InteractionVolume->SetRelativeLocation(FarmWellInteraction::Location);
	InteractionVolume->SetCollisionProfileName(VestigeInteraction::CollisionProfile);
	InteractionVolume->SetGenerateOverlapEvents(false);
}

void AFarmWell::GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const
{
	// With no can in the bag there is nothing to offer.
	const UVestigeInventoryComponent* Inventory = UVestigeInventoryComponent::FindForPawn(Interactor);
	const UFarmWaterSubsystem* Water = UFarmWaterSubsystem::Get(GetWorld());
	if (!Inventory || !Water || !FarmWellInteraction::HasCan(*Inventory))
	{
		return;
	}

	FVestigeInteractionOption& FillOption = OutOptions.AddDefaulted_GetRef();
	FillOption.Text = GetFillText(Water->GetCleanWaterLeft());
	FillOption.Action = FillAction;
}

const FText& AFarmWell::GetFillText(int32 Left) const
{
	if (Left != CachedLeft)
	{
		CachedLeft = Left;
		CachedFillText = FText::FormatNamed(LOCTEXT("Fill", "Fill can ({Left} left)"), TEXT("Left"), Left);
	}
	return CachedFillText;
}

void AFarmWell::Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option)
{
	if (Option.Action != FillAction)
	{
		return;
	}

	UVestigeInventoryComponent* Inventory = UVestigeInventoryComponent::FindForPawn(Interactor);
	UFarmWaterSubsystem* Water = UFarmWaterSubsystem::Get(GetWorld());
	const int32 Slot = Inventory ? FarmWateringCan::FindCanToFill(*Inventory) : INDEX_NONE;
	if (!Water || Slot == INDEX_NONE)
	{
		return;
	}

	// The player's reason comes first, whatever the well holds.
	const int32 Capacity = FarmWateringCan::GetCapacity(Inventory->GetSlots()[Slot].Item);
	const int32 Lacking = Capacity - FarmWateringCan::GetWater(*Inventory, Slot);
	if (Lacking <= 0)
	{
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Water_CanFull);
		return;
	}
	if (Water->GetCleanWaterLeft() <= 0)
	{
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Water_WellDry);
		return;
	}

	const int32 Drawn = Water->DrawCleanWater(Lacking);
	const int32 Added = FarmWateringCan::Fill(*Inventory, Slot, Drawn);
	if (Added != Drawn)
	{
		VESTIGE_LOG(this, LogFarm, Warning, "Drew {Drawn} water but the can took {Added}.", Drawn, Added);
	}
	const FGameplayTag& Result =
		Added < Lacking ? VestigeGameplayTags::Feedback_Water_WellRanDry : VestigeGameplayTags::Feedback_Water_CanFilled;
	UVestigeFeedbackSubsystem::Report(GetWorld(), Result);
}

#undef LOCTEXT_NAMESPACE
