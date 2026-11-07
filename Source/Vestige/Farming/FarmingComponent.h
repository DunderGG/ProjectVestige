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
 * Lets its owner, a pawn, use what is in hand: the item in the selected hotbar slot of the inventory on the owner's player
 * state. An item with a UVestigeToolFragment uses the tool it names; an empty slot or any other item uses the hand
 * (HandToolId). What a tool does is its ability (UVestigeToolData::GetAbility, a UVestigeGameplayAbility_UseTool subclass):
 * this component grants the ability of the tool in hand on the ability system the owner reaches (the player state's),
 * with the tool asset as its source object, and passes the use key's press and release to it. It never calls the
 * abilities; they read the target from UFarmTargetingComponent and the crops here, and report what they did, which this
 * component broadcasts as the extension points OnCropHarvested and OnToolUsed. Loads every tool and crop at BeginPlay, so
 * nothing in hand waits on a load. Systems.md, section 8.
 */
UCLASS(ClassGroup = (Farming), meta = (BlueprintSpawnableComponent))
class UFarmingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFarmingComponent();

	/**
	 * The tool in hand: the one the item in hand's tool fragment names, else the hand. Read from the inventory each time.
	 * Nullptr while the owner reaches no inventory (it has no player state) or the hand tool isn't set.
	 */
	const UVestigeToolData* GetCurrentTool() const;

	/** The loaded crop with this ID (FarmCrop:Base.Turnip), or nullptr if the Asset Manager knows none. */
	const UFarmCropData* FindCrop(const FPrimaryAssetId& CropId) const;

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

	/** The use key is released: the tool's ability ends once a use under way has finished. */
	void StopUsingTool();

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

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only: makes a crop the Asset Manager doesn't know (a transient one) plantable. */
	void AddCropForTests(const UFarmCropData& Crop);
#endif

protected:
	/** Loads the tools and crops, then follows the owner's controller and the inventory on its player state. */
	virtual void BeginPlay() override;

	/** Takes the tool's ability back, then stops following the owner and the inventory. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Loads every tool and crop the Asset Manager knows, with their visuals, in one batch. */
	void LoadToolsAndCrops();

	/** The inventory on the owner's player state now, or nullptr without one. */
	UVestigeInventoryComponent* FindInventory() const;

	/**
	 * Listens to the inventory on the owner's player state now, if it isn't the one listened to already, then catches up
	 * with the hand (HandleItemInHandChanged). Called at BeginPlay, when the controller changes and on each press.
	 */
	void FollowInventory();

	/** The item in hand changed: grants the ability of the tool in hand, and broadcasts OnToolChanged if the tool changed. */
	void HandleItemInHandChanged();

	/**
	 * Makes the granted ability match the tool in hand and the ability system the owner reaches now: takes back the old
	 * grant if either changed, and gives the tool's ability. Does nothing while the owner reaches no ability system.
	 */
	void UpdateGrantedTool();

	/**
	 * Takes back the granted ability through the ability system it was granted on, which the owner may no longer reach. A
	 * use under way is told the key is released and finishes its time before the ability goes, so a switch can't cut it
	 * short. A grant still queued (made while the ability list was locked) is cleared.
	 */
	void ReleaseGrantedTool();

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

	/** The tool at the last OnToolChanged, apart from GrantedTool, which is empty while there is no ability system. */
	TWeakObjectPtr<const UVestigeToolData> BroadcastTool;

	/** The inventory whose OnItemInHandChanged this component listens to. */
	TWeakObjectPtr<UVestigeInventoryComponent> Inventory;

	/** The tool used when the item in hand isn't a tool or the slot is empty (VestigeTool:Base.Hand). */
	UPROPERTY(EditDefaultsOnly, Category = "Farming", meta = (AllowPrivateAccess = "true", AllowedTypes = "VestigeTool"))
	FPrimaryAssetId HandToolId;

	/** Every tool, by its primary asset ID, loaded with its visuals at BeginPlay. */
	UPROPERTY()
	TMap<FPrimaryAssetId, TObjectPtr<const UVestigeToolData>> Tools;

	/** Every crop, by its primary asset ID, loaded with its visuals at BeginPlay. */
	UPROPERTY()
	TMap<FPrimaryAssetId, TObjectPtr<const UFarmCropData>> Crops;
};
