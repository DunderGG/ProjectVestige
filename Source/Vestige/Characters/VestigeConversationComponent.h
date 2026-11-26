// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Content/VestigeDialogueContext.h"
#include "VestigeConversationComponent.generated.h"

class UVestigeCharacterData;
class UVestigeClockSubsystem;
class UVestigeDialogueData;
struct FVestigeDialogueLine;

namespace VestigeDialogue
{
	/** The reason the conversation component holds the clock for while a talk runs. */
	const FName ClockHold(TEXT("Dialogue"));
}

DECLARE_MULTICAST_DELEGATE(FOnVestigeConversationStarted);
DECLARE_MULTICAST_DELEGATE(FOnVestigeLineChanged);
DECLARE_MULTICAST_DELEGATE(FOnVestigeConversationEnded);

/**
 * Runs the player's one talk at a time (Systems.md, section 11): chooses the speaker's conversation from the catalogue by its
 * conditions and priority, keeps the current line and the replies listed under it, applies the outcomes of lines shown and
 * replies chosen, records the talk with the relationship component and holds the clock while it runs. It shows nothing: the
 * dialogue box reads it and reports presses back through Advance, Choose and EndConversation. Lives on the player state. Not
 * saved: a talk never spans a save point.
 */
UCLASS(ClassGroup = (Characters), meta = (BlueprintSpawnableComponent))
class UVestigeConversationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVestigeConversationComponent();

	/** The conversation component of PlayerActor: its own, or its player state's if it is a pawn. Nullptr if it has none. */
	static UVestigeConversationComponent* Find(const AActor* PlayerActor);

	/**
	 * Starts the talk with Speaker, whose body is InSpeakerActor, asked by the speaker's Interact. Returns false, changing nothing,
	 * while a talk runs and while the player's ability system has Vestige.State.UsingTool (a use or the use key held: a use
	 * mustn't finish behind the box). With no conversation whose conditions hold it also reports that the speaker has nothing to
	 * say. Otherwise it records the talk, holds the clock, shows the first line and returns true.
	 */
	bool StartConversation(AActor& InSpeakerActor, const UVestigeCharacterData& Speaker);

	/**
	 * Goes to the line after the current one: its Next, or the next line in the list, and the end of the list ends the talk.
	 * Does nothing while replies are listed or no talk runs.
	 */
	void Advance();

	/**
	 * Picks the reply at Index among the listed ones (GetChoiceTexts' order), applies its outcomes and goes to its Next. Ignores a
	 * bad index.
	 */
	void Choose(int32 Index);

	/** Ends the talk now, as Escape does. Does nothing when none runs. */
	void EndConversation();

	bool IsInConversation() const { return Dialogue != nullptr; }

	/** The character talked to, or nullptr when no talk runs. */
	const UVestigeCharacterData* GetSpeaker() const { return SpeakerData; }

	/** The speaker's body, or nullptr when no talk runs or it is gone. */
	AActor* GetSpeakerActor() const { return SpeakerActor.Get(); }

	/** What the speaker says now. Empty when no talk runs. */
	const FText& GetLineText() const { return LineText; }

	/** The replies under the current line whose conditions hold, in order. Empty for a plain line. */
	const TArray<FText>& GetChoiceTexts() const { return ChoiceTexts; }

	/** Whether advancing from the current line ends the talk: no replies are listed and no line follows. For the box's Close hint. */
	bool IsLastLine() const;

	/** Broadcast once a talk has started, before its first line. */
	FOnVestigeConversationStarted OnConversationStarted;

	/** Broadcast whenever the current line changes, the first one included. Listeners read GetLineText and GetChoiceTexts. */
	FOnVestigeLineChanged OnLineChanged;

	/** Broadcast once when a talk ends, however it ends. No talk runs by then. */
	FOnVestigeConversationEnded OnConversationEnded;

protected:
	/** Ends a running talk, so the clock is never left held. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Whether the player's ability system is using a tool. */
	bool IsUsingTool() const;

	/** The values conditions and outcomes read, for Speaker now. */
	FVestigeDialogueContext MakeContext(const UVestigeCharacterData& Speaker) const;

	/** The speaker's conversation to play for Context, or nullptr: the highest priority whose conditions hold, the lowest ID on a tie. */
	const UVestigeDialogueData* ChooseDialogue(const FVestigeDialogueContext& Context) const;

	/** Makes the line at Index the current one: its text, its listed replies and its outcomes. Broadcasts nothing. */
	void EnterLine(int32 Index);

	/** Enters the line at Index and broadcasts OnLineChanged. */
	void ShowLine(int32 Index);

	/** Goes to the line named LineId, or ends the talk with an error when the asset has none. */
	void GoToLine(FName LineId);

	/** The index of the line after Line: its Next, or the following one, or INDEX_NONE at the end or for a Next that names no line. */
	int32 FindNextLineIndex(const FVestigeDialogueLine& Line) const;

	/** The conversation being played, or nullptr when no talk runs. */
	UPROPERTY()
	TObjectPtr<const UVestigeDialogueData> Dialogue;

	UPROPERTY()
	TObjectPtr<const UVestigeCharacterData> SpeakerData;

	TWeakObjectPtr<AActor> SpeakerActor;

	/** The clock held for the talk, released when it ends. */
	TWeakObjectPtr<UVestigeClockSubsystem> HeldClock;

	FText LineText;
	TArray<FText> ChoiceTexts;

	/** Which choice of the current line each listed reply is. */
	TArray<int32> ListedChoices;

	int32 CurrentLine = INDEX_NONE;

	/** The day the talk started on, and how many talks with the speaker came before it that day, for choosing a pool's text. */
	int32 TalkDay = 0;
	int32 TalksTodayBefore = 0;
};
