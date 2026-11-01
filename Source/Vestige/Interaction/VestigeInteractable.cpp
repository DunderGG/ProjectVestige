// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Interaction/VestigeInteractable.h"

bool FVestigeInteractionOption::IsSameAs(const FVestigeInteractionOption& Other) const
{
	// Compares the shown strings rather than the texts' identity, because an interactable builds a new FText for each scan.
	return Action == Other.Action && Text.ToString().Equals(Other.Text.ToString(), ESearchCase::CaseSensitive);
}
