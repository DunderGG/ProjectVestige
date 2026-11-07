// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Template/VestigeCharacter.h"
#include "VestigePlayerCharacter.generated.h"

class UAbilitySystemComponent;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UFarmingComponent;
class UFarmTargetingComponent;
class UVestigeInputConfig;
class UVestigeInventoryComponent;
class UVestigeInteractionComponent;
class UFarmTargetHighlightComponent;
class UInputAction;
class UInputMappingContext;
class UStaticMesh;
class UStaticMeshComponent;
struct FGameplayTag;

/** A key the player can press and what it does, for on-screen hints. */
struct FVestigeKeyHint
{
	FText Key;
	FText Action;
};

/** A mapping context the pawn adds to its player while it is possessed, and its priority. */
USTRUCT()
struct FVestigeMappingContextAndPriority
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<const UInputMappingContext> MappingContext = nullptr;

	/** Where two contexts map the same key, the higher priority wins. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 Priority = 0;
};

/**
 * The template's third person character plus farming.
 * BP_PlayerCharacter (Content/Vestige/Blueprints) is the player in the game: the template's character Blueprint, reparented to this
 * class so it keeps its mesh, animation and movement input.
 * Owns its input setup: binds the actions of its input config by tag to the components that own them, adds its mapping
 * contexts to its player while possessed, and describes the keys for the HUD's key hints (Systems.md, section 5). Its
 * abilities live on the player state, which outlives it; it only hands out that ability system, so code that has the pawn
 * finds it the engine's usual way.
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
	 * The other farming keys and what they do (use tool, interact): every action of the input config with a description, in
	 * the config's order, with its keyboard or mouse key in the mapping contexts, so the hints name the keys really bound.
	 * Empty without an input config.
	 */
	TArray<FVestigeKeyHint> GetActionKeyHints() const;

	/** The key that interacts, for the prompt shown next to an interactable. Empty without an input config. */
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

	/** For tests only: binds the input to TestInputComponent as possession does. Without a controller no context is added. */
	void SetupPlayerInputComponentForTests(UInputComponent& TestInputComponent);

	const UVestigeInputConfig* GetInputConfigForTests() const { return InputConfig; }
	const TArray<FVestigeMappingContextAndPriority>& GetMappingContextsForTests() const { return MappingContexts; }
#endif

protected:
	virtual void BeginPlay() override;

	/** Binds the template's movement and the input config's actions, then adds MappingContexts to the player. */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Releases a held use key and removes MappingContexts while the old controller is known, so contexts never stack. */
	virtual void UnPossessed() override;

private:
	/** Binds each action of the input config, found by its tag, to the call that owns it. Logs an error without a config. */
	void BindNativeActions(UEnhancedInputComponent& EnhancedInput);

	/** Adds or removes every context in MappingContexts on the local player's Enhanced Input. Without one, does nothing. */
	void AddMappingContexts();
	void RemoveMappingContexts();

	/**
	 * The first keyboard or mouse key Action is mapped to in MappingContexts (their default mappings), or an invalid key.
	 * Gamepad keys aren't hinted until the HUD follows the device in use.
	 */
	FKey FindHintKey(const UInputAction* Action) const;

	/** The input config's action for Tag, or nullptr, without logging: hints simply leave it out. */
	const UInputAction* FindActionForHint(const FGameplayTag& Tag) const;

	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;

	/** How long the move input has been held, in seconds, or the largest float when DoMove comes from on-screen controls. */
	float GetMoveHeldSeconds() const;

	/** Selects the hotbar slot Slot of the inventory on the player state. Does nothing without one. Bound to keys 1 to 9. */
	void SelectHotbarSlot(int32 Slot);

	/**
	 * Selects the next (positive Direction) or previous hotbar slot of the inventory on the player state. Does nothing
	 * without one. Bound to the hotbar's next and previous actions (the mouse wheel and the gamepad shoulders).
	 */
	void StepHotbarSlot(int32 Direction);

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

	/**
	 * Which input action each of the farmer's input tags means (DA_InputConfig_Player). Hard-referenced: it is tiny and
	 * needed the moment the pawn is possessed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<const UVestigeInputConfig> InputConfig = nullptr;

	/**
	 * The contexts added to the player while this pawn is possessed: moving and looking (IMC_Default, IMC_MouseLook) and
	 * farming (IMC_Farming, above them).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (AllowPrivateAccess = "true", TitleProperty = "MappingContext"))
	TArray<FVestigeMappingContextAndPriority> MappingContexts;
};
