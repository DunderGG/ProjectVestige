// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Content/VestigeDialogueContext.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "VestigeConversationComponent.generated.h"

class UVestigeCharacterData;
class UVestigeClockSubsystem;
class UVestigeDialogueData;
class UVestigeDialogueSettings;
class UVestigeMilestoneSubsystem;
enum class EVestigeDialogueDelivery : uint8;
struct FVestigeDayEnd;
struct FVestigeDialogueLine;

namespace VestigeDialogue
{
	/** The reason the conversation component holds the clock for while a talk runs. */
	const FName ClockHold(TEXT("Dialogue"));
}

DECLARE_MULTICAST_DELEGATE(FOnVestigeConversationStarted);
DECLARE_MULTICAST_DELEGATE(FOnVestigeLineChanged);
DECLARE_MULTICAST_DELEGATE(FOnVestigeConversationEnded);
DECLARE_MULTICAST_DELEGATE(FOnVestigePassingLineStarted);
DECLARE_MULTICAST_DELEGATE(FOnVestigePassingLineEnded);

/**
 * Runs the player's one talk at a time (Systems.md, section 11): chooses the speaker's conversation from the catalogue by its
 * conditions and priority, keeps the current line and the replies listed under it, applies the outcomes of lines shown and
 * replies chosen, records the talk with the relationship component and holds the clock while it runs. It shows nothing: the
 * dialogue box reads it and reports presses back through Advance, Choose and EndConversation. It also says a character's lines in
 * passing (SayPassingLine): one unasked line shown as a caption while the player keeps playing, never a talk. Lives on the
 * player state. Not saved: a talk never spans a save point, and the milestones of the day are kept in memory only.
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

	/**
	 * Says one of Speaker's passing lines, asked by the speaker's passing line component, whose body is InSpeakerActor. Returns false,
	 * changing nothing, while a talk runs, while a passing line shows, within the quiet after a talk, and once the clock's day end
	 * is reached; and when no passing conversation of the speaker holds. Otherwise it applies the line's outcomes, shows the line
	 * for GetPassingLineSeconds, broadcasts OnPassingLineStarted and returns true. It records no talk, holds no clock and gives no trust.
	 */
	bool SayPassingLine(AActor& InSpeakerActor, const UVestigeCharacterData& Speaker);

	/** How many seconds a passing line of this text shows: at least the minimum, else the base plus its length at the reading rate. */
	static float GetPassingLineSeconds(const FText& Line, const UVestigeDialogueSettings& Settings);

	bool IsSayingPassingLine() const { return bSayingPassingLine; }

	/** The character saying the passing line, or nullptr when none shows. */
	const UVestigeCharacterData* GetPassingSpeaker() const { return PassingSpeakerData; }

	/** The body saying the passing line, or nullptr when none shows or it is gone. Still the speaker while OnPassingLineEnded runs. */
	AActor* GetPassingSpeakerActor() const { return PassingSpeakerActor.Get(); }

	/** The passing line shown. Empty when none shows. */
	const FText& GetPassingLineText() const { return PassingLineText; }

	/** World time, in seconds, when the passing line appeared. */
	double GetPassingLineStartTime() const { return PassingLineStartTime; }

	/** How long the passing line shows, in seconds. */
	double GetPassingLineDuration() const { return PassingLineDuration; }

	/** Seconds until a passing line may be said again after the last talk ended, or 0 when no quiet runs. */
	double GetPassingQuietSecondsLeft() const;

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

	/** Broadcast once a passing line has started showing, its text and speaker readable. */
	FOnVestigePassingLineStarted OnPassingLineStarted;

	/** Broadcast once when a passing line ends, however it ends: its time, a talk, the day's end or EndPlay. */
	FOnVestigePassingLineEnded OnPassingLineEnded;

protected:
	/** Starts keeping the day's milestones and listens for the day's end. */
	virtual void BeginPlay() override;

	/** Ends a running talk and a showing passing line, so the clock is never left held. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Whether the player's ability system is using a tool. */
	bool IsUsingTool() const;

	/** The values conditions and outcomes read, for Speaker now. */
	FVestigeDialogueContext MakeContext(const UVestigeCharacterData& Speaker) const;

	/**
	 * The speaker's conversation of this delivery to play for Context, or nullptr: the highest priority whose conditions hold, the
	 * lowest ID on a tie.
	 */
	const UVestigeDialogueData* ChooseDialogue(const FVestigeDialogueContext& Context, EVestigeDialogueDelivery Delivery) const;

	/** Remembers Milestone as reached today. Bound to the milestone subsystem's OnMilestoneReached. */
	void RecordMilestone(const FGameplayTag& Milestone);

	/** Ends a showing passing line. Bound to the clock's OnDayEnded, so a caption never carries into the morning. */
	void HandleDayEnded(const FVestigeDayEnd& /* DayEnd */);

	/** Ends the showing passing line: clears it, stops its timer and broadcasts OnPassingLineEnded. Does nothing when none shows. */
	void EndPassingLine();

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

	/** The milestones reached, each with the game day it was reached on. Memory only: saves are written at a day's start. */
	TMap<FGameplayTag, int32> MilestoneDays;

	/** The subsystems listened to, so EndPlay unbinds from the same ones. */
	TWeakObjectPtr<UVestigeMilestoneSubsystem> MilestoneSource;
	TWeakObjectPtr<UVestigeClockSubsystem> ClockSource;

	/** The passing line showing, if any. */
	UPROPERTY()
	TObjectPtr<const UVestigeCharacterData> PassingSpeakerData;

	TWeakObjectPtr<AActor> PassingSpeakerActor;
	FText PassingLineText;
	double PassingLineStartTime = 0.0;
	double PassingLineDuration = 0.0;
	bool bSayingPassingLine = false;
	FTimerHandle PassingLineTimer;

	/** World time when the last talk ended, or a time far in the past when none has. */
	double LastTalkEndTime = -TNumericLimits<double>::Max();
};
