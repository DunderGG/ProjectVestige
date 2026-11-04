// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmFieldSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Farming/FarmField.h"
#include "Farming/FarmFieldSaveData.h"
#include "Farming/FarmLog.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Time/VestigeClockSubsystem.h"

UFarmFieldSubsystem* UFarmFieldSubsystem::Get(const UWorld* World)
{
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFarmFieldSubsystem>() : nullptr;
}

void UFarmFieldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Clock = Collection.InitializeDependency<UVestigeClockSubsystem>();
	if (Clock)
	{
		Clock->OnNewDay.AddUObject(this, &UFarmFieldSubsystem::AdvanceFields);
	}
}

void UFarmFieldSubsystem::Deinitialize()
{
	if (Clock)
	{
		Clock->OnNewDay.RemoveAll(this);
	}
	Super::Deinitialize();
}

UFarmField& UFarmFieldSubsystem::FindOrAddField(FName FieldId, const FIntPoint& Size)
{
	UFarmField* Field = FindField(FieldId);
	if (!Field)
	{
		return AddField(FieldId, Size);
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
	Field->Initialize(FieldId, Size);
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
}

void UFarmFieldSubsystem::AdvanceFields(int32 NewDay)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmFieldSubsystem::AdvanceFields);

	for (const TPair<FName, TObjectPtr<UFarmField>>& Pair : Fields)
	{
		Pair.Value->AdvanceDay();
	}
}
