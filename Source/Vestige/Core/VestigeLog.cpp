// Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

#include "Core/VestigeLog.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogVestige);
DEFINE_LOG_CATEGORY(LogVestigeFeedback);

FString VestigeLog::GetSenderName(const UObject* Sender)
{
	if (!Sender)
	{
		return TEXT("None");
	}
	if (const UActorComponent* Component = Cast<UActorComponent>(Sender))
	{
		return FString::Printf(TEXT("%s.%s"), *GetNameSafe(Component->GetOwner()), *Component->GetName());
	}
	return Sender->GetName();
}
