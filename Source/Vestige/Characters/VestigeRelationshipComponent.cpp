// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigeRelationshipComponent.h"

#include "Characters/VestigeDialogueLog.h"
#include "Characters/VestigeRelationshipSaveData.h"
#include "Content/VestigeCharacterData.h"
#include "Content/VestigeContentCatalog.h"
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
	FVestigeRelationshipSaveData Data;
	Data.Characters.Reserve(Met.Num());
	for (const FPrimaryAssetId& Character : Met)
	{
		FVestigeCharacterRelationshipSaveData& Entry = Data.Characters.AddDefaulted_GetRef();
		Entry.Character = Character.PrimaryAssetName;
		Entry.bMet = true;
	}

	// A set has no order; sorted, the same state always writes the same bytes.
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
	Met.Reset();
	Talks.Reset();
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
	}
	return true;
}

void UVestigeRelationshipComponent::StartNewGame()
{
	Met.Reset();
	Talks.Reset();
}
