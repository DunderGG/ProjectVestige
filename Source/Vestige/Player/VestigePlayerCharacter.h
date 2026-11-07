// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Template/VestigeCharacter.h"
#include "VestigePlayerCharacter.generated.h"

class UAbilitySystemComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UFarmingComponent;
class UFarmTargetingComponent;
class UVestigeInventoryComponent;
class UVestigeInteractionComponent;
class UFarmTargetHighlightComponent;
class UInputAction;
class UInputMappingContext;
class UStaticMesh;
class UStaticMeshComponent;

/** A key the player can press and what it does, for on-screen hints. */
struct FVestigeKeyHint
{
	FText Key;
	FText Action;
};

/**
 * The template's third person character plus farming.
 * BP_PlayerCharacter (Content/Vestige/Blueprints) is the player in the game: the template's character Blueprint, reparented to this
 * class so it keeps its mesh, animation and movement input.
 * Owns the farming key bindings, and describes them for the HUD's key hints. Its abilities live on the player state, which
 * outlives it; it only hands out that ability system, so code that has the pawn finds it the engine's usual way.
 */
UCLASS()
class AVestigePlayerCharacter : public AVestigeCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AVestigePlayerCharacter(const FObjectInitializer& ObjectInitializer);

	/** The ability system on the player state, or nullptr while no player state is set. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** The hotbar keys in slot order, each named after the item in its slot of Inventory. Empty slots are left out. */
	TArray<FVestigeKeyHint> GetHotbarKeyHints(const UVestigeInventoryComponent& Inventory) const;

	/**
	 * The other farming keys and what they do (use tool, interact), in the order they are mapped.
	 * Read from the mapping context, so the hints always name the keys that are really bound.
	 */
	TArray<FVestigeKeyHint> GetActionKeyHints() const;

	/** The key that interacts, for the prompt shown next to an interactable. Empty before the input is created. */
	FText GetInteractKey() const;

	/**
	 * Walks as the template does, and aims the targeting component at the neighbouring tile in the movement direction. The
	 * walk ramps in over MoveRampSeconds from a press, so a tap turns the farmer toward the tile with barely a step.
	 */
	virtual void DoMove(float Right, float Forward) override;

	/**
	 * How much of a movement input held for HeldSeconds reaches the movement component: rising evenly from 0 at the press
	 * to 1 after RampSeconds.
	 */
	static float GetMoveInputScale(float HeldSeconds, float RampSeconds);

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only: the mesh shown in the hand. */
	UStaticMesh* GetHeldToolMeshForTests() const;
#endif

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void UnPossessed() override;

private:
	/** Creates the farming input actions and maps their keys. Runs once, before the farmer is first possessed. */
	void CreateInput();

	/** A new input action mapped to Key in the farming mapping context. A Description makes it show in the key hints. */
	UInputAction* AddInputAction(const FKey& Key, const FText& Description);

	/** The key Action is mapped to in the farming mapping context, or an invalid key. */
	FKey GetMappedKey(const UInputAction* Action) const;

	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;

	/** How long the move input has been held, in seconds, or the largest float when DoMove comes from on-screen controls. */
	float GetMoveHeldSeconds() const;

	/** Selects the hotbar slot Slot of the inventory on the player state. Does nothing without one. Bound to keys 1 to 9. */
	void SelectHotbarSlot(int32 Slot);

	/** Shows the held mesh of the tool in hand, or nothing for the hand or a tool without one. Bound to OnToolChanged. */
	void ShowHeldTool();

	/**
	 * Seconds from a movement press until the farmer walks at full strength. A tap shorter than this moves him a few
	 * centimetres, so a tile next to him can be picked without leaving his own tile, while a held key walks at once.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float MoveRampSeconds = 0.25f;

	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UFarmingComponent> Farming;

	/** The tile the farmer aims at, which movement input steers. */
	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UFarmTargetingComponent> Targeting;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UVestigeInteractionComponent> Interaction;

	/** Frames the tile the targeting component aims at. */
	UPROPERTY(VisibleAnywhere, Category = "Farming")
	TObjectPtr<UFarmTargetHighlightComponent> TargetHighlight;

	/** Attached to the right hand. Move and rotate it in the Blueprint to line up each tool's grip. */
	UPROPERTY(VisibleAnywhere, Category = "Tool")
	TObjectPtr<UStaticMeshComponent> HeldTool;

	// Input is created in code so no input assets are needed yet. These can move to IA_ and IMC_ assets later.
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> FarmingMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UseToolAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	/** One action per number key; the index is the hotbar slot it selects. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> HotbarSlotActions;
};
