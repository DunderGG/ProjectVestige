// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "VestigeFeedbackResponses.generated.h"

struct FVestigeFeedbackReport;

/** How the game responds to one feedback report. A struct, so the report's sound joins the same row when audio arrives. */
USTRUCT()
struct FVestigeFeedbackResponse
{
	GENERATED_BODY()

	/**
	 * The message the HUD shows, or empty to show nothing. It may use the values its report gives in braces, such as {Day}
	 * (the tag's comment names them).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback")
	FText Message;
};

/**
 * The presentation's response to each feedback tag that gameplay reports through UVestigeFeedbackSubsystem
 * (Systems.md, section 15, feedback reports), so gameplay never writes the player's text. DA_FeedbackResponses
 * (Content/Vestige/UI) is the game's, named by BP_HUD, which hard-references it: a few dozen texts needed as soon as the HUD
 * shows.
 */
UCLASS(Const)
class UVestigeFeedbackResponses : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The response to Tag, or nullptr when the asset has no row for it. */
	const FVestigeFeedbackResponse* FindResponse(const FGameplayTag& Tag) const { return Responses.Find(Tag); }

	/** Response's message with Report's values filled in. */
	static FText FormatMessage(const FVestigeFeedbackResponse& Response, const FVestigeFeedbackReport& Report);

	const TMap<FGameplayTag, FVestigeFeedbackResponse>& GetResponses() const { return Responses; }

#if WITH_EDITOR
	/** Reports a row whose tag is empty or isn't a tag the tag manager knows. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. The asset gets its rows in the editor. */
	void SetResponsesForTests(TMap<FGameplayTag, FVestigeFeedbackResponse> InResponses) { Responses = MoveTemp(InResponses); }
#endif

private:
	/** One row per feedback tag. A report without a row shows nothing and logs a warning. */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback", meta = (Categories = "Vestige.Feedback", ForceInlineRow))
	TMap<FGameplayTag, FVestigeFeedbackResponse> Responses;
};
