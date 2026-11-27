// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmFieldSubsystem.h"

#include "AbilitySystem/VestigeGameplayTags.h"
#include "Content/FarmCropData.h"
#include "Content/VestigeContentCatalog.h"
#include "Core/VestigeFeedbackSubsystem.h"
#include "Core/VestigeSaveRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldSaveData.h"
#include "Farming/FarmFieldSettings.h"
#include "Farming/FarmLog.h"
#include "Misc/Guid.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Time/VestigeClockSubsystem.h"

namespace FarmFieldSaves
{
	/** Finds in Catalog each crop the saved fields' tiles name, once. Crops that no longer exist are left out. */
	TMap<FName, const UFarmCropData*> FindSavedCrops(const UVestigeContentCatalog* Catalog, const TArray<FFarmFieldSaveData>& SavedFields)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FarmFieldSaves::FindSavedCrops);

		TMap<FName, const UFarmCropData*> Crops;
		TSet<FName> Missing;
		for (const FFarmFieldSaveData& Field : SavedFields)
		{
			for (const FFarmTileSaveData& Tile : Field.Tiles)
			{
				if (Tile.CropId.IsNone() || Crops.Contains(Tile.CropId) || Missing.Contains(Tile.CropId))
				{
					continue;
				}

				const FPrimaryAssetId CropAssetId(UFarmCropData::AssetType, Tile.CropId);
				if (const UFarmCropData* Crop = Catalog ? Catalog->Find<UFarmCropData>(CropAssetId) : nullptr)
				{
					Crops.Add(Tile.CropId, Crop);
				}
				else
				{
					Missing.Add(Tile.CropId);
				}
			}
		}
		return Crops;
	}
}

UFarmFieldSubsystem* UFarmFieldSubsystem::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFarmFieldSubsystem>() : nullptr;
}

void UFarmFieldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MutationSeed = PickMutationSeed();

	// Restoring a save finds its crops in the catalogue.
	Catalog = Collection.InitializeDependency<UVestigeContentCatalog>();
	Clock = Collection.InitializeDependency<UVestigeClockSubsystem>();
	if (Clock)
	{
		Clock->OnNewDay.AddUObject(this, &UFarmFieldSubsystem::AdvanceFields);
		Clock->OnSeasonStarted.AddUObject(this, &UFarmFieldSubsystem::WitherOutOfSeasonCrops);
	}
	SaveRegistry = Collection.InitializeDependency<UVestigeSaveRegistry>();
	if (SaveRegistry)
	{
		SaveRegistry->RegisterGameOwner(*this);
	}
}

void UFarmFieldSubsystem::Deinitialize()
{
	if (Clock)
	{
		Clock->OnNewDay.RemoveAll(this);
		Clock->OnSeasonStarted.RemoveAll(this);
	}
	if (SaveRegistry)
	{
		SaveRegistry->UnregisterGameOwner(*this);
	}
	Super::Deinitialize();
}

UFarmField& UFarmFieldSubsystem::FindOrAddField(
	FName FieldId, const FIntPoint& Size, TConstArrayView<FFarmContaminationArea> StartingContamination)
{
	UFarmField* Field = FindField(FieldId);
	if (!Field)
	{
		// Only a field made now starts contaminated: a found one, restored or visited before, may have been cleaned.
		UFarmField& NewField = AddField(FieldId, Size);
		NewField.ApplyContaminationAreas(StartingContamination);
		return NewField;
	}

	if (Field->GetSize() != Size)
	{
		VESTIGE_LOG(this, LogFarm, Warning, "Field {FieldId} is {OldSize} tiles but its grid is {NewSize}, so it was resized.", FieldId,
			Field->GetSize().ToString(), Size.ToString());
		Field->Resize(Size);
	}
	return *Field;
}

UFarmField* UFarmFieldSubsystem::FindField(FName FieldId) const
{
	const TObjectPtr<UFarmField>* Field = Fields.Find(FieldId);
	return Field ? Field->Get() : nullptr;
}

UFarmField& UFarmFieldSubsystem::AddField(FName FieldId, const FIntPoint& Size)
{
	UFarmField* Field = NewObject<UFarmField>(this);
	const UFarmFieldSettings& Settings = UFarmFieldSettings::Get();
	Field->Initialize(FieldId, Size, Settings.GetWaterRules(), Settings.GetContaminationRules());
	Fields.Add(FieldId, Field);
	return *Field;
}

TArray<FFarmFieldSaveData> UFarmFieldSubsystem::GetSaveData() const
{
	TArray<FFarmFieldSaveData> SavedFields;
	SavedFields.Reserve(Fields.Num());
	for (const TPair<FName, TObjectPtr<UFarmField>>& Pair : Fields)
	{
		FFarmFieldSaveData& Data = SavedFields.AddDefaulted_GetRef();
		Data.FieldId = Pair.Key;
		Data.Size = Pair.Value->GetSize();
		Data.Tiles = Pair.Value->GetTilesSaveData();
	}
	return SavedFields;
}

int32 UFarmFieldSubsystem::RestoreFromSaveData(
	const TArray<FFarmFieldSaveData>& SavedFields, const TMap<FName, const UFarmCropData*>& Crops)
{
	int32 SkippedCount = 0;
	for (const FFarmFieldSaveData& Data : SavedFields)
	{
		UFarmField* Field = FindField(Data.FieldId);
		if (!Field)
		{
			Field = &AddField(Data.FieldId, Data.Size);
		}
		SkippedCount += Field->RestoreTilesFromSaveData(Data.Tiles, Crops);
	}
	return SkippedCount;
}

void UFarmFieldSubsystem::StartNewGame()
{
	Fields.Reset();
	MutationSeed = PickMutationSeed();
}

int32 UFarmFieldSubsystem::PickMutationSeed()
{
	return static_cast<int32>(GetTypeHash(FGuid::NewGuid()));
}

FName UFarmFieldSubsystem::GetSaveKey() const
{
	static const FName Key(TEXT("Base.Fields"));
	return Key;
}

int32 UFarmFieldSubsystem::GetSaveVersion() const
{
	return FFarmFieldsSaveData::Version;
}

FVestigeSaveRecord UFarmFieldSubsystem::WriteSaveRecord() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmFieldSubsystem::WriteSaveRecord);

	FFarmFieldsSaveData Data;
	Data.Fields = GetSaveData();
	Data.MutationSeed = MutationSeed;
	return FVestigeSaveRecord::Write(Data);
}

bool UFarmFieldSubsystem::ReadSaveRecord(const FVestigeSaveRecord& Record, int32& OutLeftOutCount)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmFieldSubsystem::ReadSaveRecord);

	FFarmFieldsSaveData Data;
	if (!Record.Read(Data))
	{
		return false;
	}

	// The save holds every field, so fields it doesn't name belong to another game.
	StartNewGame();

	// After StartNewGame, which picks a fresh seed: the saved one must win.
	MutationSeed = Data.MutationSeed;
	OutLeftOutCount += RestoreFromSaveData(Data.Fields, FarmFieldSaves::FindSavedCrops(Catalog, Data.Fields));
	return true;
}

void UFarmFieldSubsystem::AdvanceFields(int32 NewDay)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmFieldSubsystem::AdvanceFields);

	for (const TPair<FName, TObjectPtr<UFarmField>>& Pair : Fields)
	{
		Pair.Value->AdvanceDay(NewDay, MutationSeed);
	}

	OnFieldsAdvanced.Broadcast(NewDay);
}

void UFarmFieldSubsystem::WitherOutOfSeasonCrops(const FVestigeDate& Date)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmFieldSubsystem::WitherOutOfSeasonCrops);

	if (!Clock)
	{
		return;
	}
	int32 WitheredCount = 0;
	for (const TPair<FName, TObjectPtr<UFarmField>>& Pair : Fields)
	{
		WitheredCount += Pair.Value->WitherCropsOutOfSeason(Date.Season, Clock->GetCalendar());
	}
	if (WitheredCount == 0)
	{
		return;
	}

	FFormatNamedArguments Values = Clock->GetCalendar().GetReportValues(Date.Day);
	Values.Add(TEXT("Count"), WitheredCount);
	UVestigeFeedbackSubsystem::Report(GetGameInstance()->GetWorld(), VestigeGameplayTags::Feedback_Farm_Withered, MoveTemp(Values));
}
