# Contaminated soil: crop and item texts

> **Spoilers.** Light ones: the lore behind the farm's contaminated rows and where the cleaning crop's harvest goes.

The player-facing names and descriptions for the contamination slice ([Roadmap.md, 4.2](../../Roadmap.md#42-farming-depth); the design is [Systems/27-Contamination.md](../../Systems/27-Contamination.md), section "What the writer and the artist deliver"), and the lore they rest on. Written by the writer on 2026-11-22.

**What these texts are for:** the player should feel that bad soil is a problem with a cause, that the fix is ordinary farm work, and that a mutated crop is a pleasant surprise and not a warning.

## Status and how to enter them

- **Status: Ready** for both tables below: final wording, checked against the lore's food-safety rules ([Lore.md, section 4](../Lore.md#cinderium-dust-and-land-recovery)) and the item names already in the game.
- **Names** go into each asset's `DisplayName` (`UVestigeContentData`), as asset text.
- **Descriptions have no field yet.** No content asset has a description today, so the descriptions wait for one. When the field comes (a data asset field, so a design for the architect), these are the texts. Until then the implementer enters the names only.
- **Length.** The longest name in hand, "Sinkwort Seeds (5 days to ripen)" on the status line, is shorter than "Cabbage Seeds (10 days to ripen)", which already fits. "Buy Sinkwort Seeds (15)" and "Shipped: Bloom Cabbage ×1. The bin pays in the morning." are as long as lines the game already shows. Each description is at most about 150 characters, two sentences.
- **No numbers in the texts.** "Sells for more" rather than "twice", so a change to the price stays a data change.
- The content IDs are the design's and never shown. A name change renames no asset.

## The cleaning crop

The crop that grows only in contaminated soil and cleans its tile when harvested. **Sinkwort**: it sinks its roots into the dust and keeps what it takes.

| Content ID | Asset | Field | Text |
|---|---|---|---|
| `FarmCrop:Base.CleanupCrop` | `DA_Crop_CleanupCrop` | `DisplayName` | Sinkwort |
| `VestigeItem:Base.CleanupCrop` | `DA_Item_CleanupCrop` | `DisplayName` | Sinkwort Pod |
| `VestigeItem:Base.CleanupCrop` | `DA_Item_CleanupCrop` | Description | Sealed shut around what the root drew out of the soil. Not food: never eat, compost or burn it. Ship it, and the town's waterworks stores it sealed. |
| `VestigeItem:Base.CleanupCropSeeds` | `DA_Item_CleanupCropSeeds` | `DisplayName` | Sinkwort Seeds |
| `VestigeItem:Base.CleanupCropSeeds` | `DA_Item_CleanupCropSeeds` | Description | Sinkwort grows only in contaminated soil and draws the contamination up into its pod. Each harvest leaves the soil cleaner. |

- The pod's text says "Not food" and names all three things the lore forbids (eating, composting, burning), so no later text can treat it as food. It sells at the bin like any crop; the description says why that counts as contained.
- **For the artist** (no requirement, a fit with the name): a low, tough plant with a hard, closed seed pod at the top when ripe. The pod is the harvest, and a pod that stays shut is the lore's reason it can be stored safely.
- **For translators:** "Sinkwort" is an invented plant name ("wort" is an old word for a plant). Translate it as a plant name that suggests sinking in or holding fast, not as a literal compound.

## The mutated crops

Kept separate: @DunderGG may drop them from the slice ([GameDesign.md, section 15](../../GameDesign.md#15-open-questions), question 4). If he does, this table is unused and nothing above changes.

A normal crop that ripens mutated on lightly contaminated soil. The prefix **Bloom** is the same on all three, so the player learns the kind from the first one, and it reads as good luck: a flush of colour, not a sickness. The texts say "safe to eat" plainly, because the lore forbids any text that suggests a mutated crop is harmful from its look alone.

| Content ID | Asset | Field | Text |
|---|---|---|---|
| `VestigeItem:Base.MutatedTurnip` | `DA_Item_MutatedTurnip` | `DisplayName` | Bloom Turnip |
| `VestigeItem:Base.MutatedTurnip` | `DA_Item_MutatedTurnip` | Description | Ripened in contaminated soil and came up the wrong shape and the wrong colour. Tastes like a turnip, only more so. Safe to eat, and it sells for more. |
| `VestigeItem:Base.MutatedPotato` | `DA_Item_MutatedPotato` | `DisplayName` | Bloom Potato |
| `VestigeItem:Base.MutatedPotato` | `DA_Item_MutatedPotato` | Description | A potato that ripened in contaminated soil and grew in directions potatoes don't. Safe to eat, and it sells for more than a plain one. |
| `VestigeItem:Base.MutatedCabbage` | `DA_Item_MutatedCabbage` | `DisplayName` | Bloom Cabbage |
| `VestigeItem:Base.MutatedCabbage` | `DA_Item_MutatedCabbage` | Description | Its leaves curl the other way, and it squeaks less when you cut it. Grown in contaminated soil, safe to eat, and it sells for more. |

- The descriptions don't fix a shape ("the wrong shape", "directions potatoes don't", "curl the other way"), so the artist's meshes stay free. Only the cabbage's curled leaves is a detail; if its mesh shows something else, the line changes to match.
- **For translators:** "Bloom" means a flush of colour or a flowering, not a disease. Pick a positive word for it and use the same one on all three.
- If the mutation becomes question 4's option (b) instead (a stalled crop turns into Sinkwort), no new text is needed: the crop is Sinkwort.

## The lore behind them

Recorded in [Lore.md, section 11](../Lore.md#11-smaller-hooks) and the decision log; repeated here so the texts and their reasons stay together.

### What poisoned the far edge of the field

- **Confirmed history:** the far edge of the field runs along the drainage ditch at the farm's boundary, which carries runoff from the open ground beyond. For seventy years every radiation storm dropped dust on the whole farm, and the ditch gathered what the rain washed off the land past the boundary. When the ditch overflowed, it soaked the far rows. Contamination thins with distance from the ditch, so the rows nearest the house are clean, the middle rows lightly contaminated and the two far rows heavily.
- This is the lore's ordinary fallout behaviour (washout and runoff, [Lore.md, section 4](../Lore.md#cinderium-dust-and-land-recovery)), not a war site or a family event: the field's poison is the same weather that poisoned everyone's.
- The farm's well draws deep, from the aquifer, so its water stays clean. That is also why the town cares: runoff that reaches the ground water reaches the town's pump too.
- **For the level, later:** a ditch along the field's far boundary would show the cause without a word. Not needed for the slice.
- **Knowledge:** no character says this in the slice. The player sees it in the soil, and learns why the pods matter from the pod's description. Hob, whose memory is damaged, doesn't explain it, and nothing here says what Hob did or didn't do with the rows.

### Who buys the pods at the bin, and why it counts as contained

- **Confirmed history:** Sinkwort is a mutated plant that draws a mobile fraction of Cinderium compounds up through its roots and stores them in a hard pod that stays shut as it dries. Kept whole and sealed, the pod holds what it took ([Lore.md, section 4](../Lore.md#cinderium-dust-and-land-recovery), "What special crops do").
- **The buyer is Wellspring's waterworks** *(the town's working name; the item text says "the town's waterworks")*, the pump and treatment station the town grew around ([Lore.md, section 6](../Lore.md#towns-and-faction-hubs)). Contaminated runoff from the farms around it seeps toward the aquifer the town drinks from, so every pod taken off a field is dust that won't reach the pump. The waterworks stores the pods unopened in the station's sealed store below the treatment hall, built before the war for spent filter residue.
- **Why it pays at all, and the joke:** the waterworks still pays by the pre-war Civitas Soil Reclamation Schedule *(working name)*, a printed rate per sealed pod that nobody has revised in seventy years, so a pod is worth exactly what a Civitas board decided it was worth before the war. Its operators also keep the fourth seed dispenser stocked with Sinkwort seeds.
- **Left open:** who or what empties the bin overnight, for every crop. The pods travel with the rest; only their buyer is settled.
- **Food safety holds:** the pods are never eaten, composted or burned. Shipping them is containment because the buyer stores them sealed, not because the bin destroys them.

## Question for @DunderGG

**What lies past the far edge.** I settled it as ordinary weather and a ditch (A), and the texts don't depend on it. A or B are mine to decide, so I'm asking only because C would touch the family's fixed history:

- **A, a drainage ditch (assumed).** The poison is the world's weather. Quiet, believable, and it keeps the far edge from promising a story on day 1. A later line could read: "Every storm, the ditch fills, and the far rows drink it."
- **B, a war wreck in the ditch.** A burned-out war machine past the boundary, leaking into the runoff. More to look at, and a later wasteland hook, but it puts the war in the player's sight on the first morning. "Something big died in that ditch, a long time ago. The rows nearest it never forgave it."
- **C, the family.** Sealed drums of soil the parents lifted from the field after the war, buried at the far edge, now leaking. Moving, but it ties the field to how they were exposed, which the story saves for the late message scene. "They dug it out by hand, a barrow at a time. It had to go somewhere."

**Recommendation: A**, because the far edge is the first thing the player struggles with, and the family's story should arrive later, through Hob and the letters, not through the soil. B can come back as a wasteland site.
