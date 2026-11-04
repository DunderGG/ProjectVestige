# Config

This folder contains Project Vestige's Unreal Engine project configuration. The `Default*.ini` files provide source-controlled defaults that Unreal Engine combines with its engine, platform, and user settings.

## Files

| File | Purpose |
|---|---|
| `DefaultEngine.ini` | Core engine and platform settings, including startup maps, rendering, audio, Asset Manager scans, collision profiles, and redirects for renamed classes. |
| `DefaultGame.ini` | General project settings and additional Asset Manager configuration, including which crop, tool, and item assets Unreal scans. |
| `DefaultEditor.ini` | Editor-only behavior, such as Blueprint editing and compilation options. |
| `DefaultInput.ini` | Project-wide input settings, including axis behavior and Enhanced Input classes. |
| `DefaultGameplayTags.ini` | Gameplay tags registered from configuration, such as the season tags. |
| `DefaultScalability.ini` | Project adjustments to Unreal's graphics-quality levels: cheaper volumetric clouds below Epic. |
| `DefaultGameUserSettings.ini` | Initial player graphics-quality defaults. Unreal stores a player's chosen settings separately, and the first-run hardware benchmark can replace these defaults. |

`Layouts/` is reserved for editor layout configuration files; it is currently empty.

The player's local settings are stored under `Saved/Config/`, not here. Changes to the `Default*.ini` files affect project defaults and should be made intentionally, since they can change editor or game behavior for everyone using the project.
