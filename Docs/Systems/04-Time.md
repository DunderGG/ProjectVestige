# 4. Time, days and seasons

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** day and clock built (`UVestigeClockSubsystem`, a game-instance subsystem, with `UVestigeClockSettings`; 2026-11-04), and the lighting that follows it (`AVestigeSkyLighting`). Seasons and weather are milestone 2 and 3.

**References:** no single Epic system; the rules here follow [Architecture.md, sections 15 and 18](../Architecture.md#15-the-day-flow). Lighting follows Epic's "Sky Atmosphere", "Exponential Height Fog" and "Lumen" pages.

**Rules:**

- One owner of game time (`UVestigeClockSubsystem`), in the game instance so it keeps time across maps; its speed and hours, sunrise and sunset included, are project settings (`UVestigeClockSettings`), never a level's. Everything else reads it or listens to its events (`OnNewDay`, `OnNewDayFinished`, `OnHourStarted`, `OnMinuteStarted`, `OnDayEnded`). Nothing keeps its own clock: something that happens at a time of day listens to `OnHourStarted`, which carries the hour since the start of the day's midnight (22 for 22:00, 24 for the next midnight), and something that goes on steadily over game time (the late-night drain) listens to `OnMinuteStarted`, which carries the minutes since that midnight (1441 for 00:01), rather than watching the clock or running a timer of its own. Both come once per whole minute or hour, in order, even when a long frame or a skip passes several, the minute before its hour.
- The game mode (`AVestigeGameMode`) owns the end of the day. The clock says when a day has ended (`OnDayEnded`: it reached the day's end, or something such as going to bed asked for it with `EndDay`) and stops; the game mode starts the next day. Whatever reacts to how the night went (the rest component's exhaustion) does it on `OnNewDay`, reading the clock's `GetLastDayEnd`, so the save made after it holds the result. Nothing but the game mode starts a day from `OnDayEnded`: a listener after it may already be in the new day.
- Blueprints, and so mods, hear of a new day through `On New Day` (`OnNewDayBlueprint`), the extension point broadcast after `OnNewDay` and before the save ([Architecture.md, section 12](../Architecture.md#extension-points-today)). A Blueprint finds the clock with **Get Vestige Clock**.
- Game time is separate from real time and from `UWorld::GetTimeSeconds`. Pausing and menus stop game time: the clock ticks with the open world, so pause stops it and time dilation (`slomo`) speeds it.
- Daily changes happen in `OnNewDay` listeners that finish synchronously ([Architecture.md, section 15](../Architecture.md#15-the-day-flow)).
- Seasons are gameplay tags and change on a day event; weather is chosen once per day from data (per-season weights), so it's saved as a value, not re-rolled on load.
- Lighting, fog and sky react to time and weather through the sky lighting actor (`AVestigeSkyLighting`, one per outdoor level), which reads the clock and its sunrise and sunset hours and never changes them; nothing else moves the sun. The sun turns in small steps, not every frame, so virtual shadow maps keep its cached shadows between them; brightness and colour may change every frame.

**Pitfalls:**

- Timers (`FTimerManager`) and periodic gameplay effects for game-time events: they run on real time, ignore skips and a stopped clock, and don't save. Schedule game-time events against the clock's events.
- Comparing hours as floats for equality; compare against ranges or minute counts.
