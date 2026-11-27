// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Characters/VestigeWateringHelpComponent.h"

#include "Characters/VestigeDialogueLog.h"
#include "Characters/VestigeNpcCharacter.h"
#include "Characters/VestigeRelationshipComponent.h"
#include "Content/VestigeCharacterData.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldSubsystem.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmLog.h"
#include "Farming/FarmTileState.h"
#include "Farming/FarmWaterSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

UVestigeWateringHelpComponent::UVestigeWateringHelpComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVestigeWateringHelpComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Cast<AVestigeNpcCharacter>(GetOwner()))
	{
		VESTIGE_LOG(this, LogVestigeDialogue, Warning, "The watering help is on {Owner}, which isn't a character body, so it does nothing.",
			GetNameSafe(GetOwner()));
		return;
	}
	if (UFarmFieldSubsystem* FieldSubsystem = UFarmFieldSubsystem::Get(GetWorld()))
	{
		FieldSubsystem->OnFieldsAdvanced.AddUObject(this, &UVestigeWateringHelpComponent::WaterAtDawn);
		Fields = FieldSubsystem;
	}
}

void UVestigeWateringHelpComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFarmFieldSubsystem* FieldSubsystem = Fields.Get())
	{
		FieldSubsystem->OnFieldsAdvanced.RemoveAll(this);
	}
	Fields.Reset();
	Super::EndPlay(EndPlayReason);
}

void UVestigeWateringHelpComponent::WaterAtDawn(int32 Day)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UVestigeWateringHelpComponent::WaterAtDawn);

	const AVestigeNpcCharacter* Body = Cast<AVestigeNpcCharacter>(GetOwner());
	const UVestigeCharacterData* Character = Body ? Body->GetCharacterData() : nullptr;
	const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	UVestigeRelationshipComponent* Relationships =
		PlayerController ? UVestigeRelationshipComponent::Find(PlayerController->PlayerState) : nullptr;
	if (!Character || !Relationships || !Relationships->HasFlag(Body->GetCharacterId(), RequiredFlag) || MaxTiles < 1)
	{
		return;
	}

	if (!Grid.IsValid())
	{
		Grid = AFarmGrid::Find(GetWorld());
	}
	UFarmField* Field = Grid.IsValid() ? Grid->GetField() : nullptr;
	UFarmWaterSubsystem* Water = UFarmWaterSubsystem::Get(GetWorld());
	if (!Field || !Water)
	{
		return;
	}

	// The dry, growing, unripe tiles, nearest the body first, then by row and column so the order repeats.
	const FIntPoint Here = Grid->GetCoordinatesAt(Body->GetActorLocation());
	TArray<FIntPoint> Dry;
	for (int32 Y = 0; Y < Field->GetSize().Y; ++Y)
	{
		for (int32 X = 0; X < Field->GetSize().X; ++X)
		{
			const FFarmTileState* Tile = Field->GetTile(FIntPoint(X, Y));
			if (Tile && Tile->GetState() == EFarmTileState::Planted && !Tile->IsRipe() && Tile->GetWaterLevel() == 0)
			{
				Dry.Add(FIntPoint(X, Y));
			}
		}
	}
	Dry.Sort(
		[&Here](const FIntPoint& A, const FIntPoint& B)
		{
			const int32 DistanceA = (A - Here).SizeSquared();
			const int32 DistanceB = (B - Here).SizeSquared();
			if (DistanceA != DistanceB)
			{
				return DistanceA < DistanceB;
			}
			return A.Y != B.Y ? A.Y < B.Y : A.X < B.X;
		});

	// The same step as the player's can: the tile's change event darkens the soil.
	const int32 Drawn = Water->DrawCleanWater(FMath::Min(MaxTiles, Dry.Num()));
	for (int32 Index = 0; Index < Drawn; ++Index)
	{
		Field->Water(Dry[Index]);
	}
	VESTIGE_LOG(this, LogFarm, Verbose, "Watered {Count} of {Dry} dry tiles at dawn on day {Day}.", Drawn, Dry.Num(), Day);
	if (Drawn > 0)
	{
		Relationships->SetFlag(*Character, HelpedFlag, Day);
	}
}
