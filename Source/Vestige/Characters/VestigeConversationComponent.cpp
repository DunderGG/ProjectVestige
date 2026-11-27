// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigeConversationComponent.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Algo/AllOf.h"
#include "Characters/VestigeDialogueLog.h"
#include "Characters/VestigeDialogueSettings.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeDialogueCondition.h"
#include "Content/VestigeDialogueData.h"
#include "Content/VestigeDialogueOutcome.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "Core/VestigePlayerComponents.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "TimerManager.h"
#include "Time/VestigeClockSubsystem.h"

UVestigeConversationComponent::UVestigeConversationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UVestigeConversationComponent* UVestigeConversationComponent::Find(const AActor* PlayerActor)
{
	return VestigePlayerComponents::Find<UVestigeConversationComponent>(PlayerActor);
}

void UVestigeConversationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UVestigeMilestoneSubsystem* MilestoneSubsystem = UVestigeMilestoneSubsystem::Get(GetWorld()))
	{
		MilestoneSubsystem->OnMilestoneReached.AddUObject(this, &UVestigeConversationComponent::RecordMilestone);
		MilestoneSource = MilestoneSubsystem;
	}
	if (UVestigeClockSubsystem* ClockSubsystem = UVestigeClockSubsystem::Get(GetWorld()))
	{
		ClockSubsystem->OnDayEnded.AddUObject(this, &UVestigeConversationComponent::HandleDayEnded);
		ClockSource = ClockSubsystem;
	}
}

void UVestigeConversationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndConversation();
	EndPassingLine();
	if (UVestigeMilestoneSubsystem* MilestoneSubsystem = MilestoneSource.Get())
	{
		MilestoneSubsystem->OnMilestoneReached.RemoveAll(this);
	}
	if (UVestigeClockSubsystem* ClockSubsystem = ClockSource.Get())
	{
		ClockSubsystem->OnDayEnded.RemoveAll(this);
	}
	MilestoneSource.Reset();
	ClockSource.Reset();
	Super::EndPlay(EndPlayReason);
}

bool UVestigeConversationComponent::StartConversation(AActor& InSpeakerActor, const UVestigeCharacterData& Speaker)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeConversationComponent::StartConversation);

	if (IsInConversation() || IsUsingTool())
	{
		return false;
	}

	const FVestigeDialogueContext Context = MakeContext(Speaker);
	const UVestigeDialogueData* Chosen = ChooseDialogue(Context, EVestigeDialogueDelivery::Talk);
	if (!Chosen || Chosen->GetLines().IsEmpty())
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Warning, "{Speaker} has no conversation to play, so the talk doesn't start.",
			Speaker.GetContentId().ToString());
		UVestigeFeedbackSubsystem::Report(GetWorld(), VestigeGameplayTags::Feedback_Dialogue_NothingToSay);
		return false;
	}
	VESTIGE_LOG(this, LogVestigeDialogue, Verbose, "Talking to {Speaker}: {Conversation}.", Speaker.GetContentId().ToString(),
		Chosen->GetContentId().ToString());

	// The box takes over from a caption.
	EndPassingLine();

	Dialogue = Chosen;
	SpeakerData = &Speaker;
	SpeakerActor = &InSpeakerActor;
	TalkDay = Context.Day;
	TalksTodayBefore = Context.TalksToday;

	// "Met" counts from the start, so a first meeting cut short with Escape still happened.
	if (UVestigeRelationshipComponent* Relationships = UVestigeRelationshipComponent::Find(GetOwner()))
	{
		Relationships->RecordTalk(Speaker.GetPrimaryAssetId(), Context.Day);
	}
	if (UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(GetWorld()))
	{
		Clock->Hold(VestigeDialogue::ClockHold);
		HeldClock = Clock;
	}

	EnterLine(0);
	OnConversationStarted.Broadcast();

	// A listener that can't show the talk (no box) ends it, so the clock is never held for a talk nobody sees.
	if (!IsInConversation())
	{
		return false;
	}
	OnLineChanged.Broadcast();
	return true;
}

void UVestigeConversationComponent::Advance()
{
	if (!IsInConversation() || !ListedChoices.IsEmpty())
	{
		return;
	}
	const FVestigeDialogueLine& Line = Dialogue->GetLines()[CurrentLine];
	if (!Line.Next.IsNone())
	{
		GoToLine(Line.Next);
		return;
	}
	const int32 NextIndex = FindNextLineIndex(Line);
	if (NextIndex == INDEX_NONE)
	{
		EndConversation();
		return;
	}
	ShowLine(NextIndex);
}

void UVestigeConversationComponent::Choose(int32 Index)
{
	if (!IsInConversation() || !ListedChoices.IsValidIndex(Index))
	{
		return;
	}
	const FVestigeDialogueChoice& Choice = Dialogue->GetLines()[CurrentLine].Choices[ListedChoices[Index]];
	const FVestigeDialogueContext Context = MakeContext(*SpeakerData);
	for (const UVestigeDialogueOutcome* Outcome : Choice.Outcomes)
	{
		if (Outcome)
		{
			Outcome->Apply(Context);
		}
	}
	GoToLine(Choice.Next);
}

void UVestigeConversationComponent::EndConversation()
{
	if (!IsInConversation())
	{
		return;
	}
	if (UVestigeClockSubsystem* Clock = HeldClock.Get())
	{
		Clock->Release(VestigeDialogue::ClockHold);
	}
	HeldClock.Reset();

	Dialogue = nullptr;
	SpeakerData = nullptr;
	SpeakerActor.Reset();
	LineText = FText::GetEmpty();
	ChoiceTexts.Reset();
	ListedChoices.Reset();
	CurrentLine = INDEX_NONE;
	if (const UWorld* World = GetWorld())
	{
		LastTalkEndTime = World->GetTimeSeconds();
	}
	OnConversationEnded.Broadcast();
}

bool UVestigeConversationComponent::SayPassingLine(AActor& InSpeakerActor, const UVestigeCharacterData& Speaker)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeConversationComponent::SayPassingLine);

	const UWorld* World = GetWorld();
	const UVestigeClockSubsystem* ClockSubsystem = ClockSource.Get();
	const bool bDayOver = ClockSubsystem && !ClockSubsystem->IsRunning();
	if (!World || IsInConversation() || bSayingPassingLine || GetPassingQuietSecondsLeft() > 0.0 || bDayOver)
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Verbose, "{Speaker} says no line in passing now.", Speaker.GetContentId().ToString());
		return false;
	}

	const FVestigeDialogueContext Context = MakeContext(Speaker);
	const UVestigeDialogueData* Chosen = ChooseDialogue(Context, EVestigeDialogueDelivery::Passing);
	if (!Chosen || Chosen->GetLines().IsEmpty())
	{
		return false;
	}
	const FVestigeDialogueLine& Line = Chosen->GetLines()[0];
	for (const UVestigeDialogueOutcome* Outcome : Line.Outcomes)
	{
		if (Outcome)
		{
			Outcome->Apply(Context);
		}
	}

	PassingSpeakerData = &Speaker;
	PassingSpeakerActor = &InSpeakerActor;
	PassingLineText = Line.PickText(Context.Day, 0);
	PassingLineStartTime = World->GetTimeSeconds();
	PassingLineDuration = GetPassingLineSeconds(PassingLineText, UVestigeDialogueSettings::Get());
	bSayingPassingLine = true;
	World->GetTimerManager().SetTimer(PassingLineTimer, this, &UVestigeConversationComponent::EndPassingLine, PassingLineDuration);
	VESTIGE_LOG(this, LogVestigeDialogue, Log, "{Speaker} says {Line}.", Speaker.GetContentId().ToString(), Line.LineId.ToString());
	OnPassingLineStarted.Broadcast();
	return true;
}

float UVestigeConversationComponent::GetPassingLineSeconds(const FText& Line, const UVestigeDialogueSettings& Settings)
{
	const float ReadingSeconds = Line.ToString().Len() / Settings.GetPassingLineCharactersPerSecond();
	return FMath::Max(Settings.GetPassingLineMinSeconds(), Settings.GetPassingLineBaseSeconds() + ReadingSeconds);
}

double UVestigeConversationComponent::GetPassingQuietSecondsLeft() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0;
	}
	const double QuietEnd = LastTalkEndTime + UVestigeDialogueSettings::Get().GetPassingQuietAfterTalkSeconds();
	return FMath::Max(QuietEnd - World->GetTimeSeconds(), 0.0);
}

void UVestigeConversationComponent::EndPassingLine()
{
	if (!bSayingPassingLine)
	{
		return;
	}
	bSayingPassingLine = false;
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PassingLineTimer);
	}

	// The speaker is still named while the listeners hear it, so a body can tell its own line from another's.
	OnPassingLineEnded.Broadcast();
	PassingSpeakerData = nullptr;
	PassingSpeakerActor.Reset();
	PassingLineText = FText::GetEmpty();
	PassingLineDuration = 0.0;
}

void UVestigeConversationComponent::RecordMilestone(const FGameplayTag& Milestone)
{
	if (const UVestigeClockSubsystem* ClockSubsystem = ClockSource.Get())
	{
		MilestoneDays.Add(Milestone, ClockSubsystem->GetCurrentDay());
	}
}

void UVestigeConversationComponent::HandleDayEnded(const FVestigeDayEnd& /* DayEnd */)
{
	EndPassingLine();
}

bool UVestigeConversationComponent::IsLastLine() const
{
	return IsInConversation() && ListedChoices.IsEmpty() && FindNextLineIndex(Dialogue->GetLines()[CurrentLine]) == INDEX_NONE;
}

bool UVestigeConversationComponent::IsUsingTool() const
{
	const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(VestigeGameplayTags::State_UsingTool);
}

FVestigeDialogueContext UVestigeConversationComponent::MakeContext(const UVestigeCharacterData& Speaker) const
{
	FVestigeDialogueContext Context;
	Context.Speaker = &Speaker;
	Context.Player = Cast<APlayerState>(GetOwner());
	if (const UVestigeClockSubsystem* Clock = UVestigeClockSubsystem::Get(GetWorld()))
	{
		Context.Day = Clock->GetCurrentDay();
		Context.TimeOfDay = Clock->GetTimeOfDay();
		const FVestigeDate Date = Clock->GetDate();
		Context.Season = Date.Season;
		Context.DayOfSeason = Date.DayOfSeason;
	}
	if (const UVestigeRelationshipComponent* Relationships = UVestigeRelationshipComponent::Find(GetOwner()))
	{
		Context.bHasMet = Relationships->HasMet(Speaker.GetPrimaryAssetId());
		Context.TalksToday = Relationships->GetTalksToday(Speaker.GetPrimaryAssetId(), Context.Day);
		Context.Trust = Relationships->GetTrust(Speaker);
		Context.FlagDays = Relationships->GetFlagDays(Speaker.GetPrimaryAssetId());
	}
	if (const UVestigeMilestoneSubsystem* MilestoneSubsystem = UVestigeMilestoneSubsystem::Get(GetWorld()))
	{
		Context.Milestones = MilestoneSubsystem->GetReached();
	}
	for (const TPair<FGameplayTag, int32>& Reached : MilestoneDays)
	{
		if (Reached.Value == Context.Day)
		{
			Context.MilestonesToday.AddTag(Reached.Key);
		}
	}
	return Context;
}

const UVestigeDialogueData* UVestigeConversationComponent::ChooseDialogue(
	const FVestigeDialogueContext& Context, EVestigeDialogueDelivery Delivery) const
{
	const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(GetWorld());
	if (!Catalog)
	{
		return nullptr;
	}

	// Listed in ID order, so with a tie the lower ID is found first and kept.
	TArray<const UVestigeContentData*> All;
	Catalog->FindAll(UVestigeDialogueData::AssetType, All);

	const FPrimaryAssetId SpeakerId = Context.Speaker->GetPrimaryAssetId();
	const UVestigeDialogueData* Best = nullptr;
	for (const UVestigeContentData* Content : All)
	{
		const UVestigeDialogueData* Candidate = Cast<UVestigeDialogueData>(Content);
		if (!Candidate || Candidate->GetDelivery() != Delivery || Candidate->GetSpeaker() != SpeakerId ||
			!Candidate->AreConditionsMet(Context))
		{
			continue;
		}
		if (Best && Candidate->GetPriority() == Best->GetPriority())
		{
			VESTIGE_LOG(this, LogVestigeDialogue, Warning,
				"{First} and {Second} both apply to {Speaker} at priority {Priority}, so the first by ID is played. Give them different "
				"priorities.",
				Best->GetContentId().ToString(), Candidate->GetContentId().ToString(), SpeakerId.ToString(), Best->GetPriority());
		}
		else if (!Best || Candidate->GetPriority() > Best->GetPriority())
		{
			Best = Candidate;
		}
	}
	return Best;
}

void UVestigeConversationComponent::EnterLine(int32 Index)
{
	CurrentLine = Index;
	const FVestigeDialogueLine& Line = Dialogue->GetLines()[Index];
	LineText = Line.PickText(TalkDay, TalksTodayBefore);

	// The talk is recorded by now, so the values name the state after this talk began.
	const FVestigeDialogueContext Context = MakeContext(*SpeakerData);
	ChoiceTexts.Reset();
	ListedChoices.Reset();
	for (int32 ChoiceIndex = 0; ChoiceIndex < Line.Choices.Num(); ++ChoiceIndex)
	{
		const FVestigeDialogueChoice& Choice = Line.Choices[ChoiceIndex];
		const bool bListed = Algo::AllOf(Choice.Conditions,
			[&Context](const UVestigeDialogueCondition* Condition)
			{
				return Condition && Condition->IsMet(Context);
			});
		if (!bListed)
		{
			continue;
		}

		// The box lists at most MaxChoices; more holding at once is a data mistake, and the box's buttons must match the list.
		if (ListedChoices.Num() >= VestigeDialogue::MaxChoices)
		{
			VESTIGE_LOG(this, LogVestigeDialogue, Warning,
				"{Conversation} line {Line} has more than {Max} replies holding at once, so {Reply} and any after it are left out.",
				Dialogue->GetContentId().ToString(), Line.LineId.ToString(), VestigeDialogue::MaxChoices, Choice.ReplyId.ToString());
			break;
		}
		ListedChoices.Add(ChoiceIndex);
		ChoiceTexts.Add(Choice.Text);
	}
	for (const UVestigeDialogueOutcome* Outcome : Line.Outcomes)
	{
		if (Outcome)
		{
			Outcome->Apply(Context);
		}
	}
}

void UVestigeConversationComponent::ShowLine(int32 Index)
{
	EnterLine(Index);
	OnLineChanged.Broadcast();
}

void UVestigeConversationComponent::GoToLine(FName LineId)
{
	if (LineId == VestigeDialogue::EndTalk)
	{
		EndConversation();
		return;
	}
	const TArray<FVestigeDialogueLine>& Lines = Dialogue->GetLines();
	const int32 Index = Lines.IndexOfByPredicate(
		[LineId](const FVestigeDialogueLine& Line)
		{
			return Line.LineId == LineId;
		});
	if (Index == INDEX_NONE)
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Error, "{Conversation} goes next to {Line}, which it has no line for, so the talk ends.",
			Dialogue->GetContentId().ToString(), LineId.ToString());
		EndConversation();
		return;
	}
	ShowLine(Index);
}

int32 UVestigeConversationComponent::FindNextLineIndex(const FVestigeDialogueLine& Line) const
{
	const TArray<FVestigeDialogueLine>& Lines = Dialogue->GetLines();
	if (Line.Next == VestigeDialogue::EndTalk)
	{
		return INDEX_NONE;
	}
	if (Line.Next.IsNone())
	{
		const int32 Following = CurrentLine + 1;
		return Lines.IsValidIndex(Following) ? Following : INDEX_NONE;
	}
	return Lines.IndexOfByPredicate(
		[&Line](const FVestigeDialogueLine& Candidate)
		{
			return Candidate.LineId == Line.Next;
		});
}
