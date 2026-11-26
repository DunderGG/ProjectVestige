// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueCondition.h"
#include "VestigeDialogueCondition_TimeOfDay.generated.h"

/**
 * Holds while the clock is in a band of the day: FromHour up to, not including, ToHour, in the clock's hours since midnight at
 * the start of the day, so the band after midnight is 24 to 30.
 */
UCLASS(meta = (DisplayName = "Time Of Day"))
class UVestigeDialogueCondition_TimeOfDay : public UVestigeDialogueCondition
{
	GENERATED_BODY()

public:
	virtual bool IsMet_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetBandForTests(float InFromHour, float InToHour)
	{
		FromHour = InFromHour;
		ToHour = InToHour;
	}
#endif

private:
	/** The first hour of the band, included. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 30, Units = "Hours"))
	float FromHour = 6.f;

	/** The end of the band, not included. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 30, Units = "Hours"))
	float ToHour = 12.f;
};
