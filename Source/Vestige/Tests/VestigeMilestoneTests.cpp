// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Components/ActorTestSpawner.h"
#include "Core/VestigeMilestoneSaveData.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "Core/VestigeSaveRecord.h"
#include "Core/VestigeSaveRegistry.h"
#include "CQTest.h"
#include "GameplayTagContainer.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeMilestoneTests
{
	TEST_CLASS(Milestones, "Vestige")
	{
		FActorTestSpawner Spawner;
		UVestigeMilestoneSubsystem* Subsystem = nullptr;

		BEFORE_EACH()
		{
			Spawner.InitializeGameSubsystems();
			Subsystem = UVestigeMilestoneSubsystem::Get(&Spawner.GetWorld());
			ASSERT_THAT(IsNotNull(Subsystem));
		}

		TEST_METHOD(ReachingTwiceCountsOnce)
		{
			const FGameplayTag Harvested = VestigeGameplayTags::Milestone_Farm_Harvested;
			ASSERT_THAT(IsFalse(Subsystem->HasReached(Harvested)));

			ASSERT_THAT(IsTrue(Subsystem->Reach(Harvested)));
			ASSERT_THAT(IsFalse(Subsystem->Reach(Harvested)));

			ASSERT_THAT(IsTrue(Subsystem->HasReached(Harvested)));
			ASSERT_THAT(IsTrue(Subsystem->GetReached().Num() == 1));
		}

		TEST_METHOD(ReachingANewMilestoneBroadcastsOnceAndLoadingDoesNot)
		{
			TArray<FGameplayTag> Heard;
			Subsystem->OnMilestoneReached.AddLambda(
				[&Heard](const FGameplayTag& Milestone)
				{
					Heard.Add(Milestone);
				});
			const FGameplayTag Harvested = VestigeGameplayTags::Milestone_Farm_Harvested;

			ASSERT_THAT(IsTrue(Subsystem->Reach(Harvested)));
			// The milestone is recorded when listeners hear of it, so they can read it.
			ASSERT_THAT(IsTrue(Heard == TArray<FGameplayTag>{Harvested}));
			ASSERT_THAT(IsFalse(Subsystem->Reach(Harvested)));
			ASSERT_THAT(IsFalse(Subsystem->Reach(FGameplayTag())));
			ASSERT_THAT(IsTrue(Heard.Num() == 1));

			// A load puts back what was reached before: nothing new happened, so nothing is heard.
			FVestigeMilestoneSaveData Data;
			Data.Reached.Add(TEXT("Vestige.Milestone.Farm.Sold"));
			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Subsystem->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));
			ASSERT_THAT(IsTrue(Subsystem->HasReached(VestigeGameplayTags::Milestone_Farm_Sold)));
			ASSERT_THAT(IsTrue(Heard.Num() == 1));
			Subsystem->StartNewGame();
			ASSERT_THAT(IsTrue(Heard.Num() == 1));
		}

		TEST_METHOD(AnInvalidTagIsNeverReached)
		{
			ASSERT_THAT(IsFalse(Subsystem->Reach(FGameplayTag())));

			ASSERT_THAT(IsFalse(Subsystem->HasReached(FGameplayTag())));
			ASSERT_THAT(IsTrue(Subsystem->GetReached().IsEmpty()));
		}

		TEST_METHOD(MilestonesAreSavedAndANewGameForgetsThem)
		{
			Subsystem->Reach(VestigeGameplayTags::Milestone_Farm_Sold);
			Subsystem->Reach(VestigeGameplayTags::Milestone_Farm_Harvested);
			ASSERT_THAT(IsTrue(Subsystem->GetSaveKey() == TEXT("Base.Milestones")));
			ASSERT_THAT(IsTrue(Subsystem->GetSaveVersion() == FVestigeMilestoneSaveData::Version));
			const FVestigeSaveRecord Record = Subsystem->WriteSaveRecord();

			Subsystem->StartNewGame();
			ASSERT_THAT(IsTrue(Subsystem->GetReached().IsEmpty()));

			int32 LeftOut = 0;
			ASSERT_THAT(IsTrue(Subsystem->ReadSaveRecord(Record, LeftOut)));

			ASSERT_THAT(IsTrue(LeftOut == 0));
			ASSERT_THAT(IsTrue(Subsystem->GetReached().Num() == 2));
			ASSERT_THAT(IsTrue(Subsystem->HasReached(VestigeGameplayTags::Milestone_Farm_Sold)));
			ASSERT_THAT(IsTrue(Subsystem->HasReached(VestigeGameplayTags::Milestone_Farm_Harvested)));

			Subsystem->StartNewGame();
			ASSERT_THAT(IsFalse(Subsystem->HasReached(VestigeGameplayTags::Milestone_Farm_Sold)));
		}

		TEST_METHOD(ALoadReplacesWhatWasReached)
		{
			Subsystem->Reach(VestigeGameplayTags::Milestone_Farm_Cleaned);
			FVestigeMilestoneSaveData Data;
			Data.Reached.Add(TEXT("Vestige.Milestone.Farm.Planted"));
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Subsystem->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			ASSERT_THAT(IsFalse(Subsystem->HasReached(VestigeGameplayTags::Milestone_Farm_Cleaned)));
			ASSERT_THAT(IsTrue(Subsystem->HasReached(VestigeGameplayTags::Milestone_Farm_Planted)));
		}

		TEST_METHOD(AnUnknownSavedMilestoneIsSkipped)
		{
			FVestigeMilestoneSaveData Data;
			Data.Reached.Add(TEXT("Vestige.Milestone.Farm.Harvested"));
			Data.Reached.Add(TEXT("Vestige.Milestone.Farm.NoSuchMilestone"));
			TestRunner->AddExpectedMessagePlain(TEXT("no longer exists"), ELogVerbosity::Warning);
			int32 LeftOut = 0;

			ASSERT_THAT(IsTrue(Subsystem->ReadSaveRecord(FVestigeSaveRecord::Write(Data), LeftOut)));

			ASSERT_THAT(IsTrue(LeftOut == 1));
			ASSERT_THAT(IsTrue(Subsystem->GetReached().Num() == 1));
			ASSERT_THAT(IsTrue(Subsystem->HasReached(VestigeGameplayTags::Milestone_Farm_Harvested)));
		}

		TEST_METHOD(ASavedMilestoneComesBackThroughTheRegistry)
		{
			FVestigeMilestoneSaveData Data;
			Data.Reached.Add(TEXT("Vestige.Milestone.Farm.Sold"));

			UVestigeSaveRegistry::Get(&Spawner.GetWorld())->Load({{TEXT("Base.Milestones"), FVestigeSaveRecord::Write(Data)}});

			ASSERT_THAT(IsTrue(Subsystem->HasReached(VestigeGameplayTags::Milestone_Farm_Sold)));
		}
	};
}

#endif
