// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorTestSpawner.h"
#include "Content/VestigeToolData.h"
#include "CQTest.h"
#include "Farming/FarmToolbar.h"

// A namespace keeps the fixture's short name from hiding other declarations in unity builds.
namespace FarmToolbarTests
{
	TEST_CLASS(Toolbar, "Vestige")
	{
		FActorTestSpawner Spawner;

		/** A tool asset with this ContentId. */
		const UVestigeToolData* MakeTool(FName ContentId)
		{
			UVestigeToolData& Tool = Spawner.SpawnObject<UVestigeToolData>();
			Tool.SetContentIdForTests(ContentId);
			return &Tool;
		}

		/** A toolbar holding a hoe, an empty slot and a watering can. */
		FFarmToolbar MakeToolbar()
		{
			FFarmToolbar FarmToolbar;
			FarmToolbar.SetTools({MakeTool(TEXT("Base.Hoe")), nullptr, MakeTool(TEXT("Base.WateringCan"))});
			return FarmToolbar;
		}

		TEST_METHOD(FirstSlotIsInHandAtTheStart)
		{
			const FFarmToolbar FarmToolbar = MakeToolbar();

			ASSERT_THAT(IsTrue(FarmToolbar.GetCurrentSlot() == 0));
			ASSERT_THAT(IsTrue(FarmToolbar.GetCurrentTool() == FarmToolbar.GetTools()[0]));
		}

		TEST_METHOD(SelectingASlotPutsItsToolInHand)
		{
			FFarmToolbar FarmToolbar = MakeToolbar();

			ASSERT_THAT(IsTrue(FarmToolbar.SelectSlot(2)));
			ASSERT_THAT(IsTrue(FarmToolbar.GetCurrentTool() == FarmToolbar.GetTools()[2]));
		}

		TEST_METHOD(EmptySlotHoldsNoTool)
		{
			FFarmToolbar FarmToolbar = MakeToolbar();

			ASSERT_THAT(IsTrue(FarmToolbar.SelectSlot(1)));
			ASSERT_THAT(IsTrue(FarmToolbar.GetCurrentTool() == nullptr));
		}

		TEST_METHOD(SlotOutsideTheToolbarIsRefused)
		{
			FFarmToolbar FarmToolbar = MakeToolbar();
			ASSERT_THAT(IsTrue(FarmToolbar.SelectSlot(2)));

			ASSERT_THAT(IsFalse(FarmToolbar.SelectSlot(3)));
			ASSERT_THAT(IsFalse(FarmToolbar.SelectSlot(-1)));
			ASSERT_THAT(IsTrue(FarmToolbar.GetCurrentSlot() == 2));
		}

		TEST_METHOD(SelectingAToolByIdFindsItsSlot)
		{
			FFarmToolbar FarmToolbar = MakeToolbar();

			ASSERT_THAT(IsTrue(FarmToolbar.SelectTool(TEXT("Base.WateringCan"))));
			ASSERT_THAT(IsTrue(FarmToolbar.GetCurrentSlot() == 2));
		}

		TEST_METHOD(UnknownToolIdIsRefused)
		{
			FFarmToolbar FarmToolbar = MakeToolbar();

			ASSERT_THAT(IsFalse(FarmToolbar.SelectTool(TEXT("Base.Scythe"))));
			ASSERT_THAT(IsFalse(FarmToolbar.SelectTool(NAME_None)));
			ASSERT_THAT(IsTrue(FarmToolbar.GetCurrentSlot() == 0));
		}
	};
}

#endif
