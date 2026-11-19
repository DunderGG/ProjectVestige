// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

// Only in builds with tests, like UVestigeTestEventRecorder.
#if WITH_TESTS

#include "CoreMinimal.h"
#include "Farming/VestigeGameplayAbility_UseTool.h"
#include "VestigeTestToolAbility.generated.h"

/**
 * A tool behaviour no base tool has, for tests: tills grass and waters it in the same use. Shows that a new tool is a new
 * ability, with no change to the farming component.
 */
UCLASS(NotBlueprintable, HideDropdown)
class UVestigeTestToolAbility : public UVestigeGameplayAbility_UseTool
{
	GENERATED_BODY()

public:
	/** The presses this ability was told were refused (OnPressRefused), since the last Reset, in the static counters below. */
	static int32 GetRefusedPresses() { return RefusedPresses; }

	/** The reason of the last refused press, or an empty tag. */
	static const FGameplayTag& GetLastRefusal() { return LastRefusal; }

	static void ResetRefusals()
	{
		RefusedPresses = 0;
		LastRefusal = FGameplayTag();
	}

protected:
	virtual bool CanUseOn(const FIntPoint& Tile, FVestigeFeedbackReport* OutFailure) const override;
	virtual bool UseOn(const FIntPoint& Tile) override;
	virtual void OnPressRefused(const FIntPoint& Tile, const FVestigeFeedbackReport& Refusal) override;

private:
	// Static, since an ability is an instance per actor that the test doesn't hold; the tests run one at a time.
	static inline int32 RefusedPresses = 0;
	static inline FGameplayTag LastRefusal;
};

#endif
