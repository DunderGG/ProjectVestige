# 17. Settings

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** none yet beyond the engine defaults.

**References:** Epic's `UGameUserSettings` (resolution, scalability, frame limit) and Enhanced Input's player-mappable key settings; Lyra's split between `ULyraSettingsLocal` (machine-specific, a `UGameUserSettings` subclass saved in config) and `ULyraSettingsShared` (player preferences, a `USaveGame` that would follow the player between machines).

**Rules:**

- Follow Lyra's split: `UVestigeSettingsLocal` (a `UGameUserSettings` subclass, set in `DefaultEngine.ini` as the game user settings class) for graphics, scalability, resolution and audio device; `UVestigeSettingsShared` (a `USaveGame`) for preferences like subtitles, survival needs on or off ([GameDesign.md](../GameDesign.md)) and volume.
- **Key bindings are saved by Enhanced Input's own user settings** (`UEnhancedInputUserSettings`, a save game of its own), not in `UVestigeSettingsShared` (changed with [H6](../CodeReview.md#h6--input-is-built-in-c-with-hard-coded-keys), 2026-11-07; decision log): it is the engine's tested path for key profiles and slots, and remaps keep working without our code. When this section is built:
   - `bEnableUserSettings=True` goes in `DefaultInput.ini` (under `[/Script/EnhancedInput.EnhancedInputDeveloperSettings]`); the pawn already registers its contexts with the user settings when it adds them ([section 5](05-Input.md)).
   - Its save slot is `EnhancedInputUserSettings` in 5.8 (`InputSettingsSaveSlotName` in the same settings). It must never clash with the farm's `Farm` and `Farm2` slots, and New game or deleting saves never removes it.
   - Our settings screen calls `SaveSettings()` after each remap.
   - The player-mappable names (the input tags, `Vestige.Input.UseTool`) are the keys remaps are saved under, so they are frozen once a player can save a remap.
- Settings apply through the owner of each system (audio volumes through control buses, scalability through `UGameUserSettings`), not by each system reading the settings file.
- Settings are saved separately from the farm save, so a new game keeps them.
- The first run picks scalability with `RunHardwareBenchmark`, and the player can change it.

**Pitfalls:**

- Storing gameplay difficulty options in config files that the player can't reach from the menu.
