// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

// Only in builds with tests, like UVestigeTestEventRecorder.
#if WITH_TESTS

#include "CoreMinimal.h"
#include "UI/VestigeCaptionWidget.h"
#include "VestigeTestCaptionWidget.generated.h"

/**
 * The caption without its Blueprint, for tests: WBP_Caption is made in the widget designer, so the tests of what the caption does
 * run on this one, which holds the two text blocks the designer would bind and can be ticked by hand.
 */
UCLASS(NotBlueprintable, HideDropdown)
class UVestigeTestCaptionWidget : public UVestigeCaptionWidget
{
	GENERATED_BODY()

public:
	/** Gives the caption the speaker name and line text blocks that WBP_Caption binds. Call once, before it follows a conversation. */
	void MakeParts();

	/** Runs the tick the engine runs while the caption is shown. */
	void TickForTests(float DeltaSeconds);
};

#endif
