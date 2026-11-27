// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigeRelationshipComponent.h"

#include "Characters/VestigeDialogueLog.h"
#include "Characters/VestigeRelationshipSaveData.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
#include "Core/VestigeMilestoneSubsystem.h"
#include "Core/VestigePlayerComponents.h"
#include "Core/VestigeSaveRegistry.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

UVestigeRelationshipComponent::UVestigeRelationshipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UVestigeRelationshipComponent* UVestigeRelationshipComponent::Find(const AActor* PlayerActor)
{
	return VestigePlayerComponents::Find<UVestigeRelationshipComponent>(PlayerActor);
}

void UVestigeRelationshipComponent::BeginPlay()
{
	Super::BeginPlay();

	UVestigeSaveRegistry::RegisterWorldOwner(GetWorld(), *this);
}

void UVestigeRelationshipComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UVestigeSaveRegistry::UnregisterWorldOwner(GetWorld(), *this);
	Super::EndPlay(EndPlayReason);
}

bool UVestigeRelationshipComponent::HasMet(const FPrimaryAssetId& Character) const
{
	return Met.Contains(Character);
}

int32 UVestigeRelationshipComponent::GetTalksToday(const FPrimaryAssetId& Character, int32 Day) const
{
	const FTalksOnDay* Found = Talks.Find(Character);
	return Found && Found->Day == Day ? Found->Count : 0;
}

void UVestigeRelationshipComponent::RecordTalk(const FPrimaryAssetId& Character, int32 Day)
{
	Met.Add(Character);

	FTalksOnDay& Today = Talks.FindOrAdd(Character);
	if (Today.Day != Day)
	{
		Today = FTalksOnDay{Day, 0};
	}
	++Today.Count;
}

void UVestigeRelationshipComponent::AddTrust(const UVestigeCharacterData& Character, int32 Amount)
{
	if (Amount < 1)
	{
		return;
	}
	EarnedTrust.FindOrAdd(Character.GetPrimaryAssetId()) += Amount;
	VESTIGE_LOG(this, LogVestigeDialogue, Log, "Trust with {Character} is now {Trust} (+{Amount}).", Character.GetContentId().ToString(),
		GetTrust(Character), Amount);
}

int32 UVestigeRelationshipComponent::GetEarnedTrust(const FPrimaryAssetId& Character) const
{
	const int32* Earned = EarnedTrust.Find(Character);
	return Earned ? *Earned : 0;
}

int32 UVestigeRelationshipComponent::GetTrust(const UVestigeCharacterData& Character) const
{
	int32 Trust = GetEarnedTrust(Character.GetPrimaryAssetId());
	if (const UVestigeMilestoneSubsystem* Milestones = UVestigeMilestoneSubsystem::Get(GetWorld()))
	{
		for (const TPair<FGameplayTag, int32>& Entry : Character.GetTrustFromMilestones())
		{
			if (Milestones->HasReached(Entry.Key))
			{
				Trust += Entry.Value;
			}
		}
	}
	return Trust;
}

void UVestigeRelationshipComponent::SetFlag(const UVestigeCharacterData& Character, const FGameplayTag& Flag, int32 Day)
{
	if (!Flag.IsValid())
	{
		return;
	}
	Flags.FindOrAdd(Character.GetPrimaryAssetId()).Add(Flag, Day);
	VESTIGE_LOG(this, LogVestigeDialogue, Log, "{Character} flag {Flag} set on day {Day}.", Character.GetContentId().ToString(),
		Flag.ToString(), Day);
}

bool UVestigeRelationshipComponent::HasFlag(const FPrimaryAssetId& Character, const FGameplayTag& Flag) const
{
	return GetFlagDays(Character).Contains(Flag);
}

const TMap<FGameplayTag, int32>& UVestigeRelationshipComponent::GetFlagDays(const FPrimaryAssetId& Character) const
{
	static const TMap<FGameplayTag, int32> NoFlags;
	const TMap<FGameplayTag, int32>* Found = Flags.Find(Character);
	return Found ? *Found : NoFlags;
}

FName UVestigeRelationshipComponent::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Relationships"));
	return Key;
}

int32 UVestigeRelationshipComponent::GetSaveVersion() const
{
	return FVestigeRelationshipSaveData::Version;
}

FVestigeSaveRecord UVestigeRelationshipComponent::WriteSaveRecord() const
{
	// A character is written when met, with trust earned or with a flag set.
	TSet<FPrimaryAssetId> Written(Met);
	for (const TPair<FPrimaryAssetId, int32>& Entry : EarnedTrust)
	{
		if (Entry.Value > 0)
		{
			Written.Add(Entry.Key);
		}
	}
	for (const TPair<FPrimaryAssetId, TMap<FGameplayTag, int32>>& Entry : Flags)
	{
		if (!Entry.Value.IsEmpty())
		{
			Written.Add(Entry.Key);
		}
	}

	FVestigeRelationshipSaveData Data;
	Data.Characters.Reserve(Written.Num());
	for (const FPrimaryAssetId& Character : Written)
	{
		FVestigeCharacterRelationshipSaveData& Entry = Data.Characters.AddDefaulted_GetRef();
		Entry.Character = Character.PrimaryAssetName;
		Entry.bMet = Met.Contains(Character);
		Entry.EarnedTrust = GetEarnedTrust(Character);
		for (const TPair<FGameplayTag, int32>& Flag : GetFlagDays(Character))
		{
			Entry.Flags.Add({Flag.Key.GetTagName(), Flag.Value});
		}
		Entry.Flags.Sort(
			[](const FVestigeRelationshipFlagSaveData& A, const FVestigeRelationshipFlagSaveData& B)
			{
				return A.Flag.LexicalLess(B.Flag);
			});
	}

	// Sets and maps have no order; sorted, the same state always writes the same bytes.
	Data.Characters.Sort(
		[](const FVestigeCharacterRelationshipSaveData& A, const FVestigeCharacterRelationshipSaveData& B)
		{
			return A.Character.LexicalLess(B.Character);
		});
	return FVestigeSaveRecord::Write(Data);
}

bool UVestigeRelationshipComponent::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	FVestigeRelationshipSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}

	// Today's talks aren't saved, so a load forgets them: the day's greeting plays again.
	Clear();
	const UVestigeContentCatalog* Catalog = UVestigeContentCatalog::Get(GetWorld());
	for (const FVestigeCharacterRelationshipSaveData& Entry : Data.Characters)
	{
		const FPrimaryAssetId Character(UVestigeCharacterData::AssetType, Entry.Character);
		if (!Catalog || !Catalog->Find<UVestigeCharacterData>(Character))
		{
			VESTIGE_LOG(this, LogVestigeDialogue, Warning, "The saved character {Character} no longer exists, so it is left out.",
				Entry.Character.ToString());
			++OutLeftOutCount;
			continue;
		}
		if (Entry.bMet)
		{
			Met.Add(Character);
		}
		if (Entry.EarnedTrust > 0)
		{
			EarnedTrust.Add(Character, Entry.EarnedTrust);
		}
		for (const FVestigeRelationshipFlagSaveData& SavedFlag : Entry.Flags)
		{
			const FGameplayTag Flag = FGameplayTag::RequestGameplayTag(SavedFlag.Flag, false);
			if (!Flag.IsValid())
			{
				VESTIGE_LOG(this, LogVestigeDialogue, Warning, "The saved flag {Flag} of {Character} no longer exists, so it is left out.",
					SavedFlag.Flag.ToString(), Entry.Character.ToString());
				++OutLeftOutCount;
				continue;
			}
			Flags.FindOrAdd(Character).Add(Flag, SavedFlag.Day);
		}
	}
	return true;
}

void UVestigeRelationshipComponent::StartNewGame()
{
	Clear();
}

void UVestigeRelationshipComponent::Clear()
{
	Met.Reset();
	Talks.Reset();
	EarnedTrust.Reset();
	Flags.Reset();
}
