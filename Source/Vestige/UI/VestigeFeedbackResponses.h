// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "VestigeFeedbackResponses.generated.h"

struct FVestigeFeedbackReport;
class USoundBase;

/**
 * How the game responds to one feedback report: a message and a sound, each optional. A struct, so a row holds both.
 * BlueprintType only so editor scripts can make and set rows; Blueprints don't read it.
 */
USTRUCT(BlueprintType)
struct FVestigeFeedbackResponse
{
	GENERATED_BODY()

	/**
	 * The message the HUD shows, or empty to show nothing. It may use the values its report gives in braces, such as {Day}
	 * (the tag's comment names them).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback")
	FText Message;

	/**
	 * The sound the HUD plays for the report, or none for silence. Played as a 2D game sound (it pauses with the game) and
	 * never looping, since nothing could stop it (Systems.md, section 14). It belongs to a sound class under SCL_Overall.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Feedback")
	TObjectPtr<USoundBase> Sound;
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
	/** Reports a row whose tag is empty or isn't a tag the tag manager knows, and a row whose sound loops. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_DEV_AUTOMATION_TESTS
	/** For tests only. The asset gets its rows in the editor. */
	void SetResponsesForTests(TMap<FGameplayTag, FVestigeFeedbackResponse> InResponses) { Responses = MoveTemp(InResponses); }
#endif

private:
	/** One row per feedback tag. A report without a row shows nothing and logs a warning. */
	UPROPERTY(
		EditDefaultsOnly, Category = "Feedback", meta = (AllowPrivateAccess = "true", Categories = "Vestige.Feedback", ForceInlineRow))
	TMap<FGameplayTag, FVestigeFeedbackResponse> Responses;
};
