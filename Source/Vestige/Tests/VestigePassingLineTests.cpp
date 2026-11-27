// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "AIController.h"
#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeDialogueOutcome_SetFlag.h"
#include "Characters/VestigeDialogueSettings.h"
#include "Characters/VestigeNpcCharacter.h"
#include "Characters/VestigePassingLineComponent.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeDialogueCondition_HasMet.h"
#include "Content/VestigeDialogueCondition_Milestone.h"
#include "Content/VestigeDialogueContext.h"
#include "Content/VestigeDialogueData.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "CQTest.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Interaction/VestigeInteractable.h"
#include "Internationalization/Text.h"
#include "Misc/DataValidation.h"
#include "Player/VestigePlayerState.h"
#include "Tests/VestigeTestDialogueOutcome.h"
#include "TimerManager.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSubsystem.h"

// Lines a character says in passing (Systems/11-Dialogue.md, "Passing lines"): the choosing, the refusals, the end of a line and the
// passing line component's events, on made-up conversations and on BP_Hob's component. Hob's own lines are in VestigeHobPassingTests.cpp.

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigePassingLineTests
{
	const TCHAR* const HobPath = TEXT("/Game/Vestige/Characters/BP_Hob.BP_Hob_C");

	/** The made-up speaker of the tests that build their own conversations. */
	const FName TestSpeakerId(TEXT("Test.Speaker"));

	constexpr float MorningHour = 8.f;

	/** Closer than Hob's earshot, and well beyond it. */
	const FVector NearPlace(500.f, 0.f, 0.f);
	const FVector FarPlace(5000.f, 0.f, 0.f);

	// A function, not a static: UVestigeCharacterData::AssetType is another file's static, which a unity build may initialize after this one.
	FPrimaryAssetId HobId()
	{
		return FPrimaryAssetId(UVestigeCharacterData::AssetType, TEXT("Base.Hob"));
	}

	FVestigeDialogueLine MakeLine(FName Id, const TCHAR* Text, FName Next = NAME_None)
	{
		FVestigeDialogueLine Line;
		Line.LineId = Id;
		Line.Texts.Add(FText::AsCultureInvariant(Text));
		Line.Next = Next;
		return Line;
	}

	/** The string table key of Text, or an empty name when it isn't from one. */
	FName GetTableKey(const FText& Text)
	{
		FName TableId;
		FString Key;
		return FTextInspector::GetTableIdAndKey(Text, TableId, Key) ? FName(*Key) : NAME_None;
	}

	FGameplayTag MakeTag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name));
	}

	FGameplayTag MakeFlag(const TCHAR* Name)
	{
		return MakeTag(*(FString(TEXT("Vestige.Relationship.")) + Name));
	}

	TEST_CLASS(PassingLines, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;
		UVestigeClockSubsystem* Clock = nullptr;
		UVestigeMilestoneSubsystem* Milestones = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		UVestigeConversationComponent* Talk = nullptr;
		UVestigeRelationshipComponent* Relationships = nullptr;
		UVestigeCharacterData* Speaker = nullptr;
		AActor* SpeakerActor = nullptr;

		/** What the outcomes of a test wrote, in order. */
		TArray<FString> Applied;

		/** How many times the conversation component said a passing line started and ended. */
		int32 Started = 0;
		int32 Ended = 0;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Catalog = UVestigeContentCatalog::Get(&Spawner.GetWorld());
			Clock = UVestigeClockSubsystem::Get(&Spawner.GetWorld());
			Milestones = UVestigeMilestoneSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Catalog));
			ASSERT_THAT(IsNotNull(Clock));
			ASSERT_THAT(IsNotNull(Milestones));

			PlayerState = &Spawner.SpawnActor<AVestigePlayerState>();
			PlayerState->DispatchBeginPlay();
			Talk = PlayerState->FindComponentByClass<UVestigeConversationComponent>();
			Relationships = PlayerState->FindComponentByClass<UVestigeRelationshipComponent>();
			ASSERT_THAT(IsNotNull(Talk));
			ASSERT_THAT(IsNotNull(Relationships));
			Talk->OnPassingLineStarted.AddLambda(
				[this]()
				{
					++Started;
				});
			Talk->OnPassingLineEnded.AddLambda(
				[this]()
				{
					++Ended;
				});

			Speaker = &Spawner.SpawnObject<UVestigeCharacterData>();
			Speaker->SetContentIdForTests(TestSpeakerId);
			Speaker->SetDisplayNameForTests(FText::AsCultureInvariant(TEXT("Tester")));
			ASSERT_THAT(IsTrue(Catalog->AddForTests(*Speaker)));
			SpeakerActor = &Spawner.SpawnActor<AActor>();
			USceneComponent* SpeakerRoot = NewObject<USceneComponent>(SpeakerActor);
			SpeakerActor->SetRootComponent(SpeakerRoot);
			SpeakerRoot->RegisterComponent();
			SetClock(1, MorningHour);
		}

		// ---- Helpers ----

		void SetClock(int32 Day, float Hour)
		{
			FVestigeClockSaveData Data;
			Data.Day = Day;
			Data.TimeOfDay = Hour;
			Clock->RestoreFromSaveData(Data);
		}

		/** Adds a conversation of the test speaker to the catalogue. */
		UVestigeDialogueData& AddDialogue(const TCHAR* Id, int32 Priority, EVestigeDialogueDelivery Delivery,
			TArray<FVestigeDialogueLine> Lines, TArray<TObjectPtr<UVestigeDialogueCondition>> Conditions = {})
		{
			UVestigeDialogueData& Dialogue = Spawner.SpawnObject<UVestigeDialogueData>();
			Dialogue.SetContentIdForTests(Id);
			Dialogue.SetSpeakerForTests(Speaker->GetPrimaryAssetId());
			Dialogue.SetPriorityForTests(Priority);
			Dialogue.SetDeliveryForTests(Delivery);
			Dialogue.SetConditionsForTests(MoveTemp(Conditions));
			Dialogue.SetLinesForTests(MoveTemp(Lines));
			Catalog->AddForTests(Dialogue);
			return Dialogue;
		}

		UVestigeDialogueData& AddTalk(const TCHAR* Id, int32 Priority, const TCHAR* Text)
		{
			return AddDialogue(Id, Priority, EVestigeDialogueDelivery::Talk, {MakeLine(Id, Text)});
		}

		UVestigeDialogueData& AddPassing(
			const TCHAR* Id, int32 Priority, const TCHAR* Text, TArray<TObjectPtr<UVestigeDialogueCondition>> Conditions = {})
		{
			return AddDialogue(Id, Priority, EVestigeDialogueDelivery::Passing, {MakeLine(Id, Text)}, MoveTemp(Conditions));
		}

		UVestigeDialogueCondition_HasMet* MakeHasMet(bool bMet)
		{
			UVestigeDialogueCondition_HasMet& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_HasMet>();
			Condition.SetMetForTests(bMet);
			return &Condition;
		}

		UVestigeDialogueCondition_Milestone* MakeMilestone(const FGameplayTag& Milestone, bool bToday)
		{
			UVestigeDialogueCondition_Milestone& Condition = Spawner.SpawnObject<UVestigeDialogueCondition_Milestone>();
			Condition.SetForTests(Milestone, true, bToday);
			return &Condition;
		}

		UVestigeTestDialogueOutcome* MakeOutcome(const TCHAR* Name)
		{
			UVestigeTestDialogueOutcome& Outcome = Spawner.SpawnObject<UVestigeTestDialogueOutcome>();
			Outcome.Setup(Name, Applied);
			return &Outcome;
		}

		bool Say() { return Talk->SayPassingLine(*SpeakerActor, *Speaker); }

		bool StartTalk() { return Talk->StartConversation(*SpeakerActor, *Speaker); }

		bool LineIs(const TCHAR* Text) const { return Talk->GetPassingLineText().ToString() == Text; }

		/** Moves the world's time on, as playing would, without ticking anything. */
		void PassSeconds(double Seconds) { Spawner.GetWorld().TimeSeconds += Seconds; }

		/**
		 * Runs the world's timers for Seconds. A test world's timer manager ticks once a frame and starts a timer set since its last
		 * tick only at its next one, so this ticks twice, each in a frame of its own: the first starts the timers, the second runs them.
		 */
		void TickTimers(double Seconds)
		{
			FTimerManager& Timers = Spawner.GetWorld().GetTimerManager();
			++GFrameCounter;
			Timers.Tick(0.f);
			++GFrameCounter;
			Timers.Tick(static_cast<float>(Seconds));
		}

		// ---- The data and the choosing ----

		TEST_METHOD(APassingLineIsChosenOnlyAmongPassingAssets)
		{
			// The passing line ranks higher, so only its delivery can keep it out of the talk.
			AddTalk(TEXT("Test.Talk"), 10, TEXT("talk"));
			AddPassing(TEXT("Test.Passing"), 99, TEXT("passing"));

			ASSERT_THAT(IsTrue(StartTalk()));
			ASSERT_THAT(IsTrue(Talk->GetLineText().ToString() == TEXT("talk")));
			ASSERT_THAT(IsTrue(Started == 0));
			Talk->EndConversation();
			PassSeconds(UVestigeDialogueSettings::Get().GetPassingQuietAfterTalkSeconds() + 1.0);

			// And the talk, ranking lower, is never said in passing either way round.
			ASSERT_THAT(IsTrue(Say()));
			ASSERT_THAT(IsTrue(LineIs(TEXT("passing"))));
		}

		TEST_METHOD(WithOnlyATalkNothingIsSaidInPassing)
		{
			AddTalk(TEXT("Test.Talk"), 10, TEXT("talk"));

			ASSERT_THAT(IsFalse(Say()));

			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(Started == 0));
		}

		TEST_METHOD(WithOnlyAPassingLineATalkIsRefusedAndReported)
		{
			AddPassing(TEXT("Test.Passing"), 10, TEXT("passing"));
			TestRunner->AddExpectedMessagePlain(TEXT("has no conversation to play"), ELogVerbosity::Warning);

			ASSERT_THAT(IsFalse(StartTalk()));

			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
		}

		TEST_METHOD(TheHighestPriorityDuePassingLineIsSaid)
		{
			AddPassing(TEXT("Test.Low"), 10, TEXT("low"));
			AddPassing(TEXT("Test.Middle"), 30, TEXT("middle"));
			AddPassing(TEXT("Test.NeedsMet"), 50, TEXT("met"), {MakeHasMet(true)});

			// The top line is not due to a stranger, so the next one is.
			ASSERT_THAT(IsTrue(Say()));
			ASSERT_THAT(IsTrue(LineIs(TEXT("middle"))));
			ASSERT_THAT(IsTrue(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(Talk->GetPassingSpeaker() == Speaker));
			ASSERT_THAT(IsTrue(Talk->GetPassingSpeakerActor() == SpeakerActor));
			ASSERT_THAT(IsTrue(Started == 1 && Ended == 0));
			TickTimers(Talk->GetPassingLineDuration() + 0.1);
			Relationships->RecordTalk(Speaker->GetPrimaryAssetId(), 1);

			ASSERT_THAT(IsTrue(Say()));
			ASSERT_THAT(IsTrue(LineIs(TEXT("met"))));
		}

		TEST_METHOD(ATieAtOnePriorityPicksTheLowerIdAndWarns)
		{
			AddPassing(TEXT("Test.TieB"), 20, TEXT("b"));
			AddPassing(TEXT("Test.TieA"), 20, TEXT("a"));
			TestRunner->AddExpectedMessagePlain(TEXT("both apply"), ELogVerbosity::Warning);

			ASSERT_THAT(IsTrue(Say()));

			ASSERT_THAT(IsTrue(LineIs(TEXT("a"))));
		}

		TEST_METHOD(TheMilestoneConditionCanAskForToday)
		{
			const FGameplayTag Harvested = VestigeGameplayTags::Milestone_Farm_Harvested;
			const UVestigeDialogueCondition_Milestone* Ever = MakeMilestone(Harvested, false);
			const UVestigeDialogueCondition_Milestone* Today = MakeMilestone(Harvested, true);
			FVestigeDialogueContext Context;
			Context.Milestones.AddTag(Harvested);

			// Reached on an earlier day: counts for a talk, not for a line due only on its day.
			ASSERT_THAT(IsTrue(Ever->IsMet(Context)));
			ASSERT_THAT(IsFalse(Today->IsMet(Context)));

			Context.MilestonesToday.AddTag(Harvested);
			ASSERT_THAT(IsTrue(Ever->IsMet(Context)));
			ASSERT_THAT(IsTrue(Today->IsMet(Context)));
		}

		TEST_METHOD(TheContextCarriesOnlyTodaysMilestones)
		{
			AddPassing(TEXT("Test.Event"), 10, TEXT("event"), {MakeMilestone(VestigeGameplayTags::Milestone_Farm_Harvested, true)});
			ASSERT_THAT(IsFalse(Say()));

			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			ASSERT_THAT(IsTrue(Say()));
			TickTimers(Talk->GetPassingLineDuration() + 0.1);

			// The next morning it was reached yesterday.
			SetClock(2, MorningHour);
			ASSERT_THAT(IsFalse(Say()));
		}

		TEST_METHOD(AMilestoneReachedInTheNightSaleCountsForTheEndedDay)
		{
			// The sale's line ranks higher, so only the day can keep it out; the other one shows the morning's own milestone counts.
			AddPassing(TEXT("Test.Sold"), 30, TEXT("sold"), {MakeMilestone(VestigeGameplayTags::Milestone_Farm_Sold, true)});
			AddPassing(TEXT("Test.Morning"), 20, TEXT("morning"), {MakeMilestone(VestigeGameplayTags::Milestone_Farm_Harvested, true)});
			SetClock(1, 25.5f);

			// Sold after midnight, while the clock still shows the old day: it is that day's, not the next morning's.
			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Sold);
			Clock->StartNextDay();
			ASSERT_THAT(IsTrue(Clock->GetCurrentDay() == 2));
			ASSERT_THAT(IsFalse(Say()));

			Milestones->Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			ASSERT_THAT(IsTrue(Say()));
			ASSERT_THAT(IsTrue(LineIs(TEXT("morning"))));
		}

		// ---- Refusals ----

		TEST_METHOD(APassingLineIsRefusedDuringATalk)
		{
			AddTalk(TEXT("Test.Talk"), 10, TEXT("talk"));
			AddPassing(TEXT("Test.Passing"), 10, TEXT("passing"));
			ASSERT_THAT(IsTrue(StartTalk()));

			ASSERT_THAT(IsFalse(Say()));

			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(Started == 0));
			ASSERT_THAT(IsTrue(Talk->IsInConversation()));
		}

		TEST_METHOD(APassingLineIsRefusedWithinTheQuietAfterATalk)
		{
			AddTalk(TEXT("Test.Talk"), 10, TEXT("talk"));
			AddPassing(TEXT("Test.Passing"), 10, TEXT("passing"));
			const double Quiet = UVestigeDialogueSettings::Get().GetPassingQuietAfterTalkSeconds();
			// No quiet before any talk has ended.
			ASSERT_THAT(IsTrue(Talk->GetPassingQuietSecondsLeft() == 0.0));
			ASSERT_THAT(IsTrue(StartTalk()));
			Talk->EndConversation();

			ASSERT_THAT(IsFalse(Say()));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Talk->GetPassingQuietSecondsLeft(), Quiet, 0.01)));

			PassSeconds(Quiet - 1.0);
			ASSERT_THAT(IsFalse(Say()));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Talk->GetPassingQuietSecondsLeft(), 1.0, 0.01)));

			PassSeconds(1.5);
			ASSERT_THAT(IsTrue(Talk->GetPassingQuietSecondsLeft() == 0.0));
			ASSERT_THAT(IsTrue(Say()));
		}

		TEST_METHOD(APassingLineIsRefusedWhileAnotherShows)
		{
			AddPassing(TEXT("Test.Passing"), 10, TEXT("passing"));
			ASSERT_THAT(IsTrue(Say()));

			ASSERT_THAT(IsFalse(Say()));

			ASSERT_THAT(IsTrue(Started == 1));
		}

		TEST_METHOD(APassingLineIsRefusedAtTheDayEnd)
		{
			AddPassing(TEXT("Test.Passing"), 10, TEXT("passing"));
			ASSERT_THAT(IsTrue(Clock->EndDay()));

			ASSERT_THAT(IsFalse(Say()));
			ASSERT_THAT(IsTrue(Started == 0));

			Clock->StartNextDay();
			ASSERT_THAT(IsTrue(Say()));
		}

		// ---- Saying one, and its end ----

		TEST_METHOD(ASaidLineAppliesItsOutcomesWithoutATalkOrAClockHold)
		{
			UVestigeDialogueOutcome_SetFlag& Spacing = Spawner.SpawnObject<UVestigeDialogueOutcome_SetFlag>();
			Spacing.SetFlagForTests(MakeFlag(TEXT("PassingSaid")));
			FVestigeDialogueLine Line = MakeLine(TEXT("Test.Passing"), TEXT("passing"));
			Line.Outcomes = {MakeOutcome(TEXT("said")), &Spacing};
			AddDialogue(TEXT("Test.Passing"), 10, EVestigeDialogueDelivery::Passing, {Line});
			SetClock(3, MorningHour);

			ASSERT_THAT(IsTrue(Say()));

			const FPrimaryAssetId Id = Speaker->GetPrimaryAssetId();
			ASSERT_THAT(IsTrue(Applied == TArray<FString>{TEXT("said")}));
			const int32* FlagDay = Relationships->GetFlagDays(Id).Find(MakeFlag(TEXT("PassingSaid")));
			ASSERT_THAT(IsTrue(FlagDay && *FlagDay == 3));
			// Not a talk: nothing started, nothing held, nobody met, no talk counted, no trust given.
			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Clock->IsHeld()));
			ASSERT_THAT(IsFalse(Relationships->HasMet(Id)));
			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(Id, 3) == 0));
			ASSERT_THAT(IsTrue(Relationships->GetEarnedTrust(Id) == 0));
			ASSERT_THAT(IsTrue(Talk->GetPassingLineDuration() > 0.0));
		}

		TEST_METHOD(StartingATalkEndsThePassingLine)
		{
			AddTalk(TEXT("Test.Talk"), 10, TEXT("talk"));
			AddPassing(TEXT("Test.Passing"), 10, TEXT("passing"));
			ASSERT_THAT(IsTrue(Say()));
			ASSERT_THAT(IsTrue(Started == 1 && Ended == 0));

			ASSERT_THAT(IsTrue(StartTalk()));

			ASSERT_THAT(IsTrue(Ended == 1));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(Talk->GetPassingLineText().IsEmpty()));
			ASSERT_THAT(IsTrue(Talk->GetPassingSpeaker() == nullptr));
			ASSERT_THAT(IsTrue(Talk->IsInConversation()));
		}

		TEST_METHOD(TheDayEndEndsThePassingLine)
		{
			AddPassing(TEXT("Test.Passing"), 10, TEXT("passing"));
			ASSERT_THAT(IsTrue(Say()));

			ASSERT_THAT(IsTrue(Clock->EndDay()));

			ASSERT_THAT(IsTrue(Ended == 1));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
		}

		TEST_METHOD(ThePassingLineEndsAfterItsTime)
		{
			AddPassing(TEXT("Test.Passing"), 10, TEXT("passing"));
			ASSERT_THAT(IsTrue(Say()));
			const double Duration = Talk->GetPassingLineDuration();
			const double Expected =
				UVestigeConversationComponent::GetPassingLineSeconds(Talk->GetPassingLineText(), UVestigeDialogueSettings::Get());
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Duration, Expected)));
			ASSERT_THAT(IsTrue(Talk->GetPassingLineStartTime() == Spawner.GetWorld().GetTimeSeconds()));

			TickTimers(Duration + 0.1);

			ASSERT_THAT(IsTrue(Ended == 1));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(Talk->GetPassingLineText().IsEmpty()));
			ASSERT_THAT(IsTrue(Talk->GetPassingSpeakerActor() == nullptr));
		}

		TEST_METHOD(TheLineTimeGrowsWithItsLengthFromAMinimum)
		{
			const UVestigeDialogueSettings& Settings = UVestigeDialogueSettings::Get();
			const auto SecondsFor = [&Settings](int32 Characters)
			{
				return UVestigeConversationComponent::GetPassingLineSeconds(
					FText::AsCultureInvariant(FString::ChrN(Characters, TEXT('a'))), Settings);
			};

			// A short line shows for the minimum, a long one for the base plus its reading time.
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(SecondsFor(5), Settings.GetPassingLineMinSeconds())));
			const float LongLine = Settings.GetPassingLineBaseSeconds() + 80.f / Settings.GetPassingLineCharactersPerSecond();
			ASSERT_THAT(IsTrue(LongLine > Settings.GetPassingLineMinSeconds()));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(SecondsFor(80), LongLine)));
			ASSERT_THAT(IsTrue(SecondsFor(80) > SecondsFor(44) && SecondsFor(44) > SecondsFor(5)));
		}

		TEST_METHOD(TheDialogueSettingsHoldTheDesignedTimes)
		{
			// The game designer's numbers (GameDesign.md, section 8): 10 s quiet, 1.5 s and 1 s per 15 characters, at least 3 s.
			const UVestigeDialogueSettings& Settings = UVestigeDialogueSettings::Get();

			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Settings.GetPassingQuietAfterTalkSeconds(), 10.f)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Settings.GetPassingLineBaseSeconds(), 1.5f)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Settings.GetPassingLineCharactersPerSecond(), 15.f)));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Settings.GetPassingLineMinSeconds(), 3.f)));
		}

		// ---- Validation ----

		/** A conversation of Hob, a speaker that exists, that is not in the catalogue, for the validation tests. */
		UVestigeDialogueData& MakeToValidate(EVestigeDialogueDelivery Delivery, TArray<FVestigeDialogueLine> Lines)
		{
			UVestigeDialogueData& Dialogue = Spawner.SpawnObject<UVestigeDialogueData>();
			Dialogue.SetContentIdForTests(TEXT("Test.Validate"));
			Dialogue.SetSpeakerForTests(HobId());
			Dialogue.SetDeliveryForTests(Delivery);
			Dialogue.SetLinesForTests(MoveTemp(Lines));
			return Dialogue;
		}

		bool IsInvalid(const UVestigeDialogueData& Dialogue) const
		{
			FDataValidationContext Context;
			return Dialogue.IsDataValid(Context) == EDataValidationResult::Invalid && Context.GetNumErrors() >= 1;
		}

		TEST_METHOD(APassingAssetHasOneLineOneTextAndNoChoices)
		{
			ASSERT_THAT(IsFalse(IsInvalid(MakeToValidate(EVestigeDialogueDelivery::Passing, {MakeLine(TEXT("One"), TEXT("one"))}))));

			// Two lines.
			ASSERT_THAT(IsTrue(IsInvalid(MakeToValidate(
				EVestigeDialogueDelivery::Passing, {MakeLine(TEXT("One"), TEXT("one")), MakeLine(TEXT("Two"), TEXT("two"))}))));

			// Two texts: a pool, which a caption said once has no use for.
			FVestigeDialogueLine Pool = MakeLine(TEXT("One"), TEXT("one"));
			Pool.Texts.Add(FText::AsCultureInvariant(TEXT("uno")));
			ASSERT_THAT(IsTrue(IsInvalid(MakeToValidate(EVestigeDialogueDelivery::Passing, {Pool}))));

			// A reply: nobody can answer a caption.
			FVestigeDialogueLine Question = MakeLine(TEXT("One"), TEXT("one"));
			FVestigeDialogueChoice Reply;
			Reply.ReplyId = TEXT("Yes");
			Reply.Text = FText::AsCultureInvariant(TEXT("yes"));
			Reply.Next = VestigeDialogue::EndTalk;
			Question.Choices.Add(Reply);
			ASSERT_THAT(IsTrue(IsInvalid(MakeToValidate(EVestigeDialogueDelivery::Passing, {Question}))));

			// A Next, even the reserved end.
			ASSERT_THAT(IsTrue(IsInvalid(
				MakeToValidate(EVestigeDialogueDelivery::Passing, {MakeLine(TEXT("One"), TEXT("one"), VestigeDialogue::EndTalk)}))));
		}

		TEST_METHOD(ATalkMayStillHoldSeveralLinesTextsAndReplies)
		{
			FVestigeDialogueLine Pool = MakeLine(TEXT("One"), TEXT("one"));
			Pool.Texts.Add(FText::AsCultureInvariant(TEXT("uno")));

			ASSERT_THAT(IsFalse(IsInvalid(MakeToValidate(EVestigeDialogueDelivery::Talk, {Pool, MakeLine(TEXT("Two"), TEXT("two"))}))));
		}

		// ---- The component on BP_Hob's body, with the farmer near or far ----

		AVestigeNpcCharacter* HobBody = nullptr;
		UVestigePassingLineComponent* Earshot = nullptr;
		const UVestigeCharacterData* Hob = nullptr;
		ACharacter* Farmer = nullptr;
		AAIController* HobController = nullptr;

		/** Hob's body begun play and met by the player on day 1, it being day 2 at 09:00, with the farmer at Place. */
		void SetUpHob(const FVector& Place, bool bWithController = false)
		{
			UClass* HobClass = LoadClass<AVestigeNpcCharacter>(nullptr, HobPath);
			ASSERT_THAT(IsNotNull(HobClass));
			HobBody = &Spawner.SpawnActor<AVestigeNpcCharacter>(FActorSpawnParameters(), HobClass);
			HobBody->DispatchBeginPlay();
			Earshot = HobBody->FindComponentByClass<UVestigePassingLineComponent>();
			Hob = HobBody->GetCharacterData();
			ASSERT_THAT(IsNotNull(Earshot));
			ASSERT_THAT(IsNotNull(Hob));
			if (bWithController)
			{
				HobController = Cast<AAIController>(HobBody->GetController());
				if (!HobController)
				{
					HobController = &Spawner.SpawnActor<AAIController>();
					HobController->Possess(HobBody);
				}
				ASSERT_THAT(IsNotNull(HobController));
			}

			Farmer = &Spawner.SpawnActor<ACharacter>();
			Farmer->SetActorLocation(Place);
			APlayerController& Controller = Spawner.SpawnActor<APlayerController>();
			Controller.PlayerState = PlayerState;
			Controller.Possess(Farmer);

			Relationships->RecordTalk(HobId(), 1);
			SetClock(2, 9.f);
		}

		FName PassingKey() const { return GetTableKey(Talk->GetPassingLineText()); }

		void Reach(const FGameplayTag& Milestone) { Milestones->Reach(Milestone); }

		TEST_METHOD(OutOfEarshotNothingIsSaid)
		{
			SetUpHob(FarPlace);
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);

			Earshot->TrySay();

			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(Started == 0));
			// Nothing was used up: the line is still due when the farmer comes closer.
			Farmer->SetActorLocation(NearPlace);
			Earshot->TrySay();
			ASSERT_THAT(IsTrue(PassingKey() == TEXT("Hob.Passing.Harvest")));
		}

		TEST_METHOD(EnteringEarshotSaysADueLine)
		{
			SetUpHob(FarPlace);
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			Farmer->SetActorLocation(NearPlace);

			// Someone else walking in is not the farmer.
			AActor& Stranger = Spawner.SpawnActor<ACharacter>();
			Earshot->OnComponentBeginOverlap.Broadcast(Earshot, &Stranger, nullptr, 0, false, FHitResult());
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));

			Earshot->OnComponentBeginOverlap.Broadcast(Earshot, Farmer, nullptr, 0, false, FHitResult());

			ASSERT_THAT(IsTrue(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(PassingKey() == TEXT("Hob.Passing.Harvest")));
			ASSERT_THAT(IsTrue(Talk->GetPassingSpeakerActor() == HobBody));
		}

		TEST_METHOD(AMilestoneReachedNearHobIsSaidOnTheNextTick)
		{
			SetUpHob(NearPlace);

			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);

			// Not at once: the try waits for the tick, so two milestones of one action can be told apart.
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			TickTimers(0.01f);
			ASSERT_THAT(IsTrue(PassingKey() == TEXT("Hob.Passing.Harvest")));
		}

		TEST_METHOD(TwoMilestonesInOneFrameAreSaidByPriority)
		{
			SetUpHob(NearPlace);

			// The mutated one first, as the code may reach them in either order.
			Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			TickTimers(0.01f);

			ASSERT_THAT(IsTrue(Started == 1));
			ASSERT_THAT(IsTrue(PassingKey() == TEXT("Hob.Passing.Harvest")));
		}

		TEST_METHOD(ALineDueDuringTheQuietComesWhenItEnds)
		{
			SetUpHob(NearPlace);
			const double Quiet = UVestigeDialogueSettings::Get().GetPassingQuietAfterTalkSeconds();
			// The late line is due by the hour and no milestone, so no try waits for the tick and only the quiet's timer can say it.
			SetClock(2, 25.f);
			ASSERT_THAT(IsTrue(Talk->StartConversation(*HobBody, *Hob)));
			Talk->EndConversation();

			// Refused for the quiet: nothing is said, and one try is kept for when it is over.
			Earshot->TrySay();
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(Talk->GetPassingQuietSecondsLeft() > 0.0));

			PassSeconds(Quiet + 0.5);
			TickTimers(Quiet + 0.5);

			ASSERT_THAT(IsTrue(PassingKey() == TEXT("Hob.Passing.Late")));
		}

		TEST_METHOD(TheHourSaysTheLateLineToAPlayerWhoStayedNear)
		{
			SetUpHob(NearPlace);
			SetClock(2, 23.5f);

			// 00:00 is too early: the line is due from 00:15 and the clock only announces whole hours.
			ASSERT_THAT(IsTrue(Clock->AdvanceTo(24.f)));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));

			ASSERT_THAT(IsTrue(Clock->AdvanceTo(25.f)));

			ASSERT_THAT(IsTrue(PassingKey() == TEXT("Hob.Passing.Late")));
		}

		TEST_METHOD(HobTurnsToThePlayerAndBack)
		{
			SetUpHob(FVector(0.f, 500.f, 0.f), true);
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			ASSERT_THAT(IsNull(HobController->GetFocusActor()));

			Earshot->TrySay();

			ASSERT_THAT(IsTrue(Talk->IsSayingPassingLine()));
			ASSERT_THAT(IsTrue(HobController->GetFocusActor() == Farmer));

			TickTimers(Talk->GetPassingLineDuration() + 0.1);

			// Back to the placed facing: along +X, not at the farmer.
			ASSERT_THAT(IsNull(HobController->GetFocusActor()));
			ASSERT_THAT(IsTrue(HobController->GetFocalPoint().X > HobBody->GetActorLocation().X));
			ASSERT_THAT(IsTrue(FMath::Abs(HobController->GetFocalPoint().Y - HobBody->GetActorLocation().Y) < 1.f));
		}

		TEST_METHOD(TalkingToHobWhileHisLineShowsEndsTheLineAndKeepsHimTurned)
		{
			SetUpHob(FVector(0.f, 500.f, 0.f), true);
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			Earshot->TrySay();
			ASSERT_THAT(IsTrue(Talk->IsSayingPassingLine()));

			// The farmer talks to him: the line ends, and the talk's turning (not the line's end) has him.
			TArray<FVestigeInteractionOption> Options;
			HobBody->GetInteractionOptions_Implementation(PlayerState, Options);
			ASSERT_THAT(IsTrue(Options.Num() == 1));
			HobBody->Interact_Implementation(PlayerState, Options[0]);

			ASSERT_THAT(IsTrue(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
			// Turned to whoever talked to him (the interactor, as the talk's own test has it), and not sent home by the line's end.
			ASSERT_THAT(IsTrue(HobController->GetFocusActor() == PlayerState));
		}

		TEST_METHOD(TheComponentOnAnotherKindOfBodyDoesNothing)
		{
			AActor& Plain = Spawner.SpawnActor<AActor>();
			USceneComponent* Root = NewObject<USceneComponent>(&Plain);
			Plain.SetRootComponent(Root);
			Root->RegisterComponent();
			UVestigePassingLineComponent* Component = NewObject<UVestigePassingLineComponent>(&Plain);
			Component->SetupAttachment(Root);
			Component->RegisterComponent();
			TestRunner->AddExpectedMessagePlain(TEXT("isn't a character body"), ELogVerbosity::Error);

			Plain.DispatchBeginPlay();
			Component->TrySay();

			ASSERT_THAT(IsFalse(Component->GetGenerateOverlapEvents()));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
		}
	};
}

#endif
