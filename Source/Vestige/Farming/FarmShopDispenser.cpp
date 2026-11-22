// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmShopDispenser.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeItemData.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Farming/FarmLog.h"
#include "Inventory/VestigeInventoryComponent.h"
#include "Inventory/VestigeShop.h"
#include "Inventory/VestigeWalletComponent.h"

#define LOCTEXT_NAMESPACE "FarmShopDispenser"

namespace FarmShopDispenserInteraction
{
	/**
	 * Half the size of the box the dispenser is used from, and where it is: up to the cabinet's 1.5 m. Sized for a 0.9 by 0.9 m
	 * cabinet with its pivot on the floor at its centre: 55 cm to the front and back, so a farmer against the front never stands
	 * inside it, and 45 to the sides, the cabinet's own width, so neighbours on a 1 m pitch don't overlap (Systems.md, section 26).
	 * A change to the hull (UCX_SM_ShopDispenser_00) re-checks these numbers.
	 */
	const FVector Extent(55.f, 45.f, 75.f);
	const FVector Location(0.f, 0.f, 75.f);

	/** How many a press buys (GameDesign.md, section 9). */
	constexpr int32 CountPerPress = 1;
}

const FName AFarmShopDispenser::BuyAction(TEXT("Buy"));

AFarmShopDispenser::AFarmShopDispenser()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Keeps its default blocking collision, so the farmer walks around the cabinet rather than through it.
	DispenserMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DispenserMesh"));
	DispenserMesh->SetupAttachment(RootComponent);
	DispenserMesh->SetGenerateOverlapEvents(false);

	// Only found by the interaction component's query, which needs no overlap events.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(FarmShopDispenserInteraction::Extent);
	InteractionVolume->SetRelativeLocation(FarmShopDispenserInteraction::Location);
	InteractionVolume->SetCollisionProfileName(VestigeInteraction::CollisionProfile);
	InteractionVolume->SetGenerateOverlapEvents(false);
	InteractionVolume->SetCanEverAffectNavigation(false);
}

void AFarmShopDispenser::BeginPlay()
{
	Super::BeginPlay();

	const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(GetWorld());
	const UVestigeShopData* Shop = Catalog ? Catalog->Find<UVestigeShopData>(ShopId) : nullptr;
	const FVestigeShopOffer* FoundOffer = Shop ? Shop->FindOffer(ItemId) : nullptr;
	const UVestigeItemData* FoundItem = FoundOffer ? Catalog->Find<UVestigeItemData>(ItemId) : nullptr;
	if (!FoundItem)
	{
		VESTIGE_LOG(this, LogFarm, Warning,
			"The shop {ShopId} has no offer for the item {ItemId}, or either is unknown, so it sells nothing.", ShopId.ToString(),
			ItemId.ToString());
		return;
	}

	Offer = *FoundOffer;
	Item = FoundItem;
	BuyText = FText::Format(LOCTEXT("Buy", "Buy {Item} ({Price})"),
		FFormatNamedArguments{
			{TEXT("Item"), Item->GetDisplayName()},
			{TEXT("Price"), FText::AsNumber(Offer.Price)},
		});
}

void AFarmShopDispenser::GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const
{
	// Offered whatever the purse and the bag hold, so a press tells the player why nothing is bought.
	if (!IsResolved() || !UVestigeInventoryComponent::FindForPawn(Interactor) || !UVestigeWalletComponent::FindForPawn(Interactor))
	{
		return;
	}

	FVestigeInteractionOption& BuyOption = OutOptions.AddDefaulted_GetRef();
	BuyOption.Text = BuyText;
	BuyOption.Action = BuyAction;
}

void AFarmShopDispenser::Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option)
{
	if (Option.Action != BuyAction || !IsResolved())
	{
		return;
	}

	UVestigeInventoryComponent* Inventory = UVestigeInventoryComponent::FindForPawn(Interactor);
	UVestigeWalletComponent* Wallet = UVestigeWalletComponent::FindForPawn(Interactor);
	if (!Inventory || !Wallet)
	{
		return;
	}

	const int32 Count = FarmShopDispenserInteraction::CountPerPress;
	const int32 Cost = Offer.Price * Count;
	switch (VestigeShop::Buy(*Inventory, *Wallet, Offer, Count))
	{
	case EVestigeBuyResult::Bought:
	{
		FFormatNamedArguments Values;
		Values.Add(TEXT("Item"), Item->GetDisplayName());
		Values.Add(TEXT("Count"), Count);
		Values.Add(TEXT("Price"), Cost);
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Shop_Bought, MoveTemp(Values));
		return;
	}
	case EVestigeBuyResult::NoRoom:
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Inventory_Full);
		return;
	case EVestigeBuyResult::NotEnoughMoney:
	{
		FFormatNamedArguments Values;
		Values.Add(TEXT("Price"), Cost);
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Shop_NotEnoughMoney, MoveTemp(Values));
		return;
	}
	case EVestigeBuyResult::InvalidOffer:
		// The warning was logged at BeginPlay; the player has nothing to act on.
		return;
	}
}

#undef LOCTEXT_NAMESPACE
