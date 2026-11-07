// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/EngineTypes.h"
#include "VestigeGameplayAbility_UseTool.generated.h"

class UAbilitySystemComponent;
class UFarmField;
class UFarmingComponent;
class UVestigeInventoryComponent;
class UVestigeToolData;

/**
 * Base of every tool ability: what all tool uses share. A tool data asset names its subclass (UFarmGameplayAbility_Till...),
 * which UFarmingComponent grants while the tool is in hand, with the tool asset as the spec's source object. The subclass
 * says what the tool does to a tile (CanUseOn, UseOn); this class runs the uses.
 *
 * One press of the use key is one activation. The first use happens at once; each use that changed the farm costs the
 * tool's StaminaCost through the game data's stamina cost effect and then takes the tool's UseSeconds (OutOfStaminaTimeScale times
 * as long while the player is out of stamina). While the key is held (the spec's InputPressed) the uses repeat: the next
 * starts when the last has finished, or when the target tile changes or the farmer lands, and only if it would change the
 * tile, so a repeat that would change nothing waits quietly. No use happens in the air. Releasing the key ends the
 * ability once a use under way has finished.
 * While active it owns Vestige.State.UsingTool, which blocks every tool ability, so one tool use runs at a time.
 * Systems.md, section 9.
 */
UCLASS(Abstract)
class UVestigeGameplayAbility_UseTool : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UVestigeGameplayAbility_UseTool();

	/**
	 * How long one use of Tool takes for the player AbilitySystem belongs to, in seconds: the tool's UseSeconds,
	 * OutOfStaminaTimeScale times as long while the player is out of stamina.
	 */
	float GetUseSeconds(const UVestigeToolData& Tool, const UAbilitySystemComponent& AbilitySystem) const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** The use key was released: ends the ability now if it is waiting, or once the use under way has finished. */
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	/** Always passes: an action works at any stamina, and the stamina set keeps a cost from taking it below zero. */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	/**
	 * The game data's stamina cost effect (UVestigeGameData), which ApplyCost applies. CostGameplayEffectClass stays unset: a
	 * C++ ability can't name a GE_ asset, and its class default object exists before the game data loads.
	 */
	virtual UGameplayEffect* GetCostGameplayEffect() const override;

	/** Spends the StaminaCost of the tool in use, through the cost effect's set-by-caller magnitude. */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Whether a use would change the tile at Tile (column X, row Y) of the field now. Changes nothing and says nothing. */
	virtual bool CanUseOn(const FIntPoint& Tile) const PURE_VIRTUAL(UVestigeGameplayAbility_UseTool::CanUseOn, return false;);

	/**
	 * Uses the tool on the tile at Tile of the field. Returns whether the farm changed; when it didn't, tells the player
	 * why (Notify), and the use costs nothing. Only called while the ability is active.
	 */
	virtual bool UseOn(const FIntPoint& Tile) PURE_VIRTUAL(UVestigeGameplayAbility_UseTool::UseOn, return false;);

	/** The tool this ability was granted for: the spec's source object. Null only if the spec has none. */
	const UVestigeToolData* GetTool() const;

	/** The field the farmer works, or nullptr if the level has no grid or the ability isn't active. */
	UFarmField* GetField() const;

	/** The avatar's farming component while the ability is active, else nullptr. */
	UFarmingComponent* GetFarming() const { return Farming.Get(); }

	/** The inventory on the ability system's owner, the player state, or nullptr if it has none. */
	UVestigeInventoryComponent* GetInventory() const;

	/** Tells the player something through UVestigeFeedbackSubsystem. */
	void Notify(const FText& Message) const;

private:
	/** What the ability is doing, so an event that arrives at the wrong moment is ignored. */
	enum class EUseStep : uint8
	{
		/** Not active. */
		Idle,
		/** Inside UseOn or the reports after it. */
		Using,
		/** A use changed the farm and its time hasn't passed yet. */
		WaitingOutUse,
		/** The key is held and the target can't be worked: waiting for another target. */
		WaitingForTarget,
		/** The key is held in the air: waiting for the farmer to walk again. */
		WaitingForLanding
	};

	/** Tries the next use: the first after the press, or a repeat while the key is held. Ends the ability when there is none. */
	void TryNextUse();

	/** Waits for another target while the key is held, or ends the ability. */
	void WaitForTargetOrEnd();

	/** Whether the use key is held now, read from the spec on every decision. */
	bool IsKeyHeld() const;

	/** Whether the avatar is in the air (jumping or falling). */
	bool IsInTheAir() const;

	/** The use's time has passed. Bound to a WaitDelay task. */
	UFUNCTION()
	void FinishUse();

	/** The farmer walks again after being in the air. Bound to a WaitMovementModeChange task. */
	UFUNCTION()
	void ContinueAfterLanding(EMovementMode NewMovementMode);

	/** Bound to the farming component's OnTargetTileChanged while the ability is active. */
	void ContinueAfterTargetChange(const TOptional<FIntPoint>& Tile);

	/** Ends this activation. */
	void EndUse();

	/** How many times as long a use takes while the player is out of stamina. */
	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float OutOfStaminaTimeScale = 2.f;

	/** The avatar's farming component, found on activation and forgotten when the ability ends. */
	TWeakObjectPtr<UFarmingComponent> Farming;

	FDelegateHandle TargetChangedHandle;

	EUseStep Step = EUseStep::Idle;

	/** Set from activation until the first try, which always uses the tool and says why when it changes nothing. */
	bool bFirstTry = false;
};
