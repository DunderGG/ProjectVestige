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
- **Saves** live in `Saved/SaveGames/`: `Farm.sav` and `Farm2.sav` are the two slots saves alternate between, and `Farm_Version0.sav` and `Farm_Version1.sav` are backups of saves from older save versions, made before their first upgrade. Before testing anything that writes saves, copy the folder somewhere safe, and copy it back afterwards to get your farm back. F9 in the game deletes both slots and starts a new game on day 1; the backups stay.
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
- [ ] **Settings** (77a2218): Edit > Project Settings > Game > Vestige HUD shows Max Messages (5) and Message Seconds (3). Changing them takes effect the next time you press Play, and is saved to `Config/DefaultGame.ini`; tell the agent the values you settle on.
- [ ] **Log** (77a2218): every message also appears in the Output Log (Window > Output Log, filter on `LogVestige`) as `Feedback: ...`, so you can read messages that have already faded.

### Unreal Engine 5.8.4 build, 2026-10-30

The project now builds only with 5.8. The rest of the upgrade (assets, rendering, documents) is in [Roadmap.md, section 4.1, Engine](Roadmap.md#41-foundations).

- [x] **Editor start** (674adff): double-clicking `Vestige.uproject` opens the 5.8 editor straight away, with no "Target Upgrade Required" or "missing modules" prompt, and the Output Log at start has no `LogVestige` errors.
- [x] **Walking** (de2e2c2): a tap of W, A, S or D still turns the farmer with barely a step, and holding a key walks as before.
- [x] **Visual Studio plugin** (ae539f7): with the Development Editor configuration, open `Source/Vestige/Player/VestigePlayerCharacter.h` and, if needed, Project > Rescan UE Blueprints. After a minute, grey hints such as "1 derived Blueprint" appear above `UCLASS`, `UPROPERTY` and `UFUNCTION` lines, and `Vestige.uproject` still has `"Optional": true` on the plugin.
   - Works once Tools > Options > Text Editor > CodeLens > Show Unreal Engine Blueprints References (C++) is ticked; it was off. Visual Studio's own asset reader logs "Unexpected EOF" for some assets during the rescan, which doesn't affect the hints.

### Item data assets, 2026-10-30

Items exist as data but nothing in play uses them yet, so the game itself should behave exactly as before.

- [x] **Validation** (46bf244): right-click `Content/Farm` > Validate Assets reports no errors, and the Output Log at editor start has no "is used by more than one asset" error.
   - Checked in 5.8 on 2026-10-31 by the agent with the `DataValidation` commandlet: all 23 assets under `Content/Farm` are valid, and the editor logs have no duplicate content ID error.
- [ ] **Item assets** (46bf244): `Content/Vestige/Items` has `DA_Item_Turnip` and `DA_Item_TurnipSeeds`. Opening the seeds shows one **Farm Seed Fragment** under Fragments, with Crop Id `FarmCrop:Base.Turnip`; its Crop Id picker lists only crops. Pressing + on Fragments offers the fragment classes to pick from.
- [ ] **Turnip crop** (46bf244): `DA_Crop_Turnip` shows Harvest Item Id `VestigeItem:Base.Turnip` and Yield under Farming > Harvest, and no Seed Price or Sell Price.
- [ ] **No change in play** (46bf244): planting, growing and harvesting a turnip work as in the [smoke test](#3-smoke-test).

### Source folders, 2026-10-30

- [x] **Solution** (adef4a7): after regenerating the project files, the solution shows `Core`, `Content`, `Time`, `Farming`, `Save`, `Player`, `UI`, `Template` and `Tests` under `Source/Vestige`.
- [x] **Editor start** (adef4a7): the editor opens `L_Farm` with no errors about missing classes, and the farmer Blueprint (`Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`), `BP_FarmTile`, the crops and the tools open normally.

### Tile highlight material, 2026-10-31

After the upgrade to 5.8 the frame drew dark, because its engine material wasn't flagged for instanced meshes and 5.8 draws the default grey material instead. It now uses `Content/Farm/Materials/M_TileHighlight`, made by script, so the editor hasn't shown it yet.

- [x] **Bright frame** (a397e1c): in Play in Editor and in Standalone Game, the frame around the targeted tile is bright yellow by day and still clearly visible at night, as before the upgrade.
- [x] **No warning** (a397e1c): the Output Log (Window > Output Log) has no "missing usage flag" warning after playing.
- [x] **Asset** (a397e1c): `M_TileHighlight` opens in the material editor with Blend Mode Opaque, Shading Model Unlit and Usage > Used with Instanced Static Meshes ticked, and the editor doesn't ask to save it.

### Inventory, 2026-10-31

Seeds and harvests are now items in an inventory on the player state. `BP_PlayerState` was made, and `BP_ThirdPersonGameMode` and `BP_Farmer` were changed, by a headless editor script, so the editor hasn't shown them yet. Back up `Saved/SaveGames/` first: your save is upgraded to version 2 on the first new day.

- [x] **Old save** (a3ace26): pressing Play loads your farm, and the HUD's inventory line reads "Bag: Turnip Seeds N   Turnip M" with the seed and harvest counts you had. Sleep once: the game saves, and stopping and playing again brings the same bag back.
- [x] **New game** (a3ace26): F9 starts with "Bag: Turnip Seeds 10".
- [x] **Planting and harvesting** (a3ace26): planting lowers the seeds by one; harvesting a ripe turnip adds a Turnip stack (or raises it). With no seeds left, the seeds tool says "Out of seeds." and the Turnip Seeds entry disappears from the line.
- [x] **Assets** (a3ace26): `Content/Farm/Blueprints/BP_PlayerState` opens; its **Inventory** component shows Slot Count 24 and Starting Items with `FarmItem:Base.TurnipSeeds` × 10. `BP_ThirdPersonGameMode` shows Player State Class `BP_PlayerState`. `BP_Farmer`'s **Farming** component shows Seed Item Id `FarmItem:Base.TurnipSeeds`, and its picker lists only items. None of them asks to be saved when you close it.
- [x] **Solution** (a3ace26): after regenerating the project files, the solution shows the new `Inventory` folder under `Source/Vestige`.

### Vestige rename, 2026-11-01

The general C++ types lost their `Farm` prefix (`AFarmHUD` is now `AVestigeHUD`, `AFarmerCharacter` is `AVestigePlayerCharacter`, the full list is in the Architecture decision log). The level, `BP_Farmer` and the tool and item assets still name the old classes until they are saved, and `[CoreRedirects]` in `Config/DefaultEngine.ini` map them. A headless check loaded them all without warnings, and the tests load a save that names the old classes, but the editor hasn't shown them yet. Back up `Saved/SaveGames/` first.

- [x] **Solution** (74bff97): regenerate the project files (right-click `Vestige.uproject` > Generate Visual Studio project files). The solution lists the renamed files (`Player/VestigePlayerCharacter.h`, `UI/VestigeHUD.h`...) and no `FarmHUD` or `FarmDayCycle` files.
- [x] **Old save** (74bff97): pressing Play loads your farm as you left it: the day, the clock, the bag and the tiles. Sleep once; stopping and playing again brings the same farm back.
- [x] **Assets** (74bff97): `BP_PlayerCharacter` opens and compiles without errors, and its Class Settings show Parent Class `Vestige Player Character`; its components include **Rest** (`Vestige Rest Component`) and **Character Movement** (`Vestige Character Movement Component`). In `L_Farm`, the day cycle actor shows class `VestigeDayCycle` with its settings as before. `DA_Tool_Hoe` and `DA_Item_TurnipSeeds` open with their values. The Output Log has no "Failed to load" or "can't find class" lines after opening them and playing. If the editor asks to save any of them, saving is fine: they then store the new names.
- [x] **Settings** (74bff97): Edit > Project Settings > Game lists **Vestige HUD** (no more Farm HUD) with the same values.
- [x] **Asset types** (1849c74): the tool, item and crop assets, `BP_PlayerCharacter` and `BP_PlayerState` were re-saved by script with the new types. `BP_PlayerCharacter`'s **Farming** component shows Toolbar Ids `VestigeTool:Base.Hoe` to `VestigeTool:Base.Hand` and Seed Item Id `VestigeItem:Base.TurnipSeeds`; `BP_PlayerState`'s **Inventory** shows Starting Items `VestigeItem:Base.TurnipSeeds` × 10; `DA_Crop_Turnip` shows Harvest Item Id `VestigeItem:Base.Turnip`. Each picker still lists only its own kind of asset, and none of them asks to be saved when you close it.
- [x] **Season tags** (1849c74): `DA_Crop_Turnip`'s Seasons picker offers `Vestige.Season.Spring` to `Vestige.Season.Winter`.
- [x] **Content folder** (35975de): our assets were moved by script from `Content/Farm` to `Content/Vestige`, with the same subfolders; `L_Farm` and `BP_ThirdPersonGameMode` were re-saved to point at them. The Content Browser shows `Content/Vestige` and no `Farm` folder (if an empty one shows, right-click `Content` > Fix Up Redirectors in Folder, then delete it). The grid in `L_Farm` still spawns `BP_FarmTile` tiles with the soil meshes, and the targeted tile's frame is bright yellow. Right-click `Content/Vestige` > Validate Assets reports no errors.
- [x] **Player Blueprint** (c101b8f): `BP_Farmer` is now `Content/Vestige/Blueprints/BP_PlayerCharacter` (renamed by script, no redirector left), with Parent Class `Vestige Player Character`. `BP_ThirdPersonGameMode` shows Default Pawn Class `BP_PlayerCharacter`, and pressing Play spawns the player as before.
- [x] The [smoke test](#3-smoke-test).

### Stamina, 2026-11-01

The player now has stamina, through the Gameplay Ability System (Systems.md, section 9). Using a tool is an ability: a use that changes the farm costs the tool's Stamina Cost (hoe and watering can 2, seeds and hand 1, out of 100), and every use takes the tool's Use Seconds (0.4 s) before the next one can start, twice as long at zero stamina. Sleeping refills stamina, and the save stores it. `DA_AbilitySet_Player` was made and `BP_PlayerState` and the four tool assets changed by a headless editor script, so the editor hasn't shown them yet. Back up `Saved/SaveGames/` first: your save is upgraded to version 3 on the first new day.

- [x] **Editor start** (1bfe8f1): the editor opens without asking to enable a plugin or rebuild, and the Output Log at start has no errors from `LogAbilitySystem` or `LogVestige`. Edit > Plugins shows **Gameplay Abilities** enabled.
- [x] **HUD** (1bfe8f1): under the status line, the HUD reads "Stamina 100 / 100". Tilling a tile drops it to 98 at once.
- [x] **No cost for nothing** (1bfe8f1): watering bare grass or using the hand on an unripe crop shows its message and leaves stamina where it was.
- [ ] **Use time** (1bfe8f1): E uses the tool once per press (holding it does nothing more). Pressing it quickly uses the tool about twice a second, no faster: presses that come too soon are ignored. Walking isn't slowed while a use is under way. Say if 0.4 s feels sluggish or too quick: it is Use Seconds on each tool asset in `Content/Vestige/Tools` (Details panel, Farming).
   - Corrected 2026-11-01: this check first said holding E repeats the use. It doesn't: E is bound to the press only, and the agent wrote the check wrong.
- [ ] **Out of stamina** (1bfe8f1): keep tilling and watering until the HUD reads "Stamina 0 / 100". The message "You're out of stamina. Everything takes longer until you sleep." appears once, the tools still work, each use now takes about twice as long, and stamina stays at 0.
- [ ] **Sleep** (1bfe8f1): sleeping (N) brings the HUD back to "Stamina 100 / 100", and so does passing out at 02:00.
- [ ] **Saving** (1bfe8f1): spend some stamina, then stop and play again: stamina is full, because the game saves at the start of each day, after sleeping. Your old save loads with full stamina.
- [ ] **Assets** (1bfe8f1): `Content/Vestige/Abilities/DA_AbilitySet_Player` opens with Attribute Sets `VestigeStaminaSet` and Abilities `VestigeGameplayAbility_UseTool`. `BP_PlayerState` shows Ability Sets with `DA_AbilitySet_Player`, and its components include **AbilitySystem**. `DA_Tool_Hoe` shows Stamina Cost 2 and Use Seconds 0.4. None of them asks to be saved when you close it.
- [ ] **Tuning** (1bfe8f1): over a few in-game days, say whether 100 stamina and these costs feel right: running out should happen on a busy day, not every day. Tell the agent the numbers you settle on.
- [ ] **Solution** (1bfe8f1): after regenerating the project files, the solution shows the new `AbilitySystem` folder under `Source/Vestige`.

### Editor chores

Changes only the editor can make safely. The steps are in [Roadmap.md, Housekeeping](Roadmap.md#3-housekeeping): re-saving the template Blueprints and `L_Farm`, moving `BP_FarmTile` and the materials with redirectors fixed, and regenerating the Android File Server token if it was ever used. After each one, run the [smoke test](#3-smoke-test).

## 3. Smoke test

About five minutes in `L_Farm`, in Play in Editor at High. Run it before pushing and at the end of each milestone, and after any editor chore. Back up `Saved/SaveGames/` first.

1. **Start:** a new game (F9) starts on day 1 at 06:00. The HUD shows the day, the clock, the tool, the stamina ("Stamina 100 / 100"), the bag ("Bag: Turnip Seeds 10") and the key hints; the Output Log has no `LogVestige` errors.
2. **Tools:** with each tool on the tile in front of you: the hoe (1) tills grass, the seeds (2) plant in tilled soil and use a seed, the watering can (3) darkens the soil, and the hand (4) does nothing on an unripe crop. The tool in hand changes with each key. Each action shows a short message, and each one that changes the farm lowers the stamina; facing away from the field says "Stand facing a farm tile." and costs nothing.
3. **Growing:** water a planted tile and sleep (N), a few times. The crop grows one stage per watered night, and a tile left dry doesn't grow. Harvest the ripe crop with the hand; a Turnip stack appears in the bag or grows. Each night in bed refills the stamina.
4. **Day and night:** let the clock run through an evening (it moves about one hour every 40 seconds unless the level's day cycle is set faster). The sun sets with warm light, the night is properly dark with faint moonlight, and at 02:00 the farmer passes out into the next day.
5. **Saving:** sleep ("Game saved."), stop the game and play again. The day, the bag, the tool in hand and every tile, crops included, come back.
6. **Highlight:** walk along the field edge and turn. The frame is bright yellow, by day and at night, follows the tile in front of you and disappears off the field.
7. **Performance:** in the console, `stat fps` and `stat unit`. In a busy part of the farm, by day and by night, the frame stays under 16.6 ms (60 fps), and sleeping shows no visible hitch ([Architecture.md, section 10](Architecture.md#10-performance-by-design)). Play in Editor adds its own cost, so check a doubtful number in Standalone Game. For a hitch on the new day, start the game with `-trace=default` and open the trace in Unreal Insights (`Engine\Binaries\Win64\UnrealInsights.exe`): `AVestigeDayCycle::StartNextDay` shows the whole new day, with the tiles, the save and the load inside it.

Put the save folder back afterwards if you want your farm as it was.

## 4. What isn't manual

- **Game rules are tested automatically**, by the `Vestige` automation tests: tile states and growth, tools and seeds, stamina (costs, the zero floor, the slower uses and the refill), the inventory's stacking and saving, the clock, sleeping and passing out, the save rules and upgrades, content IDs, the include direction between source folders and the rendering settings. A rule is never covered by a manual check alone. If a check here ever finds a rule broken, the fix comes with a test that would have caught it.
- **This document is for what tests can't judge:** how the game looks and feels, lighting, the HUD as you see it, input, saves on real files across sessions, performance, and steps that need the editor.
- **When a change needs a manual check**, the commit that makes it adds a line to [Pending checks](#2-pending-checks) under a dated heading, with the commit and what to look for. A system that gains lasting behavior also gets a line in the [smoke test](#3-smoke-test).
