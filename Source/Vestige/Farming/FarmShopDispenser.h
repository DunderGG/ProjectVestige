// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeShopData.h"
#include "GameFramework/Actor.h"
#include "Interaction/VestigeInteractable.h"
#include "FarmShopDispenser.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UVestigeItemData;

/**
 * A seed dispenser on the farm: the place the player buys one item of one shop, one per press. It offers "Buy Turnip Seeds (10)"
 * to anyone with a bag and a purse, buys through VestigeShop::TryBuy and tells the player how it went. It is not the stock,
 * which is the shop's data asset, nor the buying rule, and it has no state. BP_ShopDispenser (Content/Vestige/Blueprints) sets
 * the mesh and the shop, and its children BP_ShopDispenser_<Item> the item; this class names no content. Systems.md, section 26.
 */
UCLASS(Blueprintable)
class AFarmShopDispenser : public AActor, public IVestigeInteractable
{
	GENERATED_BODY()

public:
	AFarmShopDispenser();

	/** The action name of the dispenser's one option. */
	static const FName BuyAction;

	const FPrimaryAssetId& GetShopId() const { return ShopId; }
	const FPrimaryAssetId& GetItemId() const { return ItemId; }

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only, before BeginPlay (a deferred spawn). Dispensers get their shop and item on their Blueprint. */
	void SetOfferForTests(const FPrimaryAssetId& InShopId, const FPrimaryAssetId& InItemId)
	{
		ShopId = InShopId;
		ItemId = InItemId;
	}
#endif

	virtual void GetInteractionOptions_Implementation(AActor* Interactor, TArray<FVestigeInteractionOption>& OutOptions) const override;
	virtual void Interact_Implementation(AActor* Interactor, const FVestigeInteractionOption& Option) override;

protected:
	/** Finds the shop, its offer for the item and the item, once: the data doesn't change while the game runs. */
	virtual void BeginPlay() override;

private:
	/** Whether BeginPlay found the offer and the item. Until then, and if it didn't, the dispenser offers nothing. */
	bool IsResolved() const { return Item != nullptr; }

	/** The cabinet, which the player walks around. No mesh in C++: BP_ShopDispenser sets it. */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> DispenserMesh;

	/**
	 * Where the dispenser can be used from: a box in front of the 0.9 m cabinet and as wide as it, on the interactables' collision
	 * channel, so neighbours on a 1 m pitch never overlap (Systems.md, section 26).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** The shop whose offer this dispenser sells. */
	UPROPERTY(EditDefaultsOnly, Category = "Shop", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeShop"))
	FPrimaryAssetId ShopId;

	/** The item it sells, which the shop must offer. */
	UPROPERTY(EditDefaultsOnly, Category = "Shop", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeItem"))
	FPrimaryAssetId ItemId;

	/** The shop's offer for the item, copied at BeginPlay. */
	FVestigeShopOffer Offer;

	/** The item sold, found at BeginPlay; the content catalogue keeps it loaded. Null until then, or when it wasn't found. */
	UPROPERTY(Transient)
	TObjectPtr<const UVestigeItemData> Item;

	/** What the prompt says. Built once at BeginPlay: the scan asks five times a second. */
	FText BuyText;
};
