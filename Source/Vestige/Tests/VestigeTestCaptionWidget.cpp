// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Tests/VestigeTestCaptionWidget.h"

#if WITH_TESTS

#include "Components/TextBlock.h"
#include "Layout/Geometry.h"
#include "UObject/UnrealType.h"

void UVestigeTestCaptionWidget::MakeParts()
{
	// The bound properties, set as the designer's binding would; the blocks are not in a widget tree, so a name search misses them.
	for (const TCHAR* PartName : {TEXT("SpeakerNameText"), TEXT("LineText")})
	{
		FObjectProperty* Property = FindFProperty<FObjectProperty>(UVestigeCaptionWidget::StaticClass(), PartName);
		check(Property);
		Property->SetObjectPropertyValue_InContainer(this, NewObject<UTextBlock>(this));
	}
}

void UVestigeTestCaptionWidget::TickForTests(float DeltaSeconds)
{
	NativeTick(FGeometry(), DeltaSeconds);
}

#endif
