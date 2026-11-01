// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/VestigeToolData.h"

#include "Engine/StaticMesh.h"

const FPrimaryAssetType UVestigeToolData::AssetType(TEXT("FarmTool"));

UStaticMesh* UVestigeToolData::GetHeldMesh() const
{
	return HeldMesh.Get();
}
