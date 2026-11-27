// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Core/VestigeMilestoneSubsystem.h"

#include "Core/VestigeLog.h"
#include "Core/VestigeMilestoneSaveData.h"
#include "Core/VestigeSaveRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UVestigeMilestoneSubsystem* UVestigeMilestoneSubsystem::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UVestigeMilestoneSubsystem>() : nullptr;
}

void UVestigeMilestoneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The registry holds the save part.
	SaveRegistry = Collection.InitializeDependency<UVestigeSaveRegistry>();
	if (SaveRegistry)
	{
		SaveRegistry->RegisterGameOwner(*this);
	}
}

void UVestigeMilestoneSubsystem::Deinitialize()
{
	if (SaveRegistry)
	{
		SaveRegistry->UnregisterGameOwner(*this);
	}
	Super::Deinitialize();
}

bool UVestigeMilestoneSubsystem::Reach(const FGameplayTag& Milestone)
{
	if (!Milestone.IsValid() || Reached.HasTagExact(Milestone))
	{
		return false;
	}
	Reached.AddTag(Milestone);
	VESTIGE_LOG(this, LogVestige, Log, "Milestone reached: {Milestone}.", Milestone.ToString());
	return true;
}

bool UVestigeMilestoneSubsystem::HasReached(const FGameplayTag& Milestone) const
{
	return Milestone.IsValid() && Reached.HasTagExact(Milestone);
}

void UVestigeMilestoneSubsystem::StartNewGame()
{
	Reached.Reset();
}

FName UVestigeMilestoneSubsystem::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Milestones"));
	return Key;
}

int32 UVestigeMilestoneSubsystem::GetSaveVersion() const
{
	return FVestigeMilestoneSaveData::Version;
}

FVestigeSaveRecord UVestigeMilestoneSubsystem::WriteSaveRecord() const
{
	FVestigeMilestoneSaveData Data;
	Data.Reached.Reserve(Reached.Num());
	for (const FGameplayTag& Milestone : Reached)
	{
		Data.Reached.Add(Milestone.GetTagName());
	}

	// A container has no order; sorted, the same state always writes the same bytes.
	Data.Reached.Sort(
		[](const FName& A, const FName& B)
		{
			return A.LexicalLess(B);
		});
	return FVestigeSaveRecord::Write(Data);
}

bool UVestigeMilestoneSubsystem::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FVestigeMilestoneSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}

	Reached.Reset();
	for (const FName& Name : Data.Reached)
	{
		const FGameplayTag Milestone = FGameplayTag::RequestGameplayTag(Name, false);
		if (!Milestone.IsValid())
		{
			VESTIGE_LOG(this, LogVestige, Warning, "The saved milestone {Milestone} no longer exists, so it is left out.", Name.ToString());
			++OutLeftOutCount;
			continue;
		}
		Reached.AddTag(Milestone);
	}
	return true;
}
