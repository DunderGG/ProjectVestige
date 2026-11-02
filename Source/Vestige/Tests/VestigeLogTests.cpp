// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Components/SceneComponent.h"
#include "Core/VestigeLog.h"
#include "CQTest.h"
#include "GameFramework/Actor.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/ScopeLock.h"

DEFINE_LOG_CATEGORY_STATIC(LogVestigeLogTest, Log, All);

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeLogTests
{
	/** Keeps every line written to LogVestigeLogTest while it exists. */
	class FLogCapture : public FOutputDevice
	{
	public:
		FLogCapture() { GLog->AddOutputDevice(this); }

		virtual ~FLogCapture() override { GLog->RemoveOutputDevice(this); }

		/** The captured lines, after anything still buffered has been written. */
		TArray<FString> GetLines()
		{
			GLog->Flush();
			FScopeLock Lock(&LinesLock);
			return Lines;
		}

		virtual bool CanBeUsedOnAnyThread() const override { return true; }

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category == LogVestigeLogTest.GetCategoryName())
			{
				FScopeLock Lock(&LinesLock);
				Lines.Add(Message);
			}
		}

	private:
		FCriticalSection LinesLock;
		TArray<FString> Lines;
	};

	TEST_CLASS(Logging, "Vestige")
	{
		FActorTestSpawner Spawner;

		TEST_METHOD(LinesStartWithTheSenderAndFunction)
		{
			const AActor& Sender = Spawner.SpawnActor<AActor>();
			FLogCapture Capture;

			VESTIGE_LOG(&Sender, LogVestigeLogTest, Log, "Planted {Count} {ItemId}.", 3, FName(TEXT("Base.TurnipSeeds")));

			const TArray<FString> Lines = Capture.GetLines();
			ASSERT_THAT(IsTrue(Lines.Num() == 1));
			const FString Expected =
				FString::Printf(TEXT("[%s] %s: Planted 3 Base.TurnipSeeds."), *Sender.GetName(), ANSI_TO_TCHAR(__FUNCTION__));
			ASSERT_THAT(AreEqual(Expected, Lines[0]));
		}

		TEST_METHOD(AComponentIsNamedByItsOwnerAndItself)
		{
			AActor& OwnerActor = Spawner.SpawnActor<AActor>();
			const USceneComponent* Component = NewObject<USceneComponent>(&OwnerActor, TEXT("Probe"));

			ASSERT_THAT(AreEqual(OwnerActor.GetName() + TEXT(".Probe"), VestigeLog::GetSenderName(Component)));
		}

		TEST_METHOD(AnActorIsNamedByItselfAndNoSenderByNone)
		{
			const AActor& Actor = Spawner.SpawnActor<AActor>();

			ASSERT_THAT(AreEqual(Actor.GetName(), VestigeLog::GetSenderName(&Actor)));
			ASSERT_THAT(AreEqual(FString(TEXT("None")), VestigeLog::GetSenderName(nullptr)));
		}

		TEST_METHOD(NothingIsEvaluatedWhenTheCategoryIsOff)
		{
			int32 Evaluations = 0;
			const auto Evaluate = [&Evaluations]()
			{
				++Evaluations;
				return Evaluations;
			};
			const ELogVerbosity::Type PreviousVerbosity = LogVestigeLogTest.GetVerbosity();
			LogVestigeLogTest.SetVerbosity(ELogVerbosity::Warning);

			VESTIGE_LOG(nullptr, LogVestigeLogTest, Log, "Evaluated {Count} times.", Evaluate());

			LogVestigeLogTest.SetVerbosity(PreviousVerbosity);
			ASSERT_THAT(IsTrue(Evaluations == 0));
		}
	};
}

#endif
