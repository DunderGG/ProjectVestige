# 19. Debugging and cheats

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** built (2026-11-04, [Roadmap 4.0](../Roadmap.md#40-structure-from-the-code-review)): `UVestigeCheatManager` (`Player/`) with the console commands `NewGame` (deletes both saves and reopens the map on day 1), `SkipDay` (the player sleeps where they stand), `SkipToTime <Time>` (the clock runs forward to a time today, typed as digits, 1930 for 19:30, broadcasting the minutes and hours it passes, and landing half a game minute into the typed minute so that minute's events come; added 2026-11-06) and `BenchmarkField <Size> <CropId>` (a Size by Size field of that crop, ripe, for the performance baseline in [Architecture.md, section 10](../Architecture.md#measuring)). They replace the test keys F9 and N, which are no longer bound. No console variables or debug drawing yet. The target tile highlight is a shipping component, not debug drawing.

**References:** Epic's `UCheatManager` (console commands in development builds), console variables (`TAutoConsoleVariable`), the Gameplay Debugger and Visual Logger; Lyra's `ULyraCheatManager`.

**Rules:**

- Developer commands (add seeds, skip to a day, grow every crop) live in a `UVestigeCheatManager` (set as the player controller's cheat manager class) as `UFUNCTION(Exec)` functions. Cheat managers aren't created in shipping builds: `APlayerController::AddCheats` is compiled out there, and our controller sets `CheatClass` only under `UE_WITH_CHEAT_MANAGER`, the engine's own guard. Outside Shipping the engine creates one when the game mode's `AllowCheats` agrees (a standalone game, Play in Editor included).
- A command asks the owner of the action to do it (`SkipDay` calls the player's rest component's `Sleep`, as the bed does) and has no rules of its own, so a cheat does what the game does. A command that can't run (no player state, no save subsystem) logs a warning and does nothing.
- Commands are typed in the console while playing ([PlayTest.md, Before you test](../PlayTest.md#1-before-you-test) says how to open it), named as verbs in PascalCase without a prefix (`NewGame`, `SkipDay`). Check the name isn't already an engine command (type it in the console first).
- Debug visuals sit behind console variables (`farm.Debug.Targeting 1`) and `#if ENABLE_DRAW_DEBUG`, never always on.
- No debug keys in the game's input: a developer action is a console command. Something players need (a new game) comes as a menu, behind a confirmation when it destroys data.
- Robots get Gameplay Debugger categories and Visual Logger output when their AI is built.

**Pitfalls:**

- Debug-only behavior that players come to rely on ([ArchitectureAudit A-10](../ArchitectureAudit.md)).
