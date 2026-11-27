// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Characters/VestigeConversationComponent.h"
#include "Characters/VestigeNpcCharacter.h"
#include "Characters/VestigePassingLineComponent.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Components/ActorTestSpawner.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Content/VestigeDialogueData.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "Core/VestigeSaveRecord.h"
#include "CQTest.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Internationalization/Text.h"
#include "Player/VestigePlayerState.h"
#include "Time/VestigeClockSaveData.h"
#include "Time/VestigeClockSubsystem.h"

// Hob's three lines said in passing, on the real data: when each is due, that it is said once and at most one a day, and that his
// topics wait a day after one (Systems/11-Dialogue.md, "Hob's passing lines on the real data").

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeHobPassingTests
{
	const TCHAR* const HobPath = TEXT("/Game/Vestige/Characters/BP_Hob.BP_Hob_C");

	// A function, not a static: UVestigeCharacterData::AssetType is another file's static, which a unity build may initialize after this one.
	FPrimaryAssetId HobId()
	{
		return FPrimaryAssetId(UVestigeCharacterData::AssetType, TEXT("Base.Hob"));
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

	TEST_CLASS(HobPassing, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeContentCatalog* Catalog = nullptr;
		UVestigeClockSubsystem* Clock = nullptr;
		UVestigeMilestoneSubsystem* Milestones = nullptr;
		AVestigePlayerState* PlayerState = nullptr;
		UVestigeConversationComponent* Talk = nullptr;
		UVestigeRelationshipComponent* Relationships = nullptr;
		const UVestigeCharacterData* Hob = nullptr;
		AActor* HobActor = nullptr;

		/** The player has met Hob on day 1, and it is day 2 at 09:00, with nothing else done. */
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

			Hob = Catalog->Find<UVestigeCharacterData>(HobId());
			ASSERT_THAT(IsNotNull(Hob));
			HobActor = &Spawner.SpawnActor<AActor>();

			Relationships->RecordTalk(HobId(), 1);
			SetDay(2);
		}

		// ---- Helpers ----

		void SetDay(int32 Day, float Hour = 9.f)
		{
			FVestigeClockSaveData Data;
			Data.Day = Day;
			Data.TimeOfDay = Hour;
			Clock->RestoreFromSaveData(Data);
		}

		/** Ends the line showing, as the day's end does, and moves the clock to Day at Hour. */
		void MoveTo(int32 Day, float Hour = 9.f)
		{
			Clock->EndDay();
			SetDay(Day, Hour);
		}

		FGameplayTag Flag(const TCHAR* Name) const { return MakeTag(*(FString(TEXT("Vestige.Relationship.")) + Name)); }

		void SetFlag(const TCHAR* Name, int32 Day) { Relationships->SetFlag(*Hob, Flag(Name), Day); }

		bool HasFlag(const TCHAR* Name) const { return Relationships->HasFlag(HobId(), Flag(Name)); }

		/** The day the flag was last set, or INDEX_NONE. */
		int32 GetFlagDay(const TCHAR* Name) const
		{
			const int32* Day = Relationships->GetFlagDays(HobId()).Find(Flag(Name));
			return Day ? *Day : INDEX_NONE;
		}

		void Reach(const FGameplayTag& Milestone) { Milestones->Reach(Milestone); }

		/** Asks Hob for a line in passing now. Returns the key of the line said, or none. */
		FName Say() { return Talk->SayPassingLine(*HobActor, *Hob) ? GetTableKey(Talk->GetPassingLineText()) : NAME_None; }

		/** Hob's topics that need no event are answered, so the only topic left to come is one an event opens. */
		void AnswerTheTopicsWithNoEvent()
		{
			for (const TCHAR* Topic : {TEXT("Topic.Dispensers"), TEXT("Topic.FarRows"), TEXT("Topic.Well")})
			{
				SetFlag(Topic, 1);
			}
		}

		bool Start() { return Talk->StartConversation(*HobActor, *Hob); }

		FName LineKey() const { return GetTableKey(Talk->GetLineText()); }

		/** The key of the first line of the talk Hob gives now, which is then ended (an Escape). None when no talk starts. */
		FName TalkKey()
		{
			if (!Start())
			{
				return NAME_None;
			}
			const FName Key = LineKey();
			Talk->EndConversation();
			return Key;
		}

		const UVestigeDialogueData* FindDialogue(const TCHAR* ContentId) const
		{
			return Catalog->Find<UVestigeDialogueData>(FPrimaryAssetId(UVestigeDialogueData::AssetType, ContentId));
		}

		/** Every conversation of any character said in passing. */
		TArray<const UVestigeDialogueData*> FindPassing() const
		{
			TArray<const UVestigeContentData*> All;
			Catalog->FindAll(UVestigeDialogueData::AssetType, All);
			TArray<const UVestigeDialogueData*> Passing;
			for (const UVestigeContentData* Content : All)
			{
				const UVestigeDialogueData* Dialogue = Cast<UVestigeDialogueData>(Content);
				if (Dialogue && Dialogue->GetDelivery() == EVestigeDialogueDelivery::Passing)
				{
					Passing.Add(Dialogue);
				}
			}
			return Passing;
		}

		// ---- The data ----

		TEST_METHOD(HobHasThreePassingLines)
		{
			struct FExpected
			{
				const TCHAR* ContentId;
				const TCHAR* LineId;
				int32 Priority;
			};
			const FExpected Expected[] = {{TEXT("Base.HobPassingHarvest"), TEXT("Hob.Passing.Harvest"), 30},
				{TEXT("Base.HobPassingBloom"), TEXT("Hob.Passing.Bloom"), 20}, {TEXT("Base.HobPassingLate"), TEXT("Hob.Passing.Late"), 10}};

			for (const FExpected& Line : Expected)
			{
				const UVestigeDialogueData* Dialogue = FindDialogue(Line.ContentId);
				ASSERT_THAT(IsNotNull(Dialogue, Line.ContentId));
				ASSERT_THAT(IsTrue(Dialogue->GetDelivery() == EVestigeDialogueDelivery::Passing, Line.ContentId));
				ASSERT_THAT(IsTrue(Dialogue->GetSpeaker() == HobId(), Line.ContentId));
				ASSERT_THAT(IsTrue(Dialogue->GetPriority() == Line.Priority, Line.ContentId));
				ASSERT_THAT(IsTrue(Dialogue->GetLines().Num() == 1, Line.ContentId));
				ASSERT_THAT(IsTrue(Dialogue->GetLines()[0].LineId == FName(Line.LineId), Line.ContentId));
			}
			ASSERT_THAT(IsTrue(FindPassing().Num() == 3));
		}

		TEST_METHOD(EveryPassingLineFitsTheCaptionAndIsFromTheStringTable)
		{
			for (const UVestigeDialogueData* Dialogue : FindPassing())
			{
				ASSERT_THAT(IsTrue(Dialogue->GetLines().Num() == 1, *Dialogue->GetContentId().ToString()));
				const FVestigeDialogueLine& Line = Dialogue->GetLines()[0];
				ASSERT_THAT(IsTrue(Line.Texts.Num() == 1 && Line.Choices.IsEmpty() && Line.Next.IsNone(), *Line.LineId.ToString()));
				const FText& Text = Line.Texts[0];
				ASSERT_THAT(IsFalse(Text.IsEmpty(), *Line.LineId.ToString()));
				ASSERT_THAT(IsTrue(Text.ToString().Len() <= VestigeDialogue::MaxPassingLineLength, *Line.LineId.ToString()));
				// A passing text's table key is its line's ID, as every text's key starts with it.
				ASSERT_THAT(IsTrue(GetTableKey(Text) == Line.LineId, *Line.LineId.ToString()));
			}
		}

		// ---- When each is due ----

		TEST_METHOD(TheFirstHarvestIsSaidOnItsDayAndNotOnALaterOne)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);

			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Harvest")));
			ASSERT_THAT(IsTrue(GetFlagDay(TEXT("Passing.Harvest")) == 2));
			ASSERT_THAT(IsTrue(GetFlagDay(TEXT("PassingSaid")) == 2));

			// Not said that day, it is dropped: the morning after, the harvest is no longer news.
			StartOver();
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			MoveTo(3);
			ASSERT_THAT(IsTrue(Say() == NAME_None));
		}

		TEST_METHOD(TheFirstBloomCropIsSaidOnItsDay)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);

			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Bloom")));

			ASSERT_THAT(IsTrue(GetFlagDay(TEXT("Passing.Bloom")) == 2));
			ASSERT_THAT(IsTrue(GetFlagDay(TEXT("PassingSaid")) == 2));
		}

		TEST_METHOD(TheLateLineSetsItsOwnFlag)
		{
			SetDay(2, 25.f);

			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Late")));

			ASSERT_THAT(IsTrue(GetFlagDay(TEXT("Passing.Late")) == 2));
			ASSERT_THAT(IsTrue(GetFlagDay(TEXT("PassingSaid")) == 2));
		}

		TEST_METHOD(NothingIsSaidWithNothingToReactTo)
		{
			// A plain morning, and an evening: no milestone, and not after midnight.
			ASSERT_THAT(IsTrue(Say() == NAME_None));
			SetDay(2, 21.5f);
			ASSERT_THAT(IsTrue(Say() == NAME_None));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
		}

		TEST_METHOD(TheLateLineNeverComesBefore0015)
		{
			// 24.0 is 00:00, where the HUD warns about the late hour: the line waits for 00:15 so the two never land together.
			for (const float Hour : {9.f, 23.9f, 24.f, 24.2f})
			{
				SetDay(2, Hour);
				ASSERT_THAT(IsTrue(Say() == NAME_None, *FString::SanitizeFloat(Hour)));
			}

			SetDay(2, 24.25f);
			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Late")));
		}

		TEST_METHOD(TheLateLineIsDueUntilTheDayEnds)
		{
			SetDay(2, Clock->GetDayEndHour() - 0.1f);

			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Late")));
		}

		// ---- Once, and at most one a day ----

		TEST_METHOD(TheLateLineIsSaidOncePerGameAndTheFlagIsSaved)
		{
			SetDay(2, 25.f);
			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Late")));
			MoveTo(3, 25.f);
			// A later night: nothing else stops it, but the flag.
			ASSERT_THAT(IsTrue(Say() == NAME_None));
			const FVestigeSaveRecord Record = Relationships->WriteSaveRecord();

			// A new game doesn't know it, and so he says it again.
			StartOver();
			MoveTo(3, 25.f);
			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Late")));

			// A loaded game does.
			MoveTo(4, 25.f);
			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Relationships->ReadSaveRecord(Record, LeftOut)));
			ASSERT_THAT(IsTrue(HasFlag(TEXT("Passing.Late"))));
			ASSERT_THAT(IsTrue(Say() == NAME_None));
		}

		TEST_METHOD(AtMostOnePassingLineADay)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);

			// The first harvest is also the first Bloom crop: the harvest line comes, and the Bloom line is dropped.
			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Harvest")));
			MoveTo(2, 9.5f);
			ASSERT_THAT(IsTrue(Say() == NAME_None));
			// Not even the late line, the same game day.
			MoveTo(2, 25.f);
			ASSERT_THAT(IsTrue(Say() == NAME_None));
			ASSERT_THAT(IsFalse(HasFlag(TEXT("Passing.Bloom"))));
			ASSERT_THAT(IsFalse(HasFlag(TEXT("Passing.Late"))));

			// The next night the late line is free to come; the Bloom line's day has passed.
			MoveTo(3, 25.f);
			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Late")));
			ASSERT_THAT(IsFalse(HasFlag(TEXT("Passing.Bloom"))));
		}

		TEST_METHOD(ALineSaidShowsItsTimeAndGivesNoTrustOrTalk)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			const int32 TrustBefore = Relationships->GetTrust(*Hob);
			const int32 EarnedBefore = Relationships->GetEarnedTrust(HobId());

			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Harvest")));

			// 44 characters: 1.5 s and one second for every 15 characters.
			ASSERT_THAT(IsTrue(Talk->GetPassingLineDuration() > 3.0 && Talk->GetPassingLineDuration() < 8.0));
			ASSERT_THAT(IsTrue(Relationships->GetTrust(*Hob) == TrustBefore));
			ASSERT_THAT(IsTrue(Relationships->GetEarnedTrust(HobId()) == EarnedBefore));
			ASSERT_THAT(IsTrue(Relationships->GetTalksToday(HobId(), 2) == 0));
			ASSERT_THAT(IsFalse(Talk->IsInConversation()));
			ASSERT_THAT(IsFalse(Clock->IsHeld()));
		}

		// ---- Whom he says them to, and what he has already talked about ----

		TEST_METHOD(NoPassingLineBeforeMeetingHob)
		{
			Relationships->StartNewGame();
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);

			ASSERT_THAT(IsTrue(Say() == NAME_None));
			SetDay(2, 25.f);
			ASSERT_THAT(IsTrue(Say() == NAME_None));

			ASSERT_THAT(IsTrue(Relationships->GetFlagDays(HobId()).IsEmpty()));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));
		}

		TEST_METHOD(NoHarvestLineAfterTheHarvestTopicWasTalkedAbout)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			SetFlag(TEXT("Topic.Harvest"), 2);

			ASSERT_THAT(IsTrue(Say() == NAME_None));
		}

		TEST_METHOD(NoBloomLineAfterTheBloomTopicWasTalkedAbout)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);
			SetFlag(TEXT("Topic.Bloom"), 2);

			ASSERT_THAT(IsTrue(Say() == NAME_None));
		}

		TEST_METHOD(WithoutAPassingLineTheHarvestTopicComesOnTheSameDay)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);

			ASSERT_THAT(IsTrue(TalkKey() == TEXT("Hob.Topic.Harvest.Open")));
		}

		TEST_METHOD(TheHarvestTopicWaitsADayAfterTheHarvestLine)
		{
			AnswerTheTopicsWithNoEvent();
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Harvest")));

			// Hob has just said it, so the same day's talk is not the same thing again.
			const FName SameDay = TalkKey();
			ASSERT_THAT(IsTrue(SameDay != NAME_None && SameDay != TEXT("Hob.Topic.Harvest.Open")));
			ASSERT_THAT(IsFalse(Talk->IsSayingPassingLine()));

			SetDay(3);
			ASSERT_THAT(IsTrue(TalkKey() == TEXT("Hob.Topic.Harvest.Open")));
		}

		TEST_METHOD(WithoutAPassingLineTheBloomTopicComesOnTheSameDay)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);

			ASSERT_THAT(IsTrue(TalkKey() == TEXT("Hob.Topic.Bloom.Open")));
		}

		TEST_METHOD(TheBloomTopicWaitsADayAfterTheBloomLine)
		{
			AnswerTheTopicsWithNoEvent();
			Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);
			ASSERT_THAT(IsTrue(Say() == TEXT("Hob.Passing.Bloom")));

			const FName SameDay = TalkKey();
			ASSERT_THAT(IsTrue(SameDay != NAME_None && SameDay != TEXT("Hob.Topic.Bloom.Open")));

			SetDay(3);
			ASSERT_THAT(IsTrue(TalkKey() == TEXT("Hob.Topic.Bloom.Open")));
		}

		TEST_METHOD(APassingLineNeverOpensTheDialogueBox)
		{
			Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			Reach(VestigeGameplayTags::Milestone_Farm_HarvestedMutated);
			SetDay(2, 25.f);

			// Every passing asset is due, and none is a talk: the talk is a greeting or a topic, never a passing line.
			const FName Key = TalkKey();
			ASSERT_THAT(IsTrue(Key != NAME_None));
			ASSERT_THAT(IsFalse(Key.ToString().StartsWith(TEXT("Hob.Passing."))));
		}

		// ---- BP_Hob's earshot ----

		TEST_METHOD(HobsEarshotIsEighteenMetresAndHearsOnlyPawns)
		{
			UClass* HobClass = LoadClass<AVestigeNpcCharacter>(nullptr, HobPath);
			ASSERT_THAT(IsNotNull(HobClass));
			AVestigeNpcCharacter& Body = Spawner.SpawnActor<AVestigeNpcCharacter>(FActorSpawnParameters(), HobClass);

			TArray<UVestigePassingLineComponent*> Components;
			Body.GetComponents<UVestigePassingLineComponent>(Components);
			ASSERT_THAT(IsTrue(Components.Num() == 1));
			const UVestigePassingLineComponent* Earshot = Components[0];

			// 18 m, as the game designer set it: the field is heard, the well is not.
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Earshot->GetScaledSphereRadius(), 1800.f)));
			ASSERT_THAT(IsTrue(Earshot->GetAttachParent() == Body.GetRootComponent()));
			ASSERT_THAT(IsTrue(Earshot->GetCollisionEnabled() == ECollisionEnabled::QueryOnly));
			ASSERT_THAT(IsTrue(Earshot->GetGenerateOverlapEvents()));
			ASSERT_THAT(IsFalse(Earshot->CanEverAffectNavigation()));
			ASSERT_THAT(IsFalse(Earshot->PrimaryComponentTick.bCanEverTick));
			// Pawns overlap it and everything else passes through.
			ASSERT_THAT(IsTrue(Earshot->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap));
			for (int32 Channel = 0; Channel < ECC_MAX; ++Channel)
			{
				if (Channel != ECC_Pawn)
				{
					ASSERT_THAT(IsTrue(Earshot->GetCollisionResponseToChannel(static_cast<ECollisionChannel>(Channel)) == ECR_Ignore));
				}
			}
		}

		/** A new player: Hob met, nothing else, and no milestone. */
		void StartOver()
		{
			Relationships->StartNewGame();
			Milestones->StartNewGame();
			Relationships->RecordTalk(HobId(), 1);
		}
	};
}

#endif
