// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Content/FarmCropData.h"

#include "Content/VestigeCalendar.h"
#include "Content/VestigeItemData.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "Content/VestigeGameData.h"
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "FarmCropData"

const FPrimaryAssetType UFarmCropData::AssetType(TEXT("FarmCrop"));

FFormatNamedArguments FFarmSeasonFit::GetReportValues(const FVestigeCalendar& Calendar, int32 Day) const
{
	FFormatNamedArguments Values = Calendar.GetReportValues(Day);
	Values.Add(TEXT("DaysToRipe"), DaysToRipe);

	const TArray<FVestigeSeason>& CalendarSeasons = Calendar.GetSeasons();
	switch (Fit)
	{
	case EFarmSeasonFit::Fits:
		break;
	case EFarmSeasonFit::OutOfSeason:
		if (CalendarSeasons.IsValidIndex(PlantSeasonIndex))
		{
			Values.Add(TEXT("PlantSeason"), CalendarSeasons[PlantSeasonIndex].DisplayName);
			Values.Add(TEXT("PlantSeasonInSentence"), CalendarSeasons[PlantSeasonIndex].NameInSentence);
		}
		break;
	case EFarmSeasonFit::TooLate:
		Values.Add(TEXT("DaysToDeadline"), DaysToDeadline);
		if (CalendarSeasons.IsValidIndex(DeadlineSeasonIndex))
		{
			Values.Add(TEXT("DeadlineSeason"), CalendarSeasons[DeadlineSeasonIndex].DisplayName);
			Values.Add(TEXT("DeadlineSeasonInSentence"), CalendarSeasons[DeadlineSeasonIndex].NameInSentence);
		}
		break;
	}
	return Values;
}

int32 UFarmCropData::GetGrowthStageIndex(int32 GrowthDays) const
{
	const int32 Days = FMath::Max(GrowthDays, 0);
	if (IsRipeAt(Days) || GrowthStages.IsEmpty())
	{
		return INDEX_NONE;
	}

	// The first stage also covers days before its StartDay, which only invalid data has.
	int32 StageIndex = 0;
	for (int32 Index = 1; Index < GrowthStages.Num() && GrowthStages[Index].StartDay <= Days; ++Index)
	{
		StageIndex = Index;
	}
	return StageIndex;
}

FFarmCropGrowthLook UFarmCropData::GetGrowthLook(int32 GrowthDays) const
{
	const int32 Days = FMath::Max(GrowthDays, 0);
	if (IsRipeAt(Days))
	{
		return {RipeMesh.Get(), 1.f};
	}

	const int32 StageIndex = GetGrowthStageIndex(Days);
	if (StageIndex == INDEX_NONE)
	{
		return {};
	}

	// The stage runs from its StartDay to the day before the next stage starts, the last one to the day before the crop
	// ripens. The clamps keep invalid start days from putting Days outside the span.
	const int32 NextStartDay = StageIndex + 1 < GrowthStages.Num() ? GrowthStages[StageIndex + 1].StartDay : DaysToMature;
	const int32 FirstDay = FMath::Min(GrowthStages[StageIndex].StartDay, Days);
	const int32 LastDay = FMath::Max(FMath::Min(NextStartDay - 1, DaysToMature - 1), Days);
	const float Scale = LastDay == FirstDay
		? 1.f
		: FMath::Lerp(StageStartScale, 1.f, static_cast<float>(Days - FirstDay) / static_cast<float>(LastDay - FirstDay));
	return {GrowthStages[StageIndex].Mesh.Get(), Scale};
}

bool UFarmCropData::GrowsIn(const FGameplayTag& Season, const FVestigeCalendar& Calendar) const
{
	return Seasons.HasTagExact(Season) || !ListsAnySeasonOf(Calendar);
}

FFarmSeasonFit UFarmCropData::GetSeasonFit(const FVestigeCalendar& Calendar, int32 Day) const
{
	FFarmSeasonFit Fit;
	Fit.DaysToRipe = DaysToMature;

	// Every crop fits in a calendar without seasons. A crop of every season (GrowsIn) finds no deadline below and fits too.
	const FVestigeDate Date = Calendar.GetDate(Day);
	if (!Date.HasSeason())
	{
		return Fit;
	}

	const TArray<FVestigeSeason>& CalendarSeasons = Calendar.GetSeasons();
	const int32 SeasonCount = CalendarSeasons.Num();
	if (!GrowsIn(Date.Season, Calendar))
	{
		Fit.Fit = EFarmSeasonFit::OutOfSeason;
		for (int32 Offset = 1; Offset < SeasonCount; ++Offset)
		{
			const int32 Index = (Date.SeasonIndex + Offset) % SeasonCount;
			if (GrowsIn(CalendarSeasons[Index].Tag, Calendar))
			{
				Fit.PlantSeasonIndex = Index;
				break;
			}
		}
		return Fit;
	}

	int32 DaysToDeadline = Date.DaysLeft;
	for (int32 Offset = 1; Offset < SeasonCount; ++Offset)
	{
		const int32 Index = (Date.SeasonIndex + Offset) % SeasonCount;
		if (!GrowsIn(CalendarSeasons[Index].Tag, Calendar))
		{
			Fit.DeadlineSeasonIndex = Index;
			break;
		}
		DaysToDeadline += Calendar.GetDaysPerSeason();
	}

	// No coming season ends the crop's growth: it grows in every season of the calendar.
	if (Fit.DeadlineSeasonIndex == INDEX_NONE)
	{
		return Fit;
	}
	Fit.DaysToDeadline = DaysToDeadline;
	if (Fit.DaysToRipe > DaysToDeadline)
	{
		Fit.Fit = EFarmSeasonFit::TooLate;
	}
	return Fit;
}

bool UFarmCropData::ListsAnySeasonOf(const FVestigeCalendar& Calendar) const
{
	return Calendar.GetSeasons().ContainsByPredicate(
		[this](const FVestigeSeason& Season)
		{
			return Seasons.HasTagExact(Season.Tag);
		});
}

#if WITH_EDITOR
EDataValidationResult UFarmCropData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto ReportError = [&Context, &Result](const FText& Error)
	{
		Context.AddError(Error);
		Result = EDataValidationResult::Invalid;
	};

	if (const TOptional<FText> Error = FindContentReferenceError(HarvestItemId, UVestigeItemData::AssetType))
	{
		ReportError(FText::Format(LOCTEXT("BadHarvestItemId", "Harvest Item Id {0}"), *Error));
	}
	if (Seasons.IsEmpty())
	{
		ReportError(LOCTEXT("NoSeasons", "Seasons is empty, so the crop grows in every season. List every season for that."));
	}

	// A calendar without seasons means the game data didn't load, which the asset manager already reports.
	const FVestigeCalendar& Calendar = UVestigeGameData::Get().GetCalendar();
	if (!Calendar.GetSeasons().IsEmpty())
	{
		for (const FGameplayTag& Season : Seasons)
		{
			if (!Calendar.FindSeason(Season))
			{
				ReportError(
					FText::Format(LOCTEXT("SeasonNotInCalendar", "Seasons lists {0}, which isn't a season of the calendar in DA_GameData."),
						FText::FromName(Season.GetTagName())));
			}
		}
	}
	if (DaysToMature < 1)
	{
		ReportError(FText::Format(LOCTEXT("BadDaysToMature", "Days To Mature is {0}. It must be at least 1."), DaysToMature));
	}
	if (StageStartScale <= 0.f || StageStartScale > 1.f)
	{
		ReportError(
			FText::Format(LOCTEXT("BadStageStartScale", "Stage Start Scale is {0}. It must be above 0 and at most 1."), StageStartScale));
	}
	if (RipeMesh.IsNull())
	{
		ReportError(LOCTEXT("NoRipeMesh", "Ripe Mesh is empty, so the ripe crop draws the placeholder. Pick the crop's ripe mesh."));
	}
	if (GrowthStages.IsEmpty())
	{
		ReportError(LOCTEXT(
			"NoGrowthStages", "Growth Stages is empty, so the growing crop draws the placeholder. Add the crop's stages, seedling first."));
	}
	for (int32 Index = 0; Index < GrowthStages.Num(); ++Index)
	{
		const FFarmCropGrowthStage& Stage = GrowthStages[Index];
		if (Stage.Mesh.IsNull())
		{
			ReportError(
				FText::Format(LOCTEXT("NoGrowthStageMesh", "Growth stage {0} has no mesh. Pick its mesh or remove the stage."), Index));
		}
		else if (Stage.Mesh == RipeMesh)
		{
			ReportError(FText::Format(
				LOCTEXT("RipeMeshAmongStages", "Growth stage {0} uses the Ripe Mesh, so it shows before the crop is ripe."), Index));
		}
		if (Index == 0 && Stage.StartDay != 0)
		{
			ReportError(
				FText::Format(LOCTEXT("FirstStageNotPlanted", "Growth stage 0 starts on day {0}. It must start on day 0, the day planted."),
					Stage.StartDay));
		}
		if (Index > 0 && Stage.StartDay <= GrowthStages[Index - 1].StartDay)
		{
			ReportError(FText::Format(
				LOCTEXT("StagesNotIncreasing", "Growth stage {0} starts on day {1}. It must start later than the stage before it."), Index,
				Stage.StartDay));
		}
		if (Stage.StartDay >= DaysToMature)
		{
			ReportError(FText::Format(
				LOCTEXT("StageAfterRipe", "Growth stage {0} starts on day {1}, but the crop is ripe from day {2}, so it never shows."),
				Index, Stage.StartDay, DaysToMature));
		}
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
