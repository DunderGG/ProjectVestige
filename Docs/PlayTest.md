# Project Vestige play-testing

The one place for what to check by hand in the editor.

- **[Before you test](#1-before-you-test)**: building, the automated tests, starting the game and handling saves.
- **[Pending checks](#2-pending-checks)**: what recent changes still need you to look at. Work through it after pulling or building new code.
- **[Smoke test](#3-smoke-test)**: a five-minute pass over every system, before pushing and at the end of each milestone.
- **[What isn't manual](#4-what-isnt-manual)**: what the automated tests cover instead.

---

## 1. Before you test

- **New or moved source files:** regenerate the Visual Studio project files (right-click `Vestige.uproject` in Explorer > Generate Visual Studio project files), so the solution shows them.
- **Building:** an agent working on this PC builds `VestigeEditor` (Development Editor, Win64) and runs the tests before every commit, so after its changes the editor is ready to open; the agent says so. Rebuild yourself only after changing C++ yourself, after pulling code nobody built on this PC, or when the editor says the Vestige module is missing or out of date (answer Yes to rebuild it, no Visual Studio needed). For C++ that adds classes, `UPROPERTY`s or `UFUNCTION`s, close the editor and build in Visual Studio; Live Coding can't add those, it only patches function bodies.
- **Run the automated tests first.** In the editor: Tools > Test Automation, filter on `Vestige`, tick it and press Start Tests. Or from a command prompt with the editor closed:

  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Unreal Projects\Vestige\Vestige.uproject" -ExecCmds="Automation RunTests Vestige; Quit" -unattended -nopause -nullrhi -nosplash -testexit="Automation Test Queue Empty"
  ```

  Every test logs `Result={Success}`. Two `LogAutomationTest: Error: Condition failed` lines at startup come from the engine and can be ignored.
- **Start the game:** open `/Game/Maps/L_Farm` (Content Browser > `Content/Maps`, double-click), then Play (Alt+P) for Play in Editor. For what players get, and for any performance numbers, use the arrow next to Play > Standalone Game instead.
- **Graphics settings (scalability):** the quickest way, which works in Play in Editor and in Standalone Game, is the console ([see below](#1-before-you-test)). While playing, open it, type `scalability 2` and press Enter for High, or `scalability 3` for Epic (0 is Low, 1 Medium). Everything switches at once. To see whether hardware ray tracing is on, type `r.Lumen.HardwareRayTracing` on its own: it prints 0 at High and 1 at Epic. New players start at **High**. The level viewport keeps its own setting between sessions (its Settings menu > Engine Scalability Settings); the console changes it too.
- **Saves** live in `Saved/SaveGames/`: `Farm.sav` and `Farm2.sav` are the two slots saves alternate between, and `Farm_Version0.sav` is the backup of a save from before save versions. Before testing anything that writes saves, copy the folder somewhere safe, and copy it back afterwards to get your farm back. F9 in the game deletes both slots and starts a new game on day 1; the backups stay.
- **Logs:** Window > Output Log. Our messages are under `LogVestige`; note any warning or error even if the game looks fine.
- **Console**, for `scalability`, `stat fps`, `stat unit` and similar commands. While playing, press **§** (the key under Esc on a Swedish keyboard) or the key left of Backspace (´ and `), the US layout's console key. If neither works, press Shift+F1 to free the mouse, click the **Cmd** box at the bottom of the editor (in the Output Log panel, or the status bar at the bottom left), type the command there and press Enter; it goes to the game you are playing. Click back into the game viewport afterwards.

## 2. Pending checks

Checks for recent changes that haven't been confirmed in the editor yet. Each names the commit to look at if something is wrong. When a check passes, delete its line (or tell the agent, who deletes it). When one fails, write what you saw under it; it is fixed before new work starts.

### Code clean-up, 2026-10-29

- [x] **Sleep and passing out** (758f81c, 9d77ee1): N starts the next day. Letting the clock reach 02:00 passes the farmer out into the next day.
- [x] **Key hints** (6805d86, 812ea57): the bottom line reads "1 Hoe   2 Seeds   3 Watering Can   4 Hand   |   E Use tool   |   N Sleep   |   F9 New game". The tool names come from the tool data assets.
   - This works, but we need a much better toolbar.
- [x] **Clock** (812ea57): the status line shows the day, a 24-hour clock in 10-minute steps and the tool in hand.
- [x] **Counts** (812ea57): planting lowers the seed count by one, and harvesting a ripe crop raises the harvest count by the crop's yield. Both update at once.
- [x] **Old save** (9cdf0ac, 8fff295): your existing save loads with its day, inventory and planted crops. A save from before 2026-10-29 is copied to `Farm_Version0.sav` first. Each new day saves to whichever of `Farm.sav` and `Farm2.sav` doesn't hold the newest save.
- [x] **Lighting** (42f3a44): at High and at Epic, days look as before and nights are properly dark. Epic may differ slightly, since only Epic uses hardware ray tracing now. **How: while playing, open the console (§, or see [Before you test](#1-before-you-test)), type `scalability 2` for High, look at a day and a night, then `scalability 3` for Epic and look again ([Before you test](#1-before-you-test)).** Let the clock run to get from day to night; sleeping (N) brings back the morning.
- [x] **Content validation** (139e88d): right-click `Content/Farm` > Validate Assets reports no errors, and the Output Log at editor start has no "is used by more than one asset" error.

### Tile targeting, 2026-10-30

Replaces the tile highlight check above: the frame now sits on a neighbouring tile chosen by the movement direction (GameDesign.md, section 6). The tap, walking and feel checks moved to [Smooth turning](#smooth-turning-2026-10-30) below; the results that led to it are in commit 8932448.

- [x] **Field edge** (ce1f1dc): standing on an edge tile and turning to face outwards hides the frame; turning back shows it again.
- [x] **Tools** (ce1f1dc): the tools act on the framed tile, never on the one the farmer stands on.
- [ ] **Gamepad** (ce1f1dc), if you have one: a flick of the left stick turns the farmer with barely a step, and holding it walks, the same as the keys.
   - Unable to test Gamepad support now. Save this for later, when we have a gamepad to test with.

### Smooth turning, 2026-10-30

Replaces the tap to turn, walking and feel checks of the tile targeting change, after David's notes that turns snapped round and the farmer stood still before walking. Walking now eases in over 0.25 s from a press, and every turn goes through the movement component at 540 degrees per second (GameDesign.md, section 6). Both are David's tuning from play.

- [x] **Tap** (a3e118b): a quick tap of W, A, S or D turns the farmer smoothly toward that tile, with no pop, and he moves no more than a few centimetres. The frame moves to the neighbouring tile in that direction, and after a tap he is still on his own tile.
- [x] **Walking** (a3e118b): holding a key walks at once, with no pause before he starts, and the frame stays one tile ahead in the direction of travel.
- [x] **No double turn** (a3e118b): turn the camera to an angle to the field (about 30 degrees) and tap or walk. The farmer turns once, without snapping one way and sliding back.
- [x] **Settling** (a3e118b): after walking at an angle and letting go, he finishes turning to face the framed tile.
- [x] **Tuning** (a3e118b): if a tap moves too far or the walk feels slow to start, change Move Ramp Seconds on `BP_Farmer` in `Content/Farm/Blueprints` (Details panel, Farming > Input; lower walks sooner, higher keeps taps smaller). If turns feel slow or too snappy, change Rotation Rate Z on its Character Movement component (Details panel, select CharacterMovement, Character Movement (Rotation Settings)). Note the values that feel right.
   - David tuned them in play on 2026-10-30 (300 degrees per second and 0.5 s felt heavy; 540 and 0.25 feel good), and they are now the C++ defaults. Still open for more tuning.

### Message boxes, 2026-10-30

Replaces the earlier messages check (c87081e), after David's note that messages stacked in one box read like a log.

- [x] **Separate boxes** (826f80b): every message shows in a small box of its own, stacked above the key hints with a gap between them. Sleeping shows "You slept. Day N begins." and "Game saved." as two boxes.
- [x] **Own timers** (826f80b): each box fades in, stays about three seconds and fades out on its own. Use a tool, wait a second and use it again: the first box fades a second before the second.
- [x] **At most three** (826f80b): with three boxes showing, a fourth message pushes out the oldest. Tilling and planting quickly is the easiest way to get there.

### Five messages and the message log, 2026-10-30

Raises the cap from the message boxes checks above.

- [ ] **At most five** (77a2218): with five boxes showing, a sixth message pushes out the oldest. Tilling and planting quickly along a row is the easiest way to get there. The boxes stay clear of the key hints and the status line.
- [ ] **Settings** (77a2218): Edit > Project Settings > Game > Farm HUD shows Max Messages (5) and Message Seconds (3). Changing them takes effect the next time you press Play, and is saved to `Config/DefaultGame.ini`; tell the agent the values you settle on.
- [ ] **Log** (77a2218): every message also appears in the Output Log (Window > Output Log, filter on `LogVestige`) as `Feedback: ...`, so you can read messages that have already faded.

### Unreal Engine 5.8.4 build, 2026-10-30

The project now builds only with 5.8. The rest of the upgrade (assets, rendering, documents) is in [Roadmap.md, section 4.1, Engine](Roadmap.md#41-foundations).

- [x] **Editor start** (674adff): double-clicking `Vestige.uproject` opens the 5.8 editor straight away, with no "Target Upgrade Required" or "missing modules" prompt, and the Output Log at start has no `LogVestige` errors.
- [x] **Walking** (de2e2c2): a tap of W, A, S or D still turns the farmer with barely a step, and holding a key walks as before.
- [x] **Visual Studio plugin** (ae539f7): with the Development Editor configuration, open `Source/Vestige/Player/FarmerCharacter.h` and, if needed, Project > Rescan UE Blueprints. After a minute, grey hints such as "1 derived Blueprint" appear above `UCLASS`, `UPROPERTY` and `UFUNCTION` lines, and `Vestige.uproject` still has `"Optional": true` on the plugin.
   - Works once Tools > Options > Text Editor > CodeLens > Show Unreal Engine Blueprints References (C++) is ticked; it was off. Visual Studio's own asset reader logs "Unexpected EOF" for some assets during the rescan, which doesn't affect the hints.

### Item data assets, 2026-10-30

Items exist as data but nothing in play uses them yet, so the game itself should behave exactly as before.

- [ ] **Validation** (46bf244): right-click `Content/Farm` > Validate Assets reports no errors, and the Output Log at editor start has no "is used by more than one asset" error.
- [ ] **Item assets** (46bf244): `Content/Farm/Items` has `DA_Item_Turnip` and `DA_Item_TurnipSeeds`. Opening the seeds shows one **Farm Seed Fragment** under Fragments, with Crop Id `FarmCrop:Base.Turnip`; its Crop Id picker lists only crops. Pressing + on Fragments offers the fragment classes to pick from.
- [ ] **Turnip crop** (46bf244): `DA_Crop_Turnip` shows Harvest Item Id `FarmItem:Base.Turnip` and Yield under Farming > Harvest, and no Seed Price or Sell Price.
- [ ] **No change in play** (46bf244): planting, growing and harvesting a turnip work as in the [smoke test](#3-smoke-test).

### Source folders, 2026-10-30

- [x] **Solution** (adef4a7): after regenerating the project files, the solution shows `Core`, `Content`, `Time`, `Farming`, `Save`, `Player`, `UI`, `Template` and `Tests` under `Source/Vestige`.
- [x] **Editor start** (adef4a7): the editor opens `L_Farm` with no errors about missing classes, and the farmer Blueprint (`Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`), `BP_FarmTile`, the crops and the tools open normally.

### Tile highlight material, 2026-10-31

After the upgrade to 5.8 the frame drew dark, because its engine material wasn't flagged for instanced meshes and 5.8 draws the default grey material instead. It now uses `Content/Farm/Materials/M_TileHighlight`, made by script, so the editor hasn't shown it yet.

- [ ] **Bright frame** (a397e1c): in Play in Editor and in Standalone Game, the frame around the targeted tile is bright yellow by day and still clearly visible at night, as before the upgrade.
- [ ] **No warning** (a397e1c): the Output Log (Window > Output Log) has no "missing usage flag" warning after playing.
- [ ] **Asset** (a397e1c): `M_TileHighlight` opens in the material editor with Blend Mode Opaque, Shading Model Unlit and Usage > Used with Instanced Static Meshes ticked, and the editor doesn't ask to save it.

### Editor chores

Changes only the editor can make safely. The steps are in [Roadmap.md, Housekeeping](Roadmap.md#3-housekeeping): re-saving the template Blueprints and `L_Farm`, moving `BP_FarmTile` and the materials with redirectors fixed, and regenerating the Android File Server token if it was ever used. After each one, run the [smoke test](#3-smoke-test).

## 3. Smoke test

About five minutes in `L_Farm`, in Play in Editor at High. Run it before pushing and at the end of each milestone, and after any editor chore. Back up `Saved/SaveGames/` first.

1. **Start:** a new game (F9) starts on day 1 at 06:00. The HUD shows the day, the clock, the tool, seeds and harvested crops, and the key hints; the Output Log has no `LogVestige` errors.
2. **Tools:** with each tool on the tile in front of you: the hoe (1) tills grass, the seeds (2) plant in tilled soil and use a seed, the watering can (3) darkens the soil, and the hand (4) does nothing on an unripe crop. The tool in hand changes with each key. Each action shows a short message; facing away from the field says "Stand facing a farm tile."
3. **Growing:** water a planted tile and sleep (N), a few times. The crop grows one stage per watered night, and a tile left dry doesn't grow. Harvest the ripe crop with the hand; the harvest count goes up.
4. **Day and night:** let the clock run through an evening (it moves about one hour every 40 seconds unless the level's day cycle is set faster). The sun sets with warm light, the night is properly dark with faint moonlight, and at 02:00 the farmer passes out into the next day.
5. **Saving:** sleep ("Game saved."), stop the game and play again. The day, seeds, harvest count, tool in hand and every tile, crops included, come back.
6. **Highlight:** walk along the field edge and turn. The frame is bright yellow, by day and at night, follows the tile in front of you and disappears off the field.
7. **Performance:** in the console, `stat fps` and `stat unit`. In a busy part of the farm, by day and by night, the frame stays under 16.6 ms (60 fps), and sleeping shows no visible hitch ([Architecture.md, section 10](Architecture.md#10-performance-by-design)). Play in Editor adds its own cost, so check a doubtful number in Standalone Game. For a hitch on the new day, start the game with `-trace=default` and open the trace in Unreal Insights (`Engine\Binaries\Win64\UnrealInsights.exe`): `AFarmDayCycle::StartNextDay` shows the whole new day, with the tiles, the save and the load inside it.

Put the save folder back afterwards if you want your farm as it was.

## 4. What isn't manual

- **Game rules are tested automatically**, by the `Vestige` automation tests: tile states and growth, tools and seeds, the clock, sleeping and passing out, the save rules and upgrades, content IDs, the include direction between source folders and the rendering settings. A rule is never covered by a manual check alone. If a check here ever finds a rule broken, the fix comes with a test that would have caught it.
- **This document is for what tests can't judge:** how the game looks and feels, lighting, the HUD as you see it, input, saves on real files across sessions, performance, and steps that need the editor.
- **When a change needs a manual check**, the commit that makes it adds a line to [Pending checks](#2-pending-checks) under a dated heading, with the commit and what to look for. A system that gains lasting behavior also gets a line in the [smoke test](#3-smoke-test).
