# Roadmap suggestions

Exploratory suggestions for the current and future roadmap. These are not approved changes to
[Roadmap.md](Roadmap.md), scope, or design; they describe things to evaluate when the relevant
milestone is planned.

The priorities below are advisory. "Now" means that the item should be settled before nearby
Milestone 2 work. "Milestone 3" means it should shape the first post-slice milestone. "Later" means
it should wait until the named systems exist and have been playtested.

## Summary

| Priority | Suggestion | Recommendation |
|---|---|---|
| High | Vertical-slice onboarding order | Hob and interface feedback before automation expansion |
| High | Earlier slice playtest | Test comprehension as soon as the season, shop and relationship loop are playable |
| High | Town objectives and journal | Add a small, data-driven goal structure before town restoration |
| High | Town loop milestone | Prove travel, one resident, requests and one restoration before energy systems |
| High | Resident calendar and availability | Share a calendar data model across residents, festivals and events |
| High | The game's hook in the slice | Own player model, one finished corner, readable mutations and the cryo-pod opening before the playtest |
| High | Feel of the core actions | Animations and small effects for tilling, planting, watering and harvesting, played through gameplay cues |
| High | Hard-to-undo choices | Decide a handheld target and the languages before the HUD layout pass |
| Medium | World map and landmarks | Add travel information when the town and wasteland become separate maps |
| Medium | One gathering loop | Start with foraging or scavenging rather than a broad fishing/mining ecosystem |
| Medium | Storage and processing | Add the minimum storage and one meaningful processing loop before crafting grows |
| Medium | Collection and display | Add a low-cost collection reward after gathering and exploration exist |
| Medium | Festival and season events | Add one complete festival before multiplying the calendar |
| Medium | Cooking or meal effects | Build one meal loop when survival needs are real, not merely planned |
| Medium | Common genre features | Record in, later or out for a character creator, pet, fishing, crop quality, house upgrades, mailbox, time pace and photo mode |
| Lower | Animals | Keep deferred until farming, seasons, buildings and economy are stable |
| Lower | Fast travel | Decide only after map walking and map size have been playtested |
| Lower | Achievements | Add after stable save and progression identifiers exist |

---

## 1. Reorder vertical-slice onboarding ahead of automation

**Priority:** High; before the rest of the remaining Milestone 2 order is finalized.

**Roadmap relation:** [Where we are](Roadmap.md#1-where-we-are),
[The caretaker robot](Roadmap.md#44-the-caretaker-robot),
[Interface and input](Roadmap.md#46-interface-and-input),
and [Automation and selling](Roadmap.md#45-automation-and-selling).

### What is missing

The vertical-slice plan already has all of the major systems needed for its playtest, but Hob,
the hotbar, the bag and relationship feedback are spread across the end of the current plan. An
automation unlock is therefore scheduled before the player has been fully taught to read the field,
inventory and the caretaker's changing opinion.

### Recommended order

1. Finish the hotbar and bag on screen.
2. Complete Hob's dialogue box and character data.
3. Make Hob's relationship changes visible in lines and actions.
4. Add Hob's first-days tutorial and crop-risk reminders.
5. Complete the HUD layout pass.
6. Build and playtest placing, the sprinkler and dirty water.
7. Rework the slice before cosmetic and optional polish.

This does not mean the sprinkler should be removed from the slice. It means systems that determine
whether a first-time player understands the slice should be ready for the first packaged slice
playtest.

### Why it matters

A player can fail the slice for a presentation reason while the underlying systems are working:

- They do not know why a tile is dry or contaminated.
- They cannot find seeds or the item currently in hand.
- They do not realize that Hob is responding to their actions.
- They do not notice the sprinkler saving work.
- They miss the cost of staying up late.

The playtest should discover these understanding problems before more automation and asset polish
make that feedback more expensive to change.

### Pick-up scope

Create one onboarding milestone with:

- A first-new-game walk through the day's controls
- One visible inventory screen and one row of hotbar slots
- One relationship state that is visible through a Hob line or action
- A clear tutorial for till, plant, water, harvest, ship and sleep
- One HUD layout that has room for all required status

The first design must also identify which instruction belongs in Hob, the HUD, the prompt and
written item text. The solution should not repeat the same explanation in every one of those places.

### Acceptance criteria

- A person who has not seen the game can complete an ordinary first-day loop without watching
  someone else play.
- The player can identify the item in hand and find the seed or tool they need.
- The player can explain why a crop is dry or contaminated after one or two prompts.
- Hob's opinion changes for a reason the player can see.
- The player understands the effect of a late night before the next scheduled playtest.

---

## 2. Run the first external slice playtest earlier

**Priority:** High; as soon as the vertical-slice loop is coherent.

**Roadmap relation:** [Starting and testing the slice](Roadmap.md#47-starting-and-testing-the-slice).

### What is missing

Automated tests and internal checks can verify rules, but they cannot tell whether the first-time
experience is understandable, satisfying or worth continuing. The roadmap already specifies a
packaged build and outside players, but it should happen before optional polish becomes a long
detour.

### Recommended approach

Use one or two short external playtests rather than one large final test:

1. A control-comprehension test after the title screen, HUD and Hob are usable.
2. The full-season test already listed by the roadmap.

The first test can be only a setup and day-one test. The second should cover the full season goals
and collect what the player did rather than asking the tester to narrate what they think they did.

### Details to test

Test a sheet with only measurable outcomes:

- Did the player notice dry or contaminated soil?
- Did they fix one of those conditions?
- Did they make a meaningful crop or water choice?
- Did they notice Hob's opinion change?
- Did they buy or place the sprinkler?
- Did they stay up late once?
- Did they understand the next-day cost?
- Did they finish the season wanting another one?

Do not write a manual test for a rule that automated tests can check. Manual checks remain for
clarity, fun, comfort and whether the whole loop feels good.

### Roadmap impact

The playtest should produce only a short set of new roadmap items. Those items are the follow-up
bugs or design changes found by real players, not a return to a broad feature backlog.

---

## 3. Add a town objective and journal system

**Priority:** High; design before Milestone 3 town restoration begins.

**Roadmap relation:** [The robot town](Roadmap.md#the-robot-town),
[Town restoration](Roadmap.md#the-robot-town), and
[Progression](Roadmap.md#progression).

### Why it is needed

The first release should make town restoration a clear long loop. The current design names facilities
to repair and specific items to donate, but it does not specify how the player sees which objective
is active, what is required, or what has already been delivered.

Without a shared objective model, town restoration could become a collection of one-off
interactions, screens or dialogue clauses. That would be difficult to save, translate, test and
extend.

### Recommended minimum system

A small objective model should support:

- A stable objective ID
- A title and short description
- A source character or facility
- Required items or actions
- Current and target quantities
- Prerequisite objectives
- Completion state
- A reward or unlocked feature
- A visible active objective
- A completed-objective record

No special objective type should be added for every character or facility. The common model should
cover donations, requests and restoration work. Character dialogue and the world can present it,
but they should not own the objective rules.

### Player-facing behavior

The player should be able to answer three questions without guessing:

1. What should I do next?
2. What do I need to collect or deliver?
3. What will change when I finish?

The screen should not become a contract to complete every objective at once. At least the first
version should be deliberately narrow, with one highlighted objective and a short list of
facility progress.

### Relationship to the economy

Donation items must be tracked separately from normal sales. A delivered item should remain
delivered and should not be counted twice. The same item being needed for restoration and being
valuable for currency is a meaningful choice; the objective UI should show that choice only while
it is relevant.

### Acceptance criteria

- A required item updates immediately after pickup or delivery.
- Loading a save restores objective and delivered-item counts.
- Unknown or removed content does not corrupt old saves.
- The UI remains usable with a full inventory and with more than one pending objective.
- One data change can add a town objective without changing objective code.

---

## 4. Reorder Milestone 3 around a complete town loop

**Priority:** High; recommended before the first post-slice milestone is expanded.

**Roadmap relation:** [Milestone 3](Roadmap.md#5-milestone-3-and-later),
[Seasons and weather](Roadmap.md#seasons-and-weather),
[Energy and the land](Roadmap.md#energy-and-the-land),
and [The robot town](Roadmap.md#the-robot-town).

### Missing proof

The vertical slice proves farming, limited water and one relationship. It does not prove that the
player wants to spend a year restoring a town. Milestone 3 should therefore prove the town and
relationship loop before it proves complex energy and land-health simulations.

### Recommended Milestone 3 sequence

1. Separate farm and town maps, with loading and one travel method.
2. One memorable resident with a schedule and backstory.
3. Gifts, one request and one personal event.
4. A town objective for a complete facility repair.
5. Shops and the first proper store.
6. One season with a festival and changing stock.
7. Calendar and resident availability.
8. Energy and land-health simulations after the town loop has been measured.

This is a proof milestone, not the first full implementation of every future resident. The goal is
to validate the loop with one resident and one facility before creating a large cast.

### Why energy and land health should wait

Energy and land health are valuable late systems, but they can add substantial save state, world
state and tuning work. Starting with them would allow the project to spend work on a system before
the player has been shown a reason to care about town restoration.

A balanced post-slice result should answer:

- Is travel pleasant or tiring?
- Is one resident interaction worth seeking out?
- Does the town visibly improve after a donation or repair?
- Is the player choosing what to do between farming and town work?
- Does one festival change the player's plans?

Those answers should drive the quantity and cost of later systems.

### Acceptance criteria

- The player can travel between the farm and town and understand where they are.
- A player can repair one town facility using a clear objective.
- The repair changes the town and unlocks a visible benefit.
- The player can meet a resident and notice one schedule, gift or event response.
- The one complete loop can be tested independently of the wasteland.

---

## 5. Add a resident calendar and availability system

**Priority:** High for Milestone 3.

**Roadmap relation:** [Townsfolk](Roadmap.md#the-robot-town),
[Seasons and weather](Roadmap.md#seasons-and-weather),
and [Relationships](GameDesign.md#relationships).

### Missing structure

The design already calls for resident schedules, likes and dislikes, and personal events. The roadmap
does not describe shared calendar data or player-facing availability.

### Recommended model

Create data for:

- A character's scheduled location by time and day
- Weekly, seasonal and one-off schedule overrides
- Gift likes and dislikes
- Birthdays or maintenance days
- Festivals and other town events
- Personal-event prerequisites
- Relationship-state restrictions

Data for a special day should share the calendar model rather than being embedded in one
character, map or dialogue object.

### Player-facing behavior

The interface needs to answer:

- Is this character available now?
- What time or day will they usually appear?
- Is today's event or festival changing the normal schedule?
- Is a gift particularly welcome?
- Can the player see why an event is not available?

A full town map with resident locations may be enough for a first implementation. A formal
calendar UI can follow once enough residents and events make memory and guessing expensive.

### Avoid overbuilding

Do not add:

- One schedule type for each character
- Hidden weather-dependent schedules before weather exists
- A separate relationship rank system for every character
- Romantic schedule effects before romance is implemented

Those are easier to add once a shared schedule model is proven.

### Acceptance criteria

- A character follows the same schedule after a save/load.
- A schedule can change for a season or event.
- A player can find a character or determine that they should come back later.
- Gift and event behavior is driven by data.
- A missing content asset produces a clear unknown-content result rather than a schedule crash.

---

## 6. Add a world map, landmarks and first-release travel

**Priority:** Medium; during the first town/wasteland map work.

**Roadmap relation:** [Separate maps](Roadmap.md#the-robot-town),
[World and exploration](GameDesign.md#10-world-and-exploration),
and [Later still](Roadmap.md#later-still).

### Missing feature

The roadmap describes separate maps but does not define player-facing navigation, landmarks or a
way to plan travel. This becomes important once the game has several destinations and different
daily schedules.

### Recommended minimum

Implement a map that shows:

- The current map
- The player's current position
- Town, farm and unlocked destination entrances
- Landmarks and fixed facilities
- The route or loading target for a destination
- Whether a destination is currently available

The first map can be one static map for the current zone plus a simple world overview. It does not
need to reveal unexplored wasteland areas automatically.

### Fast travel decision

Fast travel should not be added by assumption. It should be decided after playtests show whether
walking:

- Teaches the player useful geography
- Feels pleasantly short
- Costs enough time to become a meaningful choice
- Is tedious with a regular daily schedule

If fast travel exists, it should be a named, limited or unlocked mechanic, not a global teleport
control with no relationship to stamina or game time.

### Acceptance criteria

- A player can identify the next destination from the map.
- Loading destinations and map markers use the same data.
- The current marker remains accurate after loading a save.
- Unknown or locked zones are distinguishable from discovered zones.
- Map navigation works on keyboard and gamepad.

---

## 7. Add one first-release gathering loop

**Priority:** Medium; after the first wasteland or town design is stable.

**Roadmap relation:** [Scavenging](Roadmap.md#combat-and-the-wasteland),
[Building and crafting](Roadmap.md#building-and-crafting),
and [World and exploration](GameDesign.md#10-world-and-exploration).

### Missing feature

The first release has several systems that need materials but no complete early gathering loop.
The roadmap includes scavenging and wasteland zones, but scavenging is not described as a coherent
activity with sources, quantities and repeated value.

### Recommendation

Start with a small foraging or scavenging loop. For example, the farm and closest wasteland could
produce one set of renewable resources and a smaller set of rare materials.

Use one gathering loop for several purposes:

- Crafting stations
- Town donations
- Tech-tree unlocks
- Resident requests
- House or farm decoration
- Occasional selling

Do not start with fishing and mining in the same milestone unless they are already central to the
world and have distinct player decisions.

### What it needs

- Named resources as data assets
- Clear source types
- Repeatable and rare sources
- A daily or event-controlled respawn rule
- Inventory capacity
- A relationship to stamina or tool use
- One set of crafting uses
- A map or weather rule only if the content needs it

### Why it is useful

A gathering loop gives the player a reason to leave the farm and visit other maps even before combat
or advanced survival is built. It also makes town restoration and crafting depend on exploration
instead of currency alone.

### Acceptance criteria

- The player can collect and identify at least three useful material types.
- Each material has at least one concrete use.
- The gathering source refreshes according to a documented rule.
- Resources work with saves and the existing inventory model.
- Unknown content from old saves or mods is skipped safely.

---

## 8. Add shared storage and a small processing loop

**Priority:** Medium; before the first crafting or machine expansion.

**Roadmap relation:** [Building and crafting](Roadmap.md#building-and-crafting),
[Economy](GameDesign.md#economy),
and [Inventory](Systems.md).

### Missing feature

The roadmap has storage and crafting as separate ideas, but it does not define whether crops, tools
and materials share inventory, remain in machines or need specific player-owned storage.

### Recommended first implementation

Implement:

- A few shared storage containers
- One stationary processing station
- One recipe or conversion that uses a crop and one gathered material
- A result that has more long-term value than the raw input
- A clear placed-object and prompt model

Examples of the kind of loop to prove, not committed designs, include:

- Preserving a harvested crop
- Turning contaminated material into a handled resource
- Refilling or repairing a machine
- Producing a fuel or soil item

The exact recipe should depend on the world and survival design. The technical target is a data-
driven station and recipe, not a one-off hard-coded workstation.

### Design guardrails

Processing must do more than double the currency of an item. It should create one of:

- A town donation
- A crafting ingredient
- A survival benefit
- A machine or automation resource
- A relationship or story reward

Without a differentiated use, processing becomes a second inventory menu for selling.

### Acceptance criteria

- Items can enter and leave storage without loss.
- A recipe can be removed or replaced by data.
- The station reports missing ingredients clearly.
- Processing time and completion are saved or deliberately reconstructed.
- The player can identify what a station is doing.

---

## 9. Add a first collection or museum-style reward

**Priority:** Medium, later than town and gathering.

**Roadmap relation:** [Town restoration](Roadmap.md#the-robot-town),
[Story](Roadmap.md#story),
and [World and exploration](GameDesign.md#10-world-and-exploration).

### Why it may fit

Cozy life-sims commonly give exploration, fishing and collection systems a permanent reward beyond
selling. It can provide a reason to remember rare finds, protect the world from repeated combat,
and make the town feel culturally valuable.

The Project Vestige identity could support this with a pre-war archive, a robot clinic exhibit or
a town record instead of a generic museum.

### Recommended minimum

Start with one collection category that has:

- 6 to 12 entries
- Two or three levels of completion
- A clear player-facing record
- A facility or character that rewards completion
- A reward that does not replace progression tracks

Possible collections should be chosen only after content exists. Records, found text, rare crops,
pre-war tools and location discoveries are candidates. A fish codex or animal list may come later.

### Guardrails

Collection completion must not be required to buy core tools, repair essential systems or advance
the main town. It can unlock lore, decoration, a shop item or a local feature, but it should remain
optional.

### Acceptance criteria

- Discovering an item registers it once.
- The collection survives saves and does not duplicate entries.
- Unknown content is listed or ignored safely.
- One completion reward is visible and useful.
- The collection does not conflict with relationship rewards or core progression.

---

## 10. Implement one complete festival before expanding seasons

**Priority:** Medium; first Milestone 3 season.

**Roadmap relation:** [Seasons and weather](Roadmap.md#seasons-and-weather),
[Townsfolk](Roadmap.md#the-robot-town),
and [Seasonal loop](GameDesign.md#seasonal-loop-28-days).

### Missing milestone

The current roadmap includes one festival per season, but a first implementation could become a
collection of season-specific events rather than a reusable calendar, dialogue and schedule system.

### Recommendation

Build one complete festival with:

- A schedule and location
- Festival-specific dialogue
- An activity or minigame
- Resident attendance
- Rewards and relationship effects
- A clear change to normal shop and resident schedules
- One or two automated outcomes

Only after that festival works should the project add one festival to every season.

### The vertical-slice relationship

A full season must already have an end-state. The first season could use a simple recap or final
town message before a major festival system exists. The roadmap's end-of-season recap remains useful,
but it should not block the slice unless the external playtest shows the season-ending moment is
flat.

### Acceptance criteria

- The festival activates on the correct in-game date.
- Loading in or staying up late does not replay or skip it incorrectly.
- Normal schedules are overridden for the event.
- Event dialogue and actions use data.
- The complete event can be tested independently of the season crop loop.

---

## 11. Add one meaningful cooking or meal loop

**Priority:** Medium; only after eating and hunger have a real design.

**Roadmap relation:** [Survival needs](Roadmap.md#survival-needs),
[Farming](GameDesign.md#farming),
and [Needs](GameDesign.md#7-survival-needs).

### Missing feature

Eating crops is noted as part of survival needs, but the roadmap does not describe cooking or
conversion. A completed survival system could make crops more valuable than selling while giving
the player a reason to keep producing and preserve food.

### Recommendation

After crops and needs are implemented, add one cooking mechanic rather than a large recipe list:

- A cooking station
- 6 to 12 recipes
- Crops and gathered materials as ingredients
- One or more needs or stamina effects
- A way to inspect safety or contamination
- One recipe-discovery source

Cooking should support the design. It should not become the second economy or a hidden requirement
for progress.

### Safety requirement

Food that can expose the player must clearly identify that risk before consumption. Crops used to
clean soil must never be presented as edible, composted or burned. A contamination or food-safety
state belongs in item and crop data rather than in player UI alone.

### Acceptance criteria

- A recipe is data-driven and reports missing ingredients.
- Unsafe food is identified before use.
- Eating or cooking has the intended needs or stamina effect.
- The setting to turn survival needs off does not break farming or inventory.
- The meal economy can be balanced with both ordinary and poor seasons.

---

## 12. Keep animals as a later, separate milestone

**Priority:** Lower; after farming and town systems are stable.

**Roadmap relation:** [Later still](Roadmap.md#later-still),
[Building and crafting](Roadmap.md#building-and-crafting),
and [Scope and milestones](GameDesign.md#13-scope-and-milestones).

### What animals would require

Animals are a common life-sim feature, but they touch more than one system:

- Housing
- Daily care
- Feeding
- Product collection
- Breeding or rescue
- Schedules
- Animal-to-item and animal-to-save data
- Building placement
- Human and robot interaction
- Mutated variants
- Performance and world state

They should not be included as a "small feature" during the vertical slice or as a single item under
farming.

### Recommended rollout

When they become current work:

1. One animal species
2. One housing type
3. Daily care and one product
4. One sickness or environmental condition
5. Save and inventory support
6. Mutated livestock only after the normal animal loop is understood

Additional species and breeding should remain separate design questions.

### Why defer

Animal work has a high probability of expanding scope and touching save data before the core
experience is complete. It should remain a retained idea rather than a Milestone 3 feature unless
the game design is changed.

---

## 13. Do not assume fast travel is required

**Priority:** Lower; decide after maps have been playtested.

**Roadmap relation:** [Separate maps](Roadmap.md#the-robot-town),
[World and exploration](GameDesign.md#10-world-and-exploration),
and [Fast travel and saving?](GameDesign.md#15-open-questions).

### Recommendation

Fast travel is a user-experience decision, not an obvious requirement. Walking can teach landmarks
and time cost, but it can also turn the daily schedule into repeated travel.

The choice should be made after:

- Map sizes are known
- Stamina and clock costs are visible
- The town schedule is implemented
- One outside player has reported on map navigation

If fast travel is added, it should be explained by the world and should not silently remove a
planned part of the day.

---

## 14. Add achievements only after stable progression identifiers

**Priority:** Lower; no vertical-slice dependency.

**Roadmap relation:** [Progression](Roadmap.md#progression),
[Economy](GameDesign.md#economy),
and [Town restoration](GameDesign.md#9-progression).

### Missing feature

Many life-sims have achievement or collection tracking, but the project may not need an achievement
screen in the first release. Adding it early risks making every internal event an externally
visible requirement.

### Recommended scope

Wait until:

- Save identifiers are stable
- Town restoration states are complete
- Crops and items use namespaced IDs
- Relationship and automation milestones have clear state
- The main menu and settings system exist

The first achievement set can be limited to a few milestones. It should not duplicate every system
or create a second economy of rewards.

---

## 15. Defer or cut the following

These are not immediate recommendations and should not be moved onto the vertical-slice critical
path:

### Defer after Milestone 3

- Animals
- Additional camera angles
- Fishing and a full bug-catching ecosystem
- Full mod loading and a mod kit
- Co-op multiplayer
- AI voice acting
- Broad achievement infrastructure
- Multiple wasteland zones
- Full house and farm decoration
- Full combat skill progression

### Keep out of the critical path until the relevant proof

- One festival per season
- A complete collection
- A large recipe list
- A full tech tree
- Tool tiers
- Skill leveling
- Advanced survival needs
- Fast travel
- Advanced map travel

### Why they can wait

Each of these can be added after the game has a known loop and a stable data model. Bringing them
forward would test content variety before the project knows whether the core farming, town and
relationship loops are satisfying.

---

## 16. Show the game's hook in the vertical slice

**Priority:** High; before the slice playtest, since the slice is also the game's first public face.

**Roadmap relation:** [The caretaker robot](Roadmap.md#44-the-caretaker-robot),
[Starting and testing the slice](Roadmap.md#47-starting-and-testing-the-slice),
[Story](Roadmap.md#story),
[Before a public release](Roadmap.md#6-before-a-public-release)
and [Presentation](GameDesign.md#12-presentation).

### What is missing

The slice proves the farming loop, but little of what makes the game different shows in it yet. The
player is the engine template's mannequin, the art is the first low-poly pass, the cryo-pod opening
is a Story item under Milestone 3 and later, and a mutated crop still shows its normal ripe mesh
with a tint. Hob is already next up and is the right first step. Without the rest, screenshots,
clips and the playtest show a generic farm.

### Recommendation

- **Our own player character.** Bring the character part of the template replacement in
  [section 6](Roadmap.md#6-before-a-public-release) forward: a sleeper model in the game's style,
  with the template's animations retargeted to it (Unreal's IK Retargeter) until our own exist. The
  template's code stays where it is. A character creator is a separate decision (item 18).
- **One finished corner.** One part of the farm at the target art quality of
  [GameDesign.md, section 12](GameDesign.md#12-presentation): the overgrown pre-war ruins, warm
  light around the farm, contaminated soil, the cleaning crop and a properly dark night. It becomes
  the art target for the rest of the farm and the place for screenshots.
- **Mutation that reads at a glance.** Mutation is the mechanic that belongs to this world alone.
  Make the mutated ripe meshes that [the contamination guide](Systems/27-Contamination.md) already
  plans (`MutatedRipeMesh`, one per spring crop) and make them striking (oversized, twisted or
  faintly glowing), so a mutation reads in a still image and for colour-blind players. They wait on
  the open question on how often normal crops mutate in the slice
  ([GameDesign.md, section 15](GameDesign.md#15-open-questions), "Contamination in the slice, 4");
  how they look is @DunderGG's art-direction call.
- **The cryo-pod opening in the slice.** Move the Story item "The opening: waking from the cryo pod
  under the farmhouse" into the slice, as a short first minute that ends on the farm where the
  slice begins. It explains the world before the player touches a hoe. What happens in it is the
  writer's.

### Why it matters

The slice is what the first outside players see, and its screenshots and clips are the first public
image of the game. Farming loops are common; a waking sleeper, a robot caretaker and crops changed by
contamination are not. A playtest of a generic-looking farm also can't tell whether the world is
what draws players in.

### Acceptance criteria

- No template mannequin appears in the slice.
- One area of the farm is approved as the art target by the art reviewer and @DunderGG.
- If normal crops can mutate in the slice, a mutated ripe crop is recognisable as mutated in a
  greyscale screenshot from the game camera.
- A new game starts with the cryo-pod opening, which is short and can be skipped in later games.

---

## 17. Make the core farming actions feel good

**Priority:** High; before the slice playtest.

**Roadmap relation:** [Stamina and late nights](Roadmap.md#43-stamina-and-late-nights),
[Interface and input](Roadmap.md#46-interface-and-input),
[Before a public release](Roadmap.md#6-before-a-public-release)
and [Visual effects and feedback](Systems.md#15-visual-effects-and-feedback).

### What is missing

Tilling, planting, watering and harvesting are the actions a player repeats hundreds of times a
season, but they have no animation and no effect yet. The roadmap mentions them only in passing: a
use's time will come from its montage "when tools get animations" (4.3), ripe crops will glow (4.6),
and the template's animations are replaced before release. Sound waits for section 6, as @DunderGG
chose on 2026-11-22.

### Recommendation

- **An animation per action:** a montage for each tool use, played by the tool ability in place of
  its timed wait, as [the ability system guide](Systems/09-AbilitySystem.md) already plans, and
  timed so the effect starts when the tool meets the soil. It is also the montage that 4.3's use
  time comes from.
- **Small effects:** a puff of soil when tilling, a splash when watering, seeds dropping when
  planting and a burst of leaves when harvesting, as short Niagara systems.
- **A small camera shake and a light gamepad rumble** on the heavier actions only, each with a
  setting to turn it off.
- **On the path already designed:** a successful use's cosmetics come through a gameplay cue, and
  game code never spawns an effect ([the effects guide](Systems/15-EffectsAndFeedback.md); hard
  rule 9 in [AGENTS.md](../AGENTS.md)). One cue per action can play the effect, the shake, the
  rumble and, later, the sound together.
- **Sound stays deferred,** as decided on 2026-11-22. These four actions are the first place it is
  needed when it comes.

### Why it matters

How these four actions feel is most of how the game feels, because they fill most of a day. They
are also what short clips of the game would show. Animations and effects cost little next to a new
system, and the slice playtest can judge the loop's comfort only if the actions give feedback.

### Acceptance criteria

- Each of the four actions plays an animation and an effect, the effect through a gameplay cue.
- Nothing new ticks, and the effects are short-lived, pooled and given an effect type, as the
  effects guide asks, within the budget in [Architecture.md, section 10](Architecture.md#budget).
- Camera shake and rumble can each be turned off in the settings.
- The slice playtest sheet asks whether the actions feel satisfying.

---

## 18. Decide in or out on common genre features

**Priority:** Medium; decide before Milestone 3 is planned, and build only what is chosen, when its
milestone comes.

**Roadmap relation:** [Building and crafting](Roadmap.md#building-and-crafting),
[Later still](Roadmap.md#later-still),
[Farming](GameDesign.md#6-farming)
and [Open questions](GameDesign.md#15-open-questions).

### Missing decisions

Players of the genre expect several features that the design doesn't mention, or mentions only in
passing. Each needs an explicit in, later or out, so that a gap is a choice rather than an
oversight. [Item 15](#15-defer-or-cut-the-following) defers building some of them; this item asks
only for the decisions.

| Feature | Where it stands | Notes |
|---|---|---|
| Character creator | The lore makes the player customisable ([Lore.md](Spoilers/Lore.md), 2026-10-26), but no roadmap item builds it and GameDesign.md doesn't mention it | A simple first version (body, skin, hair, outfit colours) on the model from item 16 |
| A pet | No design or roadmap item; an open question in [GameDesign.md, section 15](GameDesign.md#15-open-questions) | One creature on the farm with a petting interaction and a name and no products, so far cheaper than livestock (item 12) |
| Fishing | No design or roadmap item; an open question in [GameDesign.md, section 15](GameDesign.md#15-open-questions) | Could be, or follow, the gathering loop of item 7 |
| Crop quality, with fertilizer | Fertilizer is in [GameDesign.md, section 6](GameDesign.md#tiles) with no roadmap item; quality is nowhere | The farming loop's main depth lever and a natural link to soil and land health; touches crop data and saves |
| Farmhouse interior and upgrades | The [HomeFarm plan](Levels/HomeFarm/README.md) reserves a farmhouse zone and leaves its rooms undesigned; the bed stands by the field until then | House upgrades are a long-term goal in the genre and the place for decorating |
| A mailbox | Not planned | A channel for requests (item 3), hints and letters, filled by the mail carrier already planned among the townsfolk ([The robot town](Roadmap.md#the-robot-town)) |
| A time-pace setting | Not planned | Day length, or time stopping indoors or in menus; fits *Your pace* ([GameDesign.md, section 2](GameDesign.md#2-pillars)) and changes a clock rule, so it needs tests |
| Photo mode | Not planned | A free camera, a hidden HUD, a paused world and a high-resolution screenshot; Unreal has no built-in one. After the slice |

### Recommendation

Record each decision in GameDesign.md with its reason, and give each feature that is in a roadmap
item in its milestone. The pet and fishing questions are prepared in
[GameDesign.md, section 15](GameDesign.md#15-open-questions) with options and a recommendation.

### Acceptance criteria

- Each feature in the table has an in, later or out in GameDesign.md, with a reason.
- Each feature that is in has a roadmap item in the milestone that will build it.

---

## 19. Make hard-to-undo choices early

**Priority:** High; before the HUD layout pass in
[Interface and input](Roadmap.md#46-interface-and-input).

**Roadmap relation:** [Interface and input](Roadmap.md#46-interface-and-input),
[Before a public release](Roadmap.md#6-before-a-public-release)
and [Budget](Architecture.md#budget).

### What is missing

Two choices get more expensive with every system built before them, and neither is recorded.

- **A handheld target.** The budget is set for a desktop PC with an RTX 2060 class GPU at 1080p
  High ([Architecture.md, section 10](Architecture.md#budget)). The game is meant to run at Low and
  Medium as well ([Scalability](Architecture.md#scalability)), but nothing below High has a budget
  or a look check. Many players of the genre play on handheld PCs. Being a target can mean anything
  from laying the interface out for one now and measuring on a device before release, to a handheld
  budget measured at every milestone; the options are in
  [GameDesign.md, section 15](GameDesign.md#15-open-questions). Either way it needs complete gamepad
  menus (input already gives every action a gamepad key) and text readable at 1280 by 800, set
  through the interface's DPI scaling. The lighting is the costly part: the project has no baked
  lighting, and Unreal 5.8's Medium level swaps Lumen for a cheaper mode while Low turns it off, so
  the farm and its dark nights must be checked at those levels while the art is still being made.
  Deciding late means reworking the lighting and the HUD.
- **Target languages.** [The localization guide](Systems/18-Localization.md) covers `FText` and the
  text gather, and the player's text is already `FText`, but no languages are chosen. German runs
  long, and Chinese and Japanese need a composite font with fallbacks, which changes the layout.
  The HUD layout pass should know the languages.

Two others are already settled and aren't repeated here. Co-op: single-player is the design target,
and co-op gets its own design milestone later ([GameDesign.md, section 2](GameDesign.md#2-pillars),
decision of 2026-10-28). Placement for any placeable thing:
[the placement guide](Systems/16-PlacingAndBuilding.md) makes placeables content with a footprint
and rules, and decorations instances, so the sprinkler in
[Automation and selling](Roadmap.md#45-automation-and-selling) should be its first user, not a
one-off.

### Recommendation

Decide the handheld target and the languages with @DunderGG before the HUD layout pass; both are
prepared as open questions in [GameDesign.md, section 15](GameDesign.md#15-open-questions).

### Acceptance criteria

- GameDesign.md records whether handhelds are a target and which languages the first release has.
- If handhelds are a target, the budget in Architecture.md, section 10, gains a handheld row and
  the performance baseline a matching measurement.
- The HUD layout pass's brief names the languages and, if chosen, the handheld resolution.

---

## Suggested sequence for future roadmap planning

The roadmap could be planned in this order without replacing its existing milestone structure:

1. **Finish and test the vertical slice**
   - Hob, visible relationship feedback, inventory, the choices of item 19, HUD, title screen, the
     hook and feel of items 16 and 17, and external playtest.
2. **Prove a complete town loop**
   - One map, one resident, one request, one personal event and one facility repair.
3. **Establish repeatable resident and calendar behavior**
   - Schedules, availability, gifts, festivals and a minimal objective journal.
4. **Add the second daily context**
   - A gathering or foraging loop, map travel and first storage/processing.
5. **Expand the season**
   - The remaining seasons, weather, festivals and more crops.
6. **Add long-term land and energy systems**
   - Simulation, solar power, machines, land health and haze.
7. **Expand the cast and story**
   - Additional residents, romance, humans and the family mystery.
8. **Expand infrastructure**
   - Combat, wasteland, crafting, survival needs, automation and tools.
9. **Add lower-priority depth**
   - Animals, collection, decoration, achievements and extra transport.

This sequence is based on the project's existing strengths: a farming loop, a contamination and
resource loop, a caretaker relationship and a robot town story. It recommends proving those loops
before adding systems that can multiply content or save state.
