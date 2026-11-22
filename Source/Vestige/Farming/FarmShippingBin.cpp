// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmShippingBin.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Content/VestigeItemData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Farming/FarmShippingSubsystem.h"
#include "Inventory/VestigeInventoryComponent.h"

#define LOCTEXT_NAMESPACE "FarmShippingBin"

namespace FarmShippingBinInteraction
{
	/**
	 * Half the size of the box the bin is used from, and where it is: up to the bin's 1 m. Sized for SM_ShippingBin's hull, one
	 * 90 cm box with its pivot on the floor at its centre, so the box is in front of a farmer facing any side, anywhere along it,
	 * and he can't stand inside it; at a corner the four-direction aim passes beside it (Systems.md, section 25). A change to the hull re-checks these numbers.
	 */
	const FVector Extent(55.f, 55.f, 50.f);
	const FVector Location(0.f, 0.f, 50.f);
}

const FName AFarmShippingBin::ShipAction(TEXT("Ship"));

AFarmShippingBin::AFarmShippingBin()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Keeps its default blocking collision, so the farmer walks around the bin rather than through it.
	BinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BinMesh"));
	BinMesh->SetupAttachment(RootComponent);
	BinMesh->SetGenerateOverlapEvents(false);

	// Only found by the interaction component's query, which needs no overlap events.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(FarmShippingBinInteraction::Extent);
	InteractionVolume->SetRelativeLocation(FarmShippingBinInteraction::Location);
	InteractionVolume->SetCollisionProfileName(VestigeInteraction::CollisionProfile);
	InteractionVolume->SetGenerateOverlapEvents(false);
	InteractionVolume->SetCanEverAffectNavigation(false);

	ShipText = LOCTEXT("Ship", "Ship");
}

void AFarmShippingBin::GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const
{
	// Offered whatever is in hand, so pressing F with a tool or seeds tells the player why nothing goes in.
	if (!UVestigeInventoryComponent::FindForPawn(Interactor))
	{
		return;
	}

	FVestigeInteractionOption& ShipOption = OutOptions.AddDefaulted_GetRef();
	ShipOption.Text = ShipText;
	ShipOption.Action = ShipAction;
}

void AFarmShippingBin::Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option)
{
	if (Option.Action != ShipAction)
	{
		return;
	}

	UVestigeInventoryComponent* Inventory = UVestigeInventoryComponent::FindForPawn(Interactor);
	UFarmShippingSubsystem* Shipping = UFarmShippingSubsystem::Get(GetWorld());
	if (!Inventory || !Shipping)
	{
		return;
	}

	const FFarmShipResult Shipped = Shipping->ShipFromSlot(*Inventory, Inventory->GetSelectedHotbarSlot());
	if (!Shipped.Item || Shipped.Count < 1)
	{
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Shipping_NothingToShip);
		return;
	}

	FFormatNamedArguments Values;
	Values.Add(TEXT("Item"), Shipped.Item->GetDisplayName());
	Values.Add(TEXT("Count"), Shipped.Count);
	UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Shipping_Shipped, MoveTemp(Values));
}

#undef LOCTEXT_NAMESPACE
