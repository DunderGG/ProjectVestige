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
- **Saves** live in `Saved/SaveGames/`: `Farm.sav` and `Farm2.sav` are the two slots saves alternate between. Before testing anything that writes saves, copy the folder somewhere safe, and copy it back afterwards to get your farm back. The console command `NewGame` deletes both slots and starts a new game on day 1. Until the first public build, saves aren't upgraded: when a change makes older saves unreadable, the game says it couldn't read the save and won't save, and `NewGame` starts over.
- **Logs:** Window > Output Log. Each of our systems has its own category (`LogFarm`, `LogVestigeSave`, `LogVestigeFeedback` and the others in [Systems.md, section 21](Systems.md#21-logging)), and each line starts with who wrote it, as in `[FarmGrid_1] AFarmGrid::SpawnTiles: ...`. What each part of a line means (the time, the frame number, the category, the sender) is in [Systems.md, section 21, Reading a line](Systems.md#21-logging). To see only ours, type `LogVestige` or `LogFarm` in the search box, or open the Output Log's **Filters** menu > **Categories** and tick ours. That menu only lists the categories that have written a line since the editor started, so `LogFarm`, which only warns about setup problems, is usually missing from it; the search box always works. To give each category its own colour, open Edit > Editor Preferences > General > Output Log and set **Category Colorization Mode** to **Colorize Whole Line** (chosen 2026-11-02; warnings and errors keep their yellow and red). Both are your own editor settings and aren't committed. To see a system's tracing lines, type `log LogFarm Verbose` in the console (and `log reset` to go back). Note any warning or error even if the game looks fine; the full log of the last run is `Saved/Logs/Vestige.log`.
- **Console**, for `scalability`, `stat fps`, `stat unit`, our developer commands (`NewGame` starts a new game, `SkipDay` sleeps where you stand; [Systems.md, section 19](Systems.md#19-debugging-and-cheats)) and similar commands. While playing, press **§** (the key under Esc on a Swedish keyboard) or the key left of Backspace (´ and `), the US layout's console key. If neither works, press Shift+F1 to free the mouse, click the **Cmd** box at the bottom of the editor (in the Output Log panel, or the status bar at the bottom left), type the command there and press Enter; it goes to the game you are playing. Click back into the game viewport afterwards.

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
- [ ] **Log** (77a2218): every message also appears in the Output Log (Window > Output Log, filter on `LogVestigeFeedback`) as `[L_Farm] UVestigeFeedbackSubsystem::Notify: ...`, so you can read messages that have already faded. (Updated 2026-11-02 for the logging categories; the line read `LogVestige: Feedback: ...` before.)

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
- [x] **Use time** (1bfe8f1): E uses the tool once per press (holding it does nothing more). Pressing it quickly uses the tool about twice a second, no faster: presses that come too soon are ignored. Walking isn't slowed while a use is under way. Say if 0.4 s feels sluggish or too quick: it is Use Seconds on each tool asset in `Content/Vestige/Tools` (Details panel, Farming).
   - Corrected 2026-11-01: this check first said holding E repeats the use. It doesn't: E is bound to the press only, and the agent wrote the check wrong.
- [x] **Out of stamina** (1bfe8f1): keep tilling and watering until the HUD reads "Stamina 0 / 100". The message "You're out of stamina. Everything takes longer until you sleep." appears once, the tools still work, each use now takes about twice as long, and stamina stays at 0.
- [x] **Sleep** (1bfe8f1): sleeping (N) brings the HUD back to "Stamina 100 / 100", and so does passing out at 02:00.
- [x] **Saving** (1bfe8f1): spend some stamina, then stop and play again: stamina is full, because the game saves at the start of each day, after sleeping. Your old save loads with full stamina.
- [x] **Assets** (1bfe8f1): `Content/Vestige/Abilities/DA_AbilitySet_Player` opens with Attribute Sets `VestigeStaminaSet` and Abilities `VestigeGameplayAbility_UseTool`. `BP_PlayerState` shows Ability Sets with `DA_AbilitySet_Player`, and its components include **AbilitySystem**. `DA_Tool_Hoe` shows Stamina Cost 2 and Use Seconds 0.4. None of them asks to be saved when you close it.
- [x] **Tuning** (1bfe8f1): over a few in-game days, say whether 100 stamina and these costs feel right: running out should happen on a busy day, not every day. Tell the agent the numbers you settle on.
- [x] **Solution** (1bfe8f1): after regenerating the project files, the solution shows the new `AbilitySystem` folder under `Source/Vestige`.

### Blueprint extension points, 2026-11-01

Blueprints, and later mods, can now hear of a new day, a harvest and a tool use ([Architecture.md, section 12](Architecture.md#extension-points-today)). The tests check that the events fire; these checks are for how they look in the editor. Don't save the Blueprints you try them in: close them with Don't Save.

- [x] **Events in the Details panel** (7e84ca8): open `BP_PlayerCharacter` (`Content/Vestige/Blueprints`), select **FarmingComponent** in the Components panel, and scroll the Details panel to Events: **On Crop Harvested** and **On Tool Used** are listed with a + button. Pressing + on On Tool Used adds an event node with **Tool** and **Tile** pins.
- [x] **Content ID** (7e84ca8): drag from the Tool pin and search "Get Content Id"; the node is there. Wire it into a Print String from the On Tool Used node, compile and play: tilling grass prints `Base.Hoe`, watering bare grass prints nothing. Harvesting a ripe crop with the hand also fires **On Crop Harvested**.
- [x] **New day** (7e84ca8): in the same Event Graph, right-click and search "Get Day Cycle", drag from its return value and search "Assign On New Day" (or "Bind Event to On New Day"). Wire it from Event BeginPlay to a Print String of the Day, compile and play: sleeping (N) prints 2. Close the Blueprint with Don't Save afterwards.

### Interaction and the bed, 2026-11-01

The player now sleeps in a bed, by walking up to it and pressing **F** ([Systems.md, section 7](Systems.md#7-interaction)). N stays as a test key that skips to the next day anywhere. The tests check the rules (focus, the first option, the bed sleeping the player, the ability); these checks are for how it looks and feels. The commit is the one titled "Add the interaction interface and a bed to sleep in".

- [x] **The bed**: a placeholder bed (a flat grey box, 2 m long) lies lengthwise along the near edge of the field, to the left of where the farmer starts, outside the tiles. The farmer can't walk through it.
- [x] **Prompt**: walking up to the bed shows a box reading "F Sleep" below the middle of the screen. It appears when the farmer is about a step from the bed, not from across the field, and disappears again when he walks away. Say if the reach feels too short or too long: it is Interaction Radius on the farmer's **Interaction** component (100 cm), and the bed's Interaction Volume box.
- [x] **Sleeping**: pressing F by the bed starts the next day at 06:00 with "You slept. Day 2 begins.", full stamina and "Game saved.", the same as N did. F away from the bed does nothing.
- [x] **Key hints**: the bottom line reads "... | E Use tool | F Interact | N Skip day (test) | F9 New game".
- [x] **Tool by the bed**: standing on the edge tile next to the bed, facing into the field, the prompt shows and E still uses the tool on the framed tile, not the bed. (Moved 2026-11-01: the bed first stood too far from the field to test this.)
- [x] **Assets**: `DA_AbilitySet_Player` lists `VestigeGameplayAbility_Interact` under Abilities, and `Content/Vestige/Blueprints/BP_Bed` opens with a cube **BedMesh** and an **InteractionVolume** box. Project Settings > Engine > Collision lists an Object Channel **Interactable** (default response Ignore) and a preset **VestigeInteractable**. None of them asks to be saved when you close it.
- [x] **Solution**: after regenerating the project files, the solution shows the new `Interaction` folder under `Source/Vestige`.

### Logging, 2026-11-02

Our log lines are now split into one category per system, and each line starts with its sender and function ([Systems.md, section 21](Systems.md#21-logging)). The tests check the line format and that no code logs around the macro; these checks are for the Output Log itself. The commit is the one titled "Split the log into a category per system with a sender macro".

- [x] **Colours**: with Category Colorization Mode set as in [Before you test](#1-before-you-test), `LogFarm`, `LogVestigeSave`, `LogVestigeFeedback` and the others each show in their own colour. Say which mode you settle on.
   - Chose the mode "Colorize whole line"
- [x] **Lines**: playing a day and sleeping shows `LogVestigeFeedback` lines such as `[L_Farm] UVestigeFeedbackSubsystem::Notify: Game saved.`, and a load at Play shows `LogVestigeSave: [VestigeSaveSubsystem_0] UVestigeSaveSubsystem::LoadFarm: Loading day ...`. No line shows `{` braces or a field name instead of its value.
- [x] **Filters**: after a Play, the Output Log's Filters > Categories menu lists `LogVestigeSave` and `LogVestigeFeedback`, and ticking only `LogVestigeSave` hides the feedback lines. `LogFarm` and the other categories appear in the menu only once they have written a line, which they don't in a level that is set up right. (Corrected 2026-11-02: this check first said to tick `LogFarm`.)
- [x] **Console**: `log list` in the console lists our categories, and `log LogVestigeSave Warning` hides the save's `Loading day` line on the next Play (`log reset` brings it back).
- [x] **Solution**: after regenerating the project files, each layer folder shows its new log header (`Farming/FarmLog.h`, `Save/VestigeSaveLog.h` and so on) and `Core/VestigeLog.h`.

### Out of stamina on the HUD, 2026-11-02

The stamina line turns red while the player is out of stamina. The HUD follows the set's out-of-stamina tag, so it changes colour exactly when the "You're out of stamina" message appears. The commit is the one titled "Show the stamina line in red while out of stamina".

- [x] **Red at zero**: keep tilling and watering until the HUD reads "Stamina 0 / 100". The line turns red at the same moment as the out-of-stamina message, and stays red while you keep working.
- [x] **White again**: sleeping (F at the bed, or N) brings the line back to white with "Stamina 100 / 100".
- [x] **Colour**: say if the red is hard to read on the dark backdrop, or too loud. It is `VestigeHudStyle::Warning` in `UI/VestigeHudWidget.cpp`.

### Late nights, 2026-11-02

The late-night rules from [GameDesign.md, section 5](GameDesign.md#5-time-days-and-seasons): no pass-out at 02:00, a stamina drain from midnight, and exhaustion after going to bed at 00:00 or later. The tests cover the rules; these checks are for how they feel and read. The commit is the one titled "Add the late-night rules: drain, exhaustion and warnings". Back up `Saved/SaveGames/` first: your save is upgraded to version 4 on the first new day. To get to the night quickly, lower Real Seconds Per Game Hour on the `VestigeDayCycle` in `L_Farm` for the session (Details panel, Farming > Time), and put it back afterwards.

- [x] **Warnings**: at 22:00 a message says it's getting late, and at 00:00 one says it's past midnight and stamina drains until you sleep.
- [x] **Drain**: after midnight the stamina number counts down by itself, about 10 per game hour, even standing still. It stops at 0, the line turns red, and the out-of-stamina message appears once.
- [x] **Late bed**: sleeping (F at the bed, or N) after midnight shows "You slept. Day N begins.", then "You went to bed late and wake up exhausted.", then "Game saved.". The HUD reads "Stamina 75 / 100   Exhausted", and the drain has stopped.
- [x] **Early bed**: the next night, sleeping before midnight shows "A good night's sleep. You're no longer exhausted." and the HUD reads "Stamina 100 / 100" without "Exhausted".
- [x] **All night**: staying up until 06:00 starts the next day where the farmer stands, with "You stayed up all night. Day N begins." and the exhausted message. Nothing jumps: the sun rises as the new day starts.
- [x] **Saving**: go to bed late, then stop and play again: the HUD still says "Exhausted", with 75 stamina.
- [x] **Feel**: say whether 10 per game hour pushes you toward bed without feeling harsh, and whether 75% is enough of a cost. Both are settings on `BP_PlayerCharacter`'s **Rest** component (Details panel, Farming > Rest), as are the warning and late-night hours.

### Holding E, 2026-11-02

Holding E keeps using the tool: the next use starts when the last one has finished or when the framed tile changes, and only if it would change that tile. The tests cover the rule; these checks are for the feel. The commit is the one titled "Repeat the tool use while E is held".

- [x] **A row**: with the hoe, hold E and walk along a row of grass. Every tile you pass is tilled, one "Tilled the soil." message each, and walking doesn't need to stop.
- [x] **Quiet skips**: hold E with the hoe while standing still facing a tile you just tilled. Nothing more happens and no message repeats. Walk on with E still held, and the next grass tile is tilled.
- [x] **The press says why**: press E once facing tilled soil with the hoe. "Only grass can be tilled." shows once, not again while you hold E.
- [x] **Release**: let go of E halfway along a row. The use under way finishes, and no tile after it changes.
- [x] **Watering and planting**: holding E with the watering can waters a row of tilled soil, and with the seeds plants a row until the seeds run out, then stops quietly.
- [x] **Speed**: say whether walking with E held outruns the 0.4 s use (tiles left behind unworked). If so, the use time or the walk speed needs tuning.

### Slower walk while holding E, 2026-11-03

After David's play-test: the farmer walks at half speed while E is held, so a row is worked evenly. The tests check the speed; these checks are for the feel. The commit is the one titled "Walk at half speed while E is held".

- [x] **Slower**: holding E while walking along a row moves at about half speed, and every tile of the row is worked. Letting go brings back the normal walk at once.
- [x] **No E, no change**: walking without E, and jumping or falling with E held, feel as before.
- [x] **Feel**: say if half is too slow or not slow enough. It is Tool Use Speed Scale on `BP_PlayerCharacter`'s **CharacterMovement** component (Details panel, Farming > Movement).

### No tools in the air, 2026-11-03

After David found that jumping got round the slower walk: tools don't work while the farmer is in the air, a held E carries on when he lands, and a jump with E held is slowed like the walk. The tests cover the rules. The commit is the one titled "Keep tools on the ground".

- [x] **Jumping**: hold E with the hoe and jump along a row of grass. No tile changes while he is in the air, and the tile he faces when he lands is tilled.
- [x] **No speed-up**: with E held, jumping along the row is no faster than walking it.
- [x] **Press in the air**: press E once during a jump without holding it. Nothing happens, and no message shows.

### Tile highlight material from the Blueprint, 2026-11-03

The frame's material, `M_TileHighlight`, is now set on `BP_PlayerCharacter`'s highlight component (by script) instead of in C++. The test checks the Blueprint has it; these checks are for how it looks. The commit is the one titled "Set the tile highlight material on the player Blueprint".

- [ ] **Asset**: open `Content/Vestige/Blueprints/BP_PlayerCharacter`, select its **TargetHighlight** component: Highlight Material is `M_TileHighlight` (Details panel, Farming > Visuals). The Blueprint doesn't ask to be saved when you close it.
- [ ] **Frame**: in Play in Editor the frame around the target tile is yellow and unlit as before, also at night, and the Output Log has no "No Highlight Material" warning and no "missing usage flag".

### Details categories, 2026-11-03

The general types' settings moved out of the `Farming` categories into ones named for their topic; the farm types (tiles, grid, crops, the farming component and the highlight) keep `Farming`. Only display names changed, so nothing needs saving. The commit is the one titled "Name the general types' Details categories by topic".

- [ ] **Day cycle**: the `VestigeDayCycle` in `L_Farm` shows its settings under **Time** and **Lighting** (Night Post Process too).
- [ ] **Player Blueprint**: in `BP_PlayerCharacter`, Move Ramp Seconds is under **Input** with the template's input actions, the **Rest** component's hours and drain under **Rest**, and the **CharacterMovement** component's Tool Use Speed Scale under **Movement**.
- [ ] **Assets**: a tool data asset shows **Content**, **Tool** and **Visuals**; an item shows **Content**, **Item** and **Economy**; a crop shows **Content** and **Farming** with Growth, Harvest and Visuals.
- [ ] **Blueprint menus**: in any Blueprint's graph, right-click and search: Get Day Cycle is under **Time**, Get Content Id under **Content**. In Add Component, the Vestige Rest component is listed under **Rest**.

### Test keys as console commands, 2026-11-04

F9 (new game) and N (skip day) are no longer keys: they are the console commands `NewGame` and `SkipDay`, which only development builds have. The tests check the commands exist on the game's player controller and that `SkipDay` starts the next day; these checks are for the console and the keys. The commit is the one titled "Move the test keys F9 and N into a cheat manager".

- [ ] **Key hints**: the bottom line ends "... | E Use tool | F Interact", with no N or F9. Pressing N or F9 in the game does nothing (F9 may still take an editor screenshot).
- [ ] **SkipDay**: open the console ([Before you test](#1-before-you-test)), type `SkipDay` and press Enter. The next day starts as it does in the bed: "You slept. Day 2 begins." and "Game saved." The console's suggestions should list `SkipDay` and `NewGame` as you type them.
- [ ] **NewGame**: back up `Saved/SaveGames/` first. Type `NewGame`: the map reopens on day 1 at 06:00 with the starting bag.

### Save versions restart, 2026-11-04

The save upgrades are gone and the save version starts again at 1, so your current farm (a version 4 save) can't be loaded any more. The tests cover what loading decides; these checks are for the real files. The commit is the one titled "Delete the old save versions and their upgrades".

- [ ] **Old save refused**: before deleting anything, press Play. The game says "The save is from a newer version of the game. It is kept as it is, and the game won't save until you start a new game." and the Output Log has a `LogVestigeSave` error naming slot `Farm` and version 4. Sleeping doesn't save.
- [ ] **Delete the development saves**: run the console command `NewGame` (it deletes `Farm.sav` and `Farm2.sav`), then stop the game and delete the old upgrade backups `Farm_Version0.sav`, `Farm_Version2.sav` and `Farm_Version3.sav` from `Saved/SaveGames/` by hand. Nothing makes backups any more.
- [ ] **New save round trip**: play from day 1, plant and water a tile, sleep ("Game saved."), stop and play again: "Welcome back. Day 2." with the bag, the tool in hand, the stamina and the tile as you left them. `Saved/SaveGames/` holds only `Farm.sav` (and `Farm2.sav` after a second night).

### Editor chores

Changes only the editor can make safely. The steps are in [Roadmap.md, Housekeeping](Roadmap.md#3-housekeeping): re-saving the template Blueprints and `L_Farm`, moving `BP_FarmTile` and the materials with redirectors fixed, and regenerating the Android File Server token if it was ever used. After each one, run the [smoke test](#3-smoke-test).

## 3. Smoke test

About five minutes in `L_Farm`, in Play in Editor at High. Run it before pushing and at the end of each milestone, and after any editor chore. Back up `Saved/SaveGames/` first.

1. **Start:** a new game (the console command `NewGame`) starts on day 1 at 06:00. The HUD shows the day, the clock, the tool, the stamina ("Stamina 100 / 100"), the bag ("Bag: Turnip Seeds 10") and the key hints; the Output Log has no `LogVestige` errors.
2. **Tools:** with each tool on the tile in front of you: the hoe (1) tills grass, the seeds (2) plant in tilled soil and use a seed, the watering can (3) darkens the soil, and the hand (4) does nothing on an unripe crop. The tool in hand changes with each key. Each action shows a short message, and each one that changes the farm lowers the stamina; facing away from the field says "Stand facing a farm tile." and costs nothing.
3. **Growing:** water a planted tile and sleep in the bed (walk up to it, F), a few times; the console command `SkipDay` skips a day the same way. The crop grows one stage per watered night, and a tile left dry doesn't grow. Harvest the ripe crop with the hand; a Turnip stack appears in the bag or grows. Each night in bed refills the stamina.
4. **Day and night:** let the clock run through an evening (it moves about one hour every 40 seconds unless the level's day cycle is set faster). The sun sets with warm light, the night is properly dark with faint moonlight, and at 02:00 the farmer passes out into the next day.
5. **Saving:** sleep in the bed ("Game saved."), stop the game and play again. The day, the bag, the tool in hand and every tile, crops included, come back.
6. **Highlight:** walk along the field edge and turn. The frame is bright yellow, by day and at night, follows the tile in front of you and disappears off the field.
7. **Performance:** in the console, `stat fps` and `stat unit`. In a busy part of the farm, by day and by night, the frame stays under 16.6 ms (60 fps), and sleeping shows no visible hitch ([Architecture.md, section 10](Architecture.md#10-performance-by-design)). Play in Editor adds its own cost, so check a doubtful number in Standalone Game. For a hitch on the new day, start the game with `-trace=default` and open the trace in Unreal Insights (`Engine\Binaries\Win64\UnrealInsights.exe`): `AVestigeDayCycle::StartNextDay` shows the whole new day, with the tiles, the save and the load inside it.

Put the save folder back afterwards if you want your farm as it was.

## 4. What isn't manual

- **Game rules are tested automatically**, by the `Vestige` automation tests: tile states and growth, tools and seeds, stamina (costs, the zero floor, the slower uses and the refill), the inventory's stacking and saving, the clock, sleeping and passing out, the save rules, content IDs, the include direction between source folders and the rendering settings. A rule is never covered by a manual check alone. If a check here ever finds a rule broken, the fix comes with a test that would have caught it.
- **This document is for what tests can't judge:** how the game looks and feels, lighting, the HUD as you see it, input, saves on real files across sessions, performance, and steps that need the editor.
- **When a change needs a manual check**, the commit that makes it adds a line to [Pending checks](#2-pending-checks) under a dated heading, with the commit and what to look for. A system that gains lasting behavior also gets a line in the [smoke test](#3-smoke-test).
