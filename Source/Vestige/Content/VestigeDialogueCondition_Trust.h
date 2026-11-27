// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueCondition.h"
#include "VestigeDialogueCondition_Trust.generated.h"

/** Holds when the player's trust with the speaker is at least MinTrust. A step's threshold lives in its step talk's condition. */
UCLASS(meta = (DisplayName = "Trust"))
class UVestigeDialogueCondition_Trust : public UVestigeDialogueCondition
{
	GENERATED_BODY()

public:
	virtual bool IsMet_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_EDITOR
	/** Reports a negative MinTrust. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetMinTrustForTests(int32 InMinTrust) { MinTrust = InMinTrust; }
#endif

private:
	/** The trust needed, at least 0. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	int32 MinTrust = 0;
};
