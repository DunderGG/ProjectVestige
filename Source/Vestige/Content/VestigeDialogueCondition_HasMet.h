// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueCondition.h"
#include "VestigeDialogueCondition_HasMet.generated.h"

/** Holds when the player has met the speaker (or, with bMet off, hasn't). */
UCLASS(meta = (DisplayName = "Has Met"))
class UVestigeDialogueCondition_HasMet : public UVestigeDialogueCondition
{
	GENERATED_BODY()

public:
	virtual bool IsMet_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetMetForTests(bool bInMet) { bMet = bInMet; }
#endif

private:
	/** The value the context's bHasMet must have. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	bool bMet = true;
};
