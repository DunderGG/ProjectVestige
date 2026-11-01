// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "CQTest.h"
#include "UI/VestigeHudMessages.h"

#define LOCTEXT_NAMESPACE "VestigeHudMessagesTests"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace VestigeHudMessagesTests
{
	constexpr double Lifetime = 3.0;
	constexpr int32 MaxMessages = 3;

	TEST_CLASS(HudMessages, "Vestige")
	{
		TEST_METHOD(EachMessageExpiresOnItsOwnTimer)
		{
			FVestigeHudMessages Messages;
			Messages.Add(LOCTEXT("Slept", "You slept."), 10.0, MaxMessages);
			Messages.Add(LOCTEXT("Saved", "Game saved."), 11.0, MaxMessages);

			Messages.RemoveExpired(12.9, Lifetime);
			ASSERT_THAT(IsTrue(Messages.GetMessages().Num() == 2));

			Messages.RemoveExpired(13.0, Lifetime);
			ASSERT_THAT(IsTrue(Messages.GetMessages().Num() == 1));
			ASSERT_THAT(IsTrue(Messages.GetMessages()[0].Text.EqualTo(LOCTEXT("Saved", "Game saved."))));

			Messages.RemoveExpired(14.0, Lifetime);
			ASSERT_THAT(IsTrue(Messages.IsEmpty()));
		}

		TEST_METHOD(TheNextExpiryIsTheOldestMessage)
		{
			FVestigeHudMessages Messages;
			ASSERT_THAT(IsFalse(Messages.GetSecondsUntilNextExpiry(0.0, Lifetime).IsSet()));

			Messages.Add(LOCTEXT("First", "First."), 10.0, MaxMessages);
			Messages.Add(LOCTEXT("Second", "Second."), 11.0, MaxMessages);

			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Messages.GetSecondsUntilNextExpiry(11.5, Lifetime).GetValue(), 1.5)));
		}

		TEST_METHOD(MessagesArrivingTogetherStaySeparate)
		{
			FVestigeHudMessages Messages;
			Messages.Add(LOCTEXT("Slept", "You slept."), 10.0, MaxMessages);
			Messages.Add(LOCTEXT("Saved", "Game saved."), 10.0, MaxMessages);

			ASSERT_THAT(IsTrue(Messages.GetMessages().Num() == 2));
		}

		TEST_METHOD(AFourthMessagePushesOutTheOldest)
		{
			FVestigeHudMessages Messages;
			Messages.Add(LOCTEXT("First", "First."), 10.0, MaxMessages);
			Messages.Add(LOCTEXT("Second", "Second."), 10.1, MaxMessages);
			Messages.Add(LOCTEXT("Third", "Third."), 10.2, MaxMessages);

			Messages.Add(LOCTEXT("Fourth", "Fourth."), 10.3, MaxMessages);

			ASSERT_THAT(IsTrue(Messages.GetMessages().Num() == MaxMessages));
			ASSERT_THAT(IsTrue(Messages.GetMessages()[0].Text.EqualTo(LOCTEXT("Second", "Second."))));
			ASSERT_THAT(IsTrue(Messages.GetMessages().Last().Text.EqualTo(LOCTEXT("Fourth", "Fourth."))));
		}

		TEST_METHOD(AMessageFadesInAndOut)
		{
			using namespace VestigeHudMessageFade;
			const double Start = 10.0;

			ASSERT_THAT(IsTrue(FVestigeHudMessages::GetOpacity(Start, Start, Lifetime) == 0.f));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(FVestigeHudMessages::GetOpacity(Start, Start + FadeInSeconds * 0.5, Lifetime), 0.5f)));
			ASSERT_THAT(IsTrue(FVestigeHudMessages::GetOpacity(Start, Start + Lifetime * 0.5, Lifetime) == 1.f));
			ASSERT_THAT(IsTrue(
				FMath::IsNearlyEqual(FVestigeHudMessages::GetOpacity(Start, Start + Lifetime - FadeOutSeconds * 0.5, Lifetime), 0.5f)));
			ASSERT_THAT(IsTrue(FVestigeHudMessages::GetOpacity(Start, Start + Lifetime, Lifetime) == 0.f));
		}
	};
}

#undef LOCTEXT_NAMESPACE

#endif
