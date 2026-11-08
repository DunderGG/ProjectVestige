// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "FarmingComponent.generated.h"

class AController;
class APawn;
class UAbilitySystemComponent;
class UFarmCropData;
class UVestigeInventoryComponent;
class UVestigeItemData;
class UVestigeToolData;
struct FGameplayAbilitySpec;

DECLARE_MULTICAST_DELEGATE(FOnFarmToolChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFarmToolUseHeldChanged, bool /* bHeld */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFarmCropHarvested, const UFarmCropData*, Crop, FIntPoint, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFarmToolUsed, const UVestigeToolData*, Tool, FIntPoint, Tile);

/**
 * Lets its owner, a pawn, use what is in hand and harvest. The item in hand is the item in the selected hotbar slot of the
 * inventory on the owner's player state; an item with a UVestigeToolFragment is the tool it names, and an empty slot or any
 * other item is no tool. What a tool does is its ability (UVestigeToolData::GetAbility, a UVestigeGameplayAbility_UseTool
 * subclass): this component grants the ability of the tool in hand on the ability system the owner reaches (the player
 * state's), with the tool asset as its source object, and passes the use key's press and release to it. It also grants the
 * harvest all the time (HarvestToolId), which Harvest activates once; the farm grid calls it when the player interacts with
 * a ripe tile. It never calls the abilities' rules; they read the target from UFarmTargetingComponent and content from the
 * content catalogue, and report what they did, which this component broadcasts as the extension points OnCropHarvested and
 * OnToolUsed. Finds tools in the content catalogue (UVestigeContentCatalog), which loaded them before play, so nothing
 * waits on a load. Systems.md, sections 2, 7 and 8.
 */
UCLASS(ClassGroup = (Farming), meta = (BlueprintSpawnableComponent))
class UFarmingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFarmingComponent();

	/**
	 * The tool in hand: the one the item in hand's tool fragment names, read from the inventory each time. Nullptr for an empty
	 * slot, an item without a tool fragment or whose tool isn't loaded, and while the owner reaches no inventory.
	 */
	const UVestigeToolData* GetCurrentTool() const;

	/** The tool whose ability harvests (VestigeTool:Base.Hand), or nullptr if HarvestToolId names none. */
	const UVestigeToolData* GetHarvestTool() const;

	/**
	 * Uses the tool in hand once on the target tile, by activating its ability without pressing the use key: the ability
	 * decides whether a use can start now and what it costs. Does nothing while a tool use is under way, or without an
	 * ability system. For tests and console commands; the use key calls StartUsingTool.
	 */
	void UseTool();

	/**
	 * The use key is pressed: activates the tool's ability with its input pressed, so it uses the tool and keeps using it
	 * while the key is held (UVestigeGameplayAbility_UseTool says when).
	 */
	void StartUsingTool();

	/**
	 * The use key is released: the tool's ability ends once a use under way has finished. Does nothing unless a press held
	 * the key, so the pawn can call it when it is unpossessed.
	 */
	void StopUsingTool();

	/**
	 * Harvests the target tile once, whatever is in hand, by activating the harvest ability without pressing it. Does nothing
	 * while a tool use is under way. Called when the player interacts with a ripe tile (AFarmGrid).
	 */
	void Harvest();

	/** Whether the use key is held. The farmer walks slower meanwhile. */
	bool IsToolUseHeld() const { return bToolUseHeld; }

	/** A tool ability changed the tile at Tile with Tool: broadcasts OnToolUsed. Called by the tool abilities only. */
	void ReportToolUsed(const UVestigeToolData& Tool, const FIntPoint& Tile);

	/** A tool ability harvested Crop from the tile at Tile: broadcasts OnCropHarvested. Called by the tool abilities only. */
	void ReportCropHarvested(const UFarmCropData& Crop, const FIntPoint& Tile);

	/**
	 * Broadcast when the tool in hand changes. It carries no value: a listener called during a nested broadcast may be called
	 * for the newer change first, so listeners read GetCurrentTool when called.
	 */
	FOnFarmToolChanged OnToolChanged;

	/** Broadcast when the use key is pressed or released, with whether it is now held. The farmer walks slower meanwhile. */
	FOnFarmToolUseHeldChanged OnToolUseHeldChanged;

	/**
	 * Extension point: broadcast when the owner harvests Crop from the tile at the coordinates Tile (column X, row Y) of the
	 * grid's field, once the harvest is in the bag and the tile is tilled soil again. Before OnToolUsed for the same use.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Farming")
	FOnFarmCropHarvested OnCropHarvested;

	/**
	 * Extension point: broadcast when a use of Tool changed the tile at the coordinates Tile (column X, row Y) of the grid's
	 * field, after the change. A use that changes nothing (no target, nothing to water, nothing to plant, a full bag) isn't
	 * broadcast, as it costs nothing.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Farming")
	FOnFarmToolUsed OnToolUsed;

protected:
	/** Warns if HarvestToolId names no tool, then follows the owner's controller and the inventory on its player state. */
	virtual void BeginPlay() override;

	/** Takes the tool's ability back, then stops following the owner and the inventory. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The inventory on the owner's player state now, or nullptr without one. */
	UVestigeInventoryComponent* FindInventory() const;

	/**
	 * Listens to the inventory on the owner's player state now, if it isn't the one listened to already, then catches up
	 * with the hand (HandleItemInHandChanged). Called at BeginPlay, when the controller changes and on each press.
	 */
	void FollowInventory();

	/** The item in hand changed: updates the grants, and broadcasts OnToolChanged if the tool in hand changed. */
	void HandleItemInHandChanged();

	/**
	 * Makes the grants match the ability system the owner reaches now: the harvest, given again only when that changes, and
	 * the tool in hand's ability, taken back and given again when either changes. Nothing is granted without an ability system.
	 */
	void UpdateGrantedTool();

	/** Gives Tool's ability on AbilitySystem with Tool as the source object, or returns an invalid handle if either is missing. */
	static FGameplayAbilitySpecHandle Grant(UAbilitySystemComponent* AbilitySystem, const UVestigeToolData* Tool);

	/** Reports there is nothing in hand to use, and warns if the item in hand names a tool that isn't loaded. */
	void ReportNothingInHand() const;

	/**
	 * Takes back the grant Spec through AbilitySystem, the ability system it was granted on, which the owner may no longer reach,
	 * and empties both. A use under way is told the key is released and finishes its time before the ability goes, so a switch
	 * can't cut it short. A grant still queued (made while the ability list was locked) is cleared.
	 */
	static void ReleaseGrant(FGameplayAbilitySpecHandle& Spec, TWeakObjectPtr<UAbilitySystemComponent>& AbilitySystem);

	/** The ability system the owner reaches now: the player state's once the owner is possessed. */
	UAbilitySystemComponent* FindAbilitySystem() const;

	/** The granted ability's spec on its ability system, or nullptr. Never kept: specs move when abilities are removed. */
	FGameplayAbilitySpec* FindGrantedSpec() const;

	/** The owner gained or lost its controller, and with it the player state. Bound to the owner's event. */
	UFUNCTION()
	void FollowController(APawn* Pawn, AController* OldController, AController* NewController);

	/** Set while the use key is held. Only for the walk speed: the tool's ability reads its spec's input instead. */
	bool bToolUseHeld = false;

	/** The tool whose ability is granted, the ability system it is granted on, and its spec. Empty when nothing is granted. */
	TWeakObjectPtr<const UVestigeToolData> GrantedTool;
	TWeakObjectPtr<UAbilitySystemComponent> GrantedAbilitySystem;
	FGameplayAbilitySpecHandle GrantedSpec;

	/** The harvest's grant and the ability system it is granted on, apart from the tool's. */
	FGameplayAbilitySpecHandle HarvestSpec;
	TWeakObjectPtr<UAbilitySystemComponent> HarvestAbilitySystem;

	/** The tool at the last OnToolChanged, apart from GrantedTool, which is empty while there is no ability system. */
	TWeakObjectPtr<const UVestigeToolData> BroadcastTool;

	/** The inventory whose OnItemInHandChanged this component listens to. */
	TWeakObjectPtr<UVestigeInventoryComponent> Inventory;

	/** The tool whose ability harvests, granted all the time (VestigeTool:Base.Hand). Was HandToolId (a redirect loads it). */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeTool"))
	FPrimaryAssetId HarvestToolId;
};
