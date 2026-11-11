# 20. Maps, loading screens and the main menu

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** one map (`L_Farm`), no menu, no loading screen. Decided 2026-11-04: separate maps opened with `OpenLevel`, with the clock and the farm data in game-instance subsystems (built) ([Architecture.md, section 2](../Architecture.md#2-unreals-gameplay-framework-where-things-go)).

**References:** Epic's "Level Streaming", "World Partition", "Level Instances" and "Data Layers" pages, and the MoviePlayer loading screen; Lyra's CommonLoadingScreen plugin (a loading screen shown during map loads and while systems report they're not ready) and its front-end map.

**Rules:**

- The farm, the town and the wasteland are separate maps opened with `OpenLevel` (decision log, 2026-11-04), not streamed sublevels or World Partition. What must go on while its map is closed lives in a game-instance subsystem; a level only shows it.
- A main menu is its own small map with its own game mode, set in its World Settings, so it loads nothing from the farm: `BP_GameMode`, the default for every map, hard-references the player's Blueprints.
- A loading screen covers map changes and stays up until the systems that load content at start (the save; the content catalogue loads before the first map, [section 2](02-Content.md)) report ready, as Lyra's loading screen waits on its "loading processors".
- A main menu loads the chosen save with an explicit Continue (or starts a New game), instead of the save subsystem loading when the first map is initialised; the save is already read into the game instance, so travelling between maps never loads it again ([section 1](01-Saving.md), [Architecture.md, section 6](../Architecture.md#6-saving)).

**Pitfalls:**

- A map that hard-references every other map's content through a game mode or a Blueprint.
