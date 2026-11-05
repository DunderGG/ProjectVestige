// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Farming/FarmFieldMeshesComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Farming/FarmField.h"
#include "Farming/FarmGrid.h"
#include "Farming/FarmTileState.h"
#include "Materials/MaterialInterface.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UObject/ConstructorHelpers.h"

namespace FarmFieldMeshes
{
	/** A part's color as the per-instance custom data the placeholder material reads. */
	TArray<float, TFixedAllocator<FarmTileLooks::NumCustomDataFloats>> ToCustomData(const FLinearColor& Color)
	{
		return {Color.R, Color.G, Color.B};
	}
}

UFarmFieldMeshesComponent::UFarmFieldMeshesComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Engine placeholders, which ConstructorHelpers may load. The placeholder material is our own asset, so BP_FarmGrid sets it.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Looks.PlaceholderSoilMesh = CubeMesh.Object;
	Looks.PlaceholderCropMesh = CylinderMesh.Object;
}

void UFarmFieldMeshesComponent::ShowField(UFarmField& InField, float InTileSize)
{
	StopShowing();
	Field = &InField;
	TileSize = InTileSize;
	InField.OnTileChanged.AddUObject(this, &UFarmFieldMeshesComponent::RedrawTile);
	InField.OnTilesChanged.AddUObject(this, &UFarmFieldMeshesComponent::RedrawField);
	RedrawField();
}

void UFarmFieldMeshesComponent::StopShowing()
{
	if (UFarmField* CurrentField = Field.Get())
	{
		CurrentField->OnTileChanged.RemoveAll(this);
		CurrentField->OnTilesChanged.RemoveAll(this);
	}
	Field.Reset();
	for (UInstancedStaticMeshComponent* Set : Sets)
	{
		Set->ClearInstances();
	}
	SoilInstances.Reset();
	CropInstances.Reset();
	FieldWidth = 0;
}

void UFarmFieldMeshesComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopShowing();
	Super::EndPlay(EndPlayReason);
}

FTransform UFarmFieldMeshesComponent::GetTileTransform(const FIntPoint& Coordinates) const
{
	const FVector Center = AFarmGrid::GetLocalTileCenter(Coordinates, TileSize);
	return FTransform(FQuat::Identity, Center, FVector(TileSize / AFarmGrid::UnscaledTileSize));
}

int32 UFarmFieldMeshesComponent::FindOrAddSet(UStaticMesh& Mesh, UMaterialInterface* Material)
{
	const TPair<TObjectKey<UStaticMesh>, TObjectKey<UMaterialInterface>> Key(&Mesh, Material);
	if (const int32* Found = SetIndices.Find(Key))
	{
		return *Found;
	}

	const FName SetName =
		MakeUniqueObjectName(GetOwner(), UInstancedStaticMeshComponent::StaticClass(), *FString::Printf(TEXT("Tiles_%s"), *Mesh.GetName()));
	UInstancedStaticMeshComponent* Set = NewObject<UInstancedStaticMeshComponent>(GetOwner(), SetName, RF_Transient);
	Set->SetupAttachment(this);
	Set->SetStaticMesh(&Mesh);
	if (Material)
	{
		for (int32 Slot = 0; Slot < Set->GetNumMaterials(); ++Slot)
		{
			Set->SetMaterial(Slot, Material);
		}
	}
	// Before any instance is added: custom data set earlier is dropped.
	Set->SetNumCustomDataFloats(FarmTileLooks::NumCustomDataFloats);
	Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Set->SetGenerateOverlapEvents(false);
	Set->SetCanEverAffectNavigation(false);
	Set->RegisterComponent();

	const int32 SetIndex = Sets.Add(Set);
	SetIndices.Add(Key, SetIndex);
	return SetIndex;
}

void UFarmFieldMeshesComponent::RedrawField()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFarmFieldMeshesComponent::RedrawField);

	for (UInstancedStaticMeshComponent* Set : Sets)
	{
		Set->ClearInstances();
	}
	const UFarmField* CurrentField = Field.Get();
	const FIntPoint Size = CurrentField ? CurrentField->GetSize() : FIntPoint::ZeroValue;
	FieldWidth = Size.X;
	SoilInstances.Init(FTileInstance(), Size.X * Size.Y);
	CropInstances.Init(FTileInstance(), Size.X * Size.Y);
	if (!CurrentField)
	{
		return;
	}

	// Queued per set first, so each set takes all its instances in one call.
	TArray<FPendingSet> Pending;
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			const FIntPoint Coordinates(X, Y);
			const FFarmTileLook Look = FarmTileLooks::GetLook(*CurrentField->GetTile(Coordinates), Looks);
			const FTransform TileTransform = GetTileTransform(Coordinates);
			const int32 Index = Y * Size.X + X;
			QueuePart(Pending, SoilInstances[Index], Look.Soil, TileTransform);
			if (Look.Crop)
			{
				QueuePart(Pending, CropInstances[Index], *Look.Crop, TileTransform);
			}
		}
	}

	for (int32 SetIndex = 0; SetIndex < Pending.Num(); ++SetIndex)
	{
		FPendingSet& SetBatch = Pending[SetIndex];
		if (SetBatch.Transforms.IsEmpty())
		{
			continue;
		}
		UInstancedStaticMeshComponent* Set = Sets[SetIndex];
		const TArray<FPrimitiveInstanceId> Ids = Set->AddInstancesById(SetBatch.Transforms, false, false);
		Set->SetCustomDataById(Ids, SetBatch.CustomData);
		for (int32 Entry = 0; Entry < Ids.Num(); ++Entry)
		{
			SetBatch.Instances[Entry]->Id = Ids[Entry];
		}
	}
}

void UFarmFieldMeshesComponent::QueuePart(
	TArray<FPendingSet>& Pending, FTileInstance& Instance, const FFarmTilePartLook& Look, const FTransform& TileTransform)
{
	if (!Look.Mesh)
	{
		return;
	}
	Instance.SetIndex = FindOrAddSet(*Look.Mesh, Look.Material);
	if (Pending.Num() <= Instance.SetIndex)
	{
		Pending.SetNum(Instance.SetIndex + 1);
	}
	FPendingSet& SetBatch = Pending[Instance.SetIndex];
	SetBatch.Transforms.Add(Look.Transform * TileTransform);
	SetBatch.CustomData.Append(FarmFieldMeshes::ToCustomData(Look.Color));
	SetBatch.Instances.Add(&Instance);
}

void UFarmFieldMeshesComponent::RedrawTile(const FIntPoint& Coordinates)
{
	const UFarmField* CurrentField = Field.Get();
	const FFarmTileState* Tile = CurrentField ? CurrentField->GetTile(Coordinates) : nullptr;
	const int32 Index = Coordinates.Y * FieldWidth + Coordinates.X;
	if (!Tile || !SoilInstances.IsValidIndex(Index))
	{
		return;
	}

	const FFarmTileLook Look = FarmTileLooks::GetLook(*Tile, Looks);
	const FTransform TileTransform = GetTileTransform(Coordinates);
	ShowPart(SoilInstances[Index], &Look.Soil, TileTransform);
	ShowPart(CropInstances[Index], Look.Crop.GetPtrOrNull(), TileTransform);
}

void UFarmFieldMeshesComponent::ShowPart(FTileInstance& Instance, const FFarmTilePartLook* Look, const FTransform& TileTransform)
{
	if (!Look || !Look->Mesh)
	{
		HidePart(Instance);
		return;
	}

	const int32 SetIndex = FindOrAddSet(*Look->Mesh, Look->Material);
	const FTransform InstanceTransform = Look->Transform * TileTransform;
	const auto CustomData = FarmFieldMeshes::ToCustomData(Look->Color);
	if (SetIndex != Instance.SetIndex)
	{
		HidePart(Instance);
		Instance.SetIndex = SetIndex;
		Instance.Id = Sets[SetIndex]->AddInstanceById(InstanceTransform, false);
	}
	else
	{
		Sets[SetIndex]->UpdateInstanceTransformById(Instance.Id, InstanceTransform, false, true);
	}
	Sets[SetIndex]->SetCustomDataById(MakeArrayView(&Instance.Id, 1), CustomData);
}

void UFarmFieldMeshesComponent::HidePart(FTileInstance& Instance)
{
	if (Sets.IsValidIndex(Instance.SetIndex))
	{
		Sets[Instance.SetIndex]->RemoveInstanceById(Instance.Id);
	}
	Instance = FTileInstance();
}

int32 UFarmFieldMeshesComponent::GetInstanceCount(const UStaticMesh* Mesh, const UMaterialInterface* Material) const
{
	const TPair<TObjectKey<UStaticMesh>, TObjectKey<UMaterialInterface>> Key(Mesh, Material);
	const int32* SetIndex = SetIndices.Find(Key);
	return SetIndex ? Sets[*SetIndex]->GetInstanceCount() : 0;
}

int32 UFarmFieldMeshesComponent::GetTotalInstanceCount() const
{
	int32 Count = 0;
	for (const UInstancedStaticMeshComponent* Set : Sets)
	{
		Count += Set->GetInstanceCount();
	}
	return Count;
}

TArray<float> UFarmFieldMeshesComponent::GetSoilCustomData(const FIntPoint& Coordinates) const
{
	const int32 Index = Coordinates.Y * FieldWidth + Coordinates.X;
	const bool bInField = Coordinates.X >= 0 && Coordinates.X < FieldWidth && SoilInstances.IsValidIndex(Index);
	if (!bInField || !Sets.IsValidIndex(SoilInstances[Index].SetIndex))
	{
		return {};
	}

	const FTileInstance& Instance = SoilInstances[Index];
	const UInstancedStaticMeshComponent* Set = Sets[Instance.SetIndex];
	const int32 InstanceIndex = Set->GetInstanceIndexForId(Instance.Id);
	const int32 First = InstanceIndex * Set->NumCustomDataFloats;
	return TArray<float>(Set->PerInstanceSMCustomData.GetData() + First, Set->NumCustomDataFloats);
}
