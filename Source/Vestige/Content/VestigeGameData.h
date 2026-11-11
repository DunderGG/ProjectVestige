// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "VestigeGameData.generated.h"

class UGameplayEffect;

/**
 * Global data that game code needs and that isn't one piece of content: for now, the gameplay effects code applies
 * (Systems.md, section 9). One asset, DA_GameData, named by UVestigeAssetManager's GameDataPath and loaded once at
 * startup, so the effects it names are loaded before any world and never during play. Designers change what an effect
 * does in its GE_ asset; the numbers that are rules (a tool's cost, the drain per game hour) stay with their owners.
 */
UCLASS(Const)
class UVestigeGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The loaded game data, or the class default object (every effect unset) if it didn't load. */
	static const UVestigeGameData& Get();

	/** Spends stamina once by the amount set as Vestige.Data.StaminaCost (negative). Applied by every tool use that changes the farm. */
	TSubclassOf<UGameplayEffect> GetStaminaCostEffect() const { return StaminaCostEffect; }

	/** Sets stamina once to the amount set as Vestige.Data.StaminaRefill. Applied when the player wakes up. */
	TSubclassOf<UGameplayEffect> GetStaminaRefillEffect() const { return StaminaRefillEffect; }

	/**
	 * Drains stamina once by the amount set as Vestige.Data.StaminaDrain (negative). Applied each game minute from midnight
	 * until the day ends.
	 */
	TSubclassOf<UGameplayEffect> GetLateNightDrainEffect() const { return LateNightDrainEffect; }

	/** Grants Vestige.State.Exhausted while it is active. Applied after a late night, removed after an early one. */
	TSubclassOf<UGameplayEffect> GetExhaustedEffect() const { return ExhaustedEffect; }

#if WITH_EDITOR
	/** Reports an unset effect, and a late-night drain that isn't instant. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. The game data gets its effects in the editor. */
	void SetEffectsForTests(TSubclassOf<UGameplayEffect> InStaminaCost, TSubclassOf<UGameplayEffect> InStaminaRefill,
		TSubclassOf<UGameplayEffect> InLateNightDrain, TSubclassOf<UGameplayEffect> InExhausted);
#endif

private:
	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> StaminaCostEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> StaminaRefillEffect;

	/** Must be instant: the rest component applies it once per game minute of the late night, following the clock. */
	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> LateNightDrainEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> ExhaustedEffect;
};
