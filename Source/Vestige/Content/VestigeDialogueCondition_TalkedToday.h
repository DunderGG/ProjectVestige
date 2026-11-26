// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Content/VestigeDialogueCondition.h"
#include "VestigeDialogueCondition_TalkedToday.generated.h"

/** Holds when a talk with the speaker already started today (or, with bTalked off, none has). */
UCLASS(meta = (DisplayName = "Talked Today"))
class UVestigeDialogueCondition_TalkedToday : public UVestigeDialogueCondition
{
	GENERATED_BODY()

public:
	virtual bool IsMet_Implementation(const FVestigeDialogueContext& Context) const override;

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. */
	void SetTalkedForTests(bool bInTalked) { bTalked = bInTalked; }
#endif

private:
	/** Whether a talk with the speaker must already have started today. */
	UPROPERTY(EditAnywhere, Category = "Dialogue", meta = (AllowPrivateAccess = "true"))
	bool bTalked = true;
};
