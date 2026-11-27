# Hob: the trust track (slice)

> **Spoilers.** Hob's slice arc, and the first returning memory of the family.

The lines for Hob's trust track ([Roadmap.md, 4.4](../../Roadmap.md), "The caretaker robot"), written by the writer on 2026-11-27 on the game designer's recommended options ([GameDesign.md, section 15](../../GameDesign.md#15-open-questions), "Hob's trust track in the slice", all assumed until @DunderGG answers). They sit beside [Hob-Everyday.md](Hob-Everyday.md) (H-01 to H-03), whose rules for speakers, line IDs and length apply here too.

**What these lines are for:** the player sees Hob change because of how they treated Hob. Asking gets a view; telling gets "as instructed". The first change is surprise that Hob has a view at all; the second is Hob shyly wanting something and doing it; the payoff is one small, certain memory of the family that the cleaned field brings back.

## Status and how to enter them

- **Status: Ready** for every line in this file: final wording, voice and continuity checked. How the conditions below are coded is the architect's design; the order and priority of talks are the game designer's (section 15, questions 1, 4, 6 and 8). This file says only what each line needs to stay true.
- **Speaker:** HOB lines show Hob's display name (`VestigeCharacter:Base.Hob`). Reply lines are the player's and have no speaker.
- **Length:** at most 110 characters for a HOB line and 40 for a reply (English), as in H-01 to H-03; the longest of each are listed at the end of the file.
- **No placeholders.** Hob says no player name (the game has none, question 5).
- **No third-person pronoun for Hob** in any line. "Her" in H-07 is the sleeper's mother.
- **Item names in the text:** "Sinkwort" and "Bloom" are the display names in [Items/Contamination.md](../Items/Contamination.md); "Civitas" is a maker's working name ([Lore.md, section 7](../Lore.md#7-the-robot-makers)). A rename changes the lines that use them.
- **Line IDs** are stable keys; never reuse or renumber one.

## Knowledge

- **The player** in the slice knows only H-01 to H-03 and the farm. Until H-07 nobody has said there was a family; nothing here mentions the pod, the war, the time asleep or the parents' fate.
- **Hob** knows the farm, the order, and what the player has done on it. Hob never explains why the far rows are spoiled (the ditch stays unsaid, [Items/Contamination.md](../Items/Contamination.md#what-poisoned-the-far-edge-of-the-field)) or what Hob did or didn't do with them.
- **H-07** tells the player one thing: the sleeper's mother worked this field and taught Hob how to tell when soil is ready. Not where she is or what happened to her.

## Where the lines came from

@DunderGG asked (2026-11-27) whether these come from the phase drafts. The phase drafts are the full game's opening and early scenes, set after the cryo-pod wake-up; the slice has neither, so their lines can't be dropped in whole, but these were adapted from them:

| Line here | Adapted from | Change |
|---|---|---|
| `Hob.Step.Attentive.020A` | [O-06](Phase-01-Opening.md#o-06---a-home-not-a-mission): "Some people have requests. Some have questions. Those aren't the same as orders." | Turned to the player's questions to Hob |
| `Hob.Step.Wish.020` | [E-02](Phase-02-Early.md#e-02---hobs-next-purpose): "For seventy years, an order filled every hour..." "I would like to choose what grows there." | No "seventy years" (the slice doesn't say it); the empty hours go to the player's rows |
| `Hob.Step.Wish.030` | [O-02](Phase-01-Opening.md#o-02---first-work): "I would like to clear the east bed with you. If that's all right." | The watering offer, same shy ending |
| `Hob.Step.Wish.Answer.Yes` | E-02: "A morning walking the aqueduct won't rust my joints." | Cans at first light instead of the aqueduct |
| `Hob.Topic.FarRows.Answer.Ask` | [O-01](Phase-01-Opening.md#o-01---waking): "We can start with the next thing, not the whole world." | The row, not the field |
| `Hob.Topic.Well.Answer.Leave` | [O-05](Phase-01-Opening.md#o-05---first-family-trace): "Then not today." | Unchanged |
| `Hob.Step.Memory.040`, `Hob.Step.Memory.Answer.Else` | O-05: "I won't fill the gap with a guess." | Same promise, new memory |

Every other line is new: the phase drafts have no farm topics, greetings or watering lines, and their family memories (O-05's laughter, E-11's kitchen notes) are later traces that this first memory must not use up.

## H-04 - Farm topics

**When:** in place of the day's greeting, on the first talk of a day, as the designer's rules choose (question 1). Each topic plays once. A topic ended with Escape before a reply comes back on a later day.
**For:** the player gets a small, useful farm thought from Hob and a choice between asking what Hob thinks and telling Hob what to do. Every reply gets a true, useful answer; only the ask reply raises trust.

**How each topic runs:** the opening line, then the reply list with the replies in the order given (the ask reply is not always first, so confirming the highlight doesn't always ask), then that reply's answer. The talk ends after the answer.

- **Ask** (+1 trust): Hob's own view. The same line at every step.
- **Order:** before the attentive step (H-05), Hob's dutiful "As instructed" answer; from the attentive step on, the **Order, own view** line replaces it, adding Hob's view unasked. No trust change.
- **Leave:** a short, kind answer, the same at every step, with no view pressed on a player who declined. No trust change.
- One topic (Bloom) has no order reply.

**When each topic may come up** (plain words; the designer sets the rule):

| Topic | Available when | Why |
|---|---|---|
| Well | from day 2 | always true |
| Dispensers | from day 2 | always true |
| Planting | the player has planted at least one seed | the line says so |
| Harvest | the player has harvested a crop at least once | the line says so |
| Bin | the bin has paid at least once | the line says so |
| FarRows | from day 2 | the far rows are spoiled from a new game's start |
| Sinkwort | the player has planted Sinkwort at least once | the line says it is growing |
| Bloom | the player has harvested a Bloom crop at least once | the line says so; skipped if Bloom crops are cut from the slice |
| SeasonEnd | spring, day 19 to day 25 | from day 19 a cabbage can't ripen in time; after day 25 a turnip can't either |

Seven topics can come up before day 16 (eight with a Bloom), so the designer's pace (question 7) holds; SeasonEnd is a late extra.

### Well

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.Well.Open` | HOB | A report on the well. It draws deep and runs clean, but it gives only so much a day. |
| 2a | `Hob.Topic.Well.Reply.Order` | PLAYER | Keep the jug inside full. |
| 2b | `Hob.Topic.Well.Reply.Ask` | PLAYER | How would you share it out? |
| 2c | `Hob.Topic.Well.Reply.Leave` | PLAYER | Not today. |
| 3 | `Hob.Topic.Well.Answer.Ask` | HOB | Planted soil first. Bare ground can wait. It isn't going anywhere. |
| 3 | `Hob.Topic.Well.Answer.Order` | HOB | As instructed. The jug will be full. |
| 3 | `Hob.Topic.Well.Answer.OrderView` | HOB | As instructed. And if I may: planted soil first. Bare ground isn't going anywhere. |
| 3 | `Hob.Topic.Well.Answer.Leave` | HOB | Then not today. |

### Dispensers

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.Dispensers.Open` | HOB | The seed dispensers are old Civitas machines. They still take coins. Nobody has told them to stop. |
| 2a | `Hob.Topic.Dispensers.Reply.Ask` | PLAYER | Which seeds would you buy? |
| 2b | `Hob.Topic.Dispensers.Reply.Leave` | PLAYER | I'll have a look myself. |
| 2c | `Hob.Topic.Dispensers.Reply.Order` | PLAYER | Keep an eye on them for me. |
| 3 | `Hob.Topic.Dispensers.Answer.Ask` | HOB | Turnips, to start. They come up quickest, and a quick crop is good for the spirits. |
| 3 | `Hob.Topic.Dispensers.Answer.Order` | HOB | As instructed. They don't do much, but I'll watch them do it. |
| 3 | `Hob.Topic.Dispensers.Answer.OrderView` | HOB | As instructed. If you're buying, I'd start with turnips. A quick crop is good for the spirits. |
| 3 | `Hob.Topic.Dispensers.Answer.Leave` | HOB | Of course. They're very patient machines. |

- "Turnips come up quickest" is true of the slice's three food crops (3 nights against 6 and 10); Sinkwort (5) isn't food and isn't meant.

### Planting

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.Planting.Open` | HOB | You've planted something. The field hasn't had a new seed in a long while. |
| 2a | `Hob.Topic.Planting.Reply.Order` | PLAYER | Watch the rows while I'm out. |
| 2b | `Hob.Topic.Planting.Reply.Leave` | PLAYER | Let's see how it goes. |
| 2c | `Hob.Topic.Planting.Reply.Ask` | PLAYER | What do you make of it? |
| 3 | `Hob.Topic.Planting.Answer.Ask` | HOB | I think it looks like a beginning. I didn't expect to have a view on a row of seeds. |
| 3 | `Hob.Topic.Planting.Answer.Order` | HOB | As instructed. I'll watch them from the door. |
| 3 | `Hob.Topic.Planting.Answer.OrderView` | HOB | As instructed. I'll watch them from the door. For what it's worth, it looks like a beginning. |
| 3 | `Hob.Topic.Planting.Answer.Leave` | HOB | We'll see. Seeds are patient. |

### Harvest

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.Harvest.Open` | HOB | You brought in the first crop. I watched from the door and didn't know what to do with my hands. |
| 2a | `Hob.Topic.Harvest.Reply.Leave` | PLAYER | It's only the first. |
| 2b | `Hob.Topic.Harvest.Reply.Ask` | PLAYER | What did you think, watching? |
| 2c | `Hob.Topic.Harvest.Reply.Order` | PLAYER | Keep count of the harvests. |
| 3 | `Hob.Topic.Harvest.Answer.Ask` | HOB | That the field was glad of it. A field can't be glad, I know. That's what I thought. |
| 3 | `Hob.Topic.Harvest.Answer.Order` | HOB | As instructed. I'll keep count. |
| 3 | `Hob.Topic.Harvest.Answer.OrderView` | HOB | As instructed. I'll keep count. I'll also say it was good to watch. |
| 3 | `Hob.Topic.Harvest.Answer.Leave` | HOB | The first is the one I'll remember. |

- No answer gives a count, so the topic stays true however many days after the first harvest it plays.

### Bin

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.Bin.Open` | HOB | The bin paid out overnight. I didn't see who emptied it. I never have. |
| 2a | `Hob.Topic.Bin.Reply.Ask` | PLAYER | What should we do with the coins? |
| 2b | `Hob.Topic.Bin.Reply.Order` | PLAYER | Don't worry about the bin. |
| 2c | `Hob.Topic.Bin.Reply.Leave` | PLAYER | Coins are coins. |
| 3 | `Hob.Topic.Bin.Answer.Ask` | HOB | More seed, I think. Coins in a purse don't grow. |
| 3 | `Hob.Topic.Bin.Answer.Order` | HOB | As instructed. I'll try not to wonder about it, too. |
| 3 | `Hob.Topic.Bin.Answer.OrderView` | HOB | As instructed. Though if you want my view, spend it on seed. Coins in a purse don't grow. |
| 3 | `Hob.Topic.Bin.Answer.Leave` | HOB | They are. Small brass ones, shaped like gears. |

- "The bin paid out overnight" is true on any later day too: it did, at least once. "I never have" keeps who empties the bin open ([Lore.md, section 11](../Lore.md#11-smaller-hooks)).
- The coins are the pre-war brass gear scrip ([Lore.md, section 8](../Lore.md#8-the-currency)); the line names no currency, so the open coin name changes nothing.

### FarRows

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.FarRows.Open` | HOB | The far rows are spoiled. Nothing ordinary will grow at the very edge. |
| 2a | `Hob.Topic.FarRows.Reply.Order` | PLAYER | Leave them be. |
| 2b | `Hob.Topic.FarRows.Reply.Ask` | PLAYER | What would you do about them? |
| 2c | `Hob.Topic.FarRows.Reply.Leave` | PLAYER | Another day. |
| 3 | `Hob.Topic.FarRows.Answer.Ask` | HOB | Start with the next row, not the whole field. One row at a time is how anything comes back. |
| 3 | `Hob.Topic.FarRows.Answer.Order` | HOB | As instructed. They'll keep. |
| 3 | `Hob.Topic.FarRows.Answer.OrderView` | HOB | As instructed. Though I think one row at a time would bring them back. |
| 3 | `Hob.Topic.FarRows.Answer.Leave` | HOB | Another day, then. They've waited this long. |

- "Nothing ordinary will grow at the very edge" is the heavy soil's rule (normal seeds are refused there); Sinkwort is not ordinary, so the line stays true once the player knows it. It doesn't say why the rows are spoiled.

### Sinkwort

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.Sinkwort.Open` | HOB | That's Sinkwort out there. It keeps what it pulls from the soil, and the pod stays shut. |
| 2a | `Hob.Topic.Sinkwort.Reply.Leave` | PLAYER | It's a weed with a job. |
| 2b | `Hob.Topic.Sinkwort.Reply.Order` | PLAYER | Keep the pods away from the kitchen. |
| 2c | `Hob.Topic.Sinkwort.Reply.Ask` | PLAYER | Is it worth the trouble? |
| 3 | `Hob.Topic.Sinkwort.Answer.Ask` | HOB | I think so. It doesn't sell for much. It gives the soil back, a pod at a time. |
| 3 | `Hob.Topic.Sinkwort.Answer.Order` | HOB | As instructed. No pod comes past the door. |
| 3 | `Hob.Topic.Sinkwort.Answer.OrderView` | HOB | As instructed. No pod comes past the door. I'm glad it's growing, all the same. |
| 3 | `Hob.Topic.Sinkwort.Answer.Leave` | HOB | A good job, though. |

- Consistent with the pod's text: not food, sealed, sold for little.

### Bloom

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.Bloom.Open` | HOB | That one you picked came up a Bloom. Odd to look at, safe to eat, and it sells for more. |
| 2a | `Hob.Topic.Bloom.Reply.Leave` | PLAYER | Odd-looking, but fine. |
| 2b | `Hob.Topic.Bloom.Reply.Ask` | PLAYER | Would you eat one? |
| 3 | `Hob.Topic.Bloom.Answer.Ask` | HOB | I don't eat. If I did, I'd start with that one. It looks interesting. |
| 3 | `Hob.Topic.Bloom.Answer.Leave` | HOB | Odd is allowed out here. |

- "Safe to eat" follows the lore's food-safety rule: a mutated look never means harmful. No order reply: there is nothing here for Hob to do.

### SeasonEnd

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Topic.SeasonEnd.Open` | HOB | Spring's getting on. A cabbage planted now wouldn't ripen before summer. |
| 2a | `Hob.Topic.SeasonEnd.Reply.Ask` | PLAYER | What would you plant now? |
| 2b | `Hob.Topic.SeasonEnd.Reply.Order` | PLAYER | Keep track of the days for me. |
| 2c | `Hob.Topic.SeasonEnd.Reply.Leave` | PLAYER | I'll work it out. |
| 3 | `Hob.Topic.SeasonEnd.Answer.Ask` | HOB | Turnips, while there's time. They're quick enough to ripen almost to the last day. |
| 3 | `Hob.Topic.SeasonEnd.Answer.Order` | HOB | As instructed. I count them anyway. |
| 3 | `Hob.Topic.SeasonEnd.Answer.OrderView` | HOB | As instructed. I count them anyway. If you're planting, turnips still have time. |
| 3 | `Hob.Topic.SeasonEnd.Answer.Leave` | HOB | Of course. There'll be another spring. |

- True only from day 19 to day 25 of spring (cabbage 10 nights, turnip 3, last turnip day 25). It is a planning thought, not the later "crops at risk" reminder, which names crops already in the ground.
- `Answer.Order` promises only that Hob counts the days, not a reminder the slice doesn't have.

## H-05 - Attentive (step 1)

**When:** once, on the next talk after the attentive step is reached, ahead of topics and greetings. Lines only; the talk ends after line 4. Escape after line 1 still counts as played.
**For:** the player's surprise that Hob has a view at all, and the first sign they caused it. Line 2 has two versions so it names what the player actually did.

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Step.Attentive.010` | HOB | May I say something? It isn't a report. |
| 2 | `Hob.Step.Attentive.020A` | HOB | You've been asking what I think. A question isn't an order. I've found I have answers. |
| 2 | `Hob.Step.Attentive.020B` | HOB | You've been working the field I kept. Watching you, I've found I have opinions. |
| 3 | `Hob.Step.Attentive.030` | HOB | Here's one. The kettle boils faster on the left ring. I've thought so for years. Nobody asked. |
| 4 | `Hob.Step.Attentive.040` | HOB | I'll mention the others as they come, if you don't mind. |

- **020A** plays if the player has chosen at least one ask reply; otherwise **020B** (the step came from farm firsts alone).

## H-06 - "I'd like to" (step 2)

**When:** once, on the next talk after the "I'd like to" step is reached, ahead of topics and greetings.
**For:** Hob wants something for the first time and asks shyly; the player's answer decides whether Hob waters. Neither answer costs trust, and the offer isn't made again in the slice.

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Step.Wish.010` | HOB | There's something I'd like to do. That's a new sentence for me. |
| 2 | `Hob.Step.Wish.020` | HOB | The order used to fill every hour. Now there are hours left over, and I'd like to spend a few on your rows. |
| 3 | `Hob.Step.Wish.030` | HOB | I'd like to water a few by the door in the mornings, before you're up. If that's all right. |
| 4 | | | *The reply list opens, line 3 still showing.* |
| 4a | `Hob.Step.Wish.Reply.Yes` | PLAYER | Please do. |
| 4b | `Hob.Step.Wish.Reply.No` | PLAYER | I'll manage. |
| 5a | `Hob.Step.Wish.Answer.Yes` | HOB | Thank you. A few cans at first light won't rust my joints. |
| 5b | `Hob.Step.Wish.Answer.No` | HOB | Then I won't. Thank you for letting me ask. |

- **Yes** sets "Hob waters in the mornings"; from the next dawn Hob waters as the designer's question 6 says. **No** sets nothing more, and Hob says no more about it.
- Escape before a reply leaves the offer unanswered: the talk plays again on a later day (the story needs the player to be asked once, properly).
- "A few by the door" matches the rule: up to 4 dry, planted, unripe tiles nearest Hob's spot by the door.

## H-07 - The memory (step 3)

**When:** once, on the next talk after the memory step's conditions hold (the points and the first Sinkwort harvest), ahead of everything else. Escape after line 1 still counts as played.
**For:** the payoff. A small, domestic, certain memory comes back because the player cleaned the soil, and Hob discovers that something they thought was simply their know-how was taught to them by someone. Quiet, not a reward screen; nothing given or unlocked.
**Knowledge:** the player learns that the sleeper's mother worked this field and taught Hob the squeeze test. Nothing about where she is, what happened to her or the rest of the family. Hob doesn't guess at the gaps.

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Step.Memory.010` | HOB | I picked up a handful of soil where the Sinkwort came out. I squeezed it, to see if it would hold. |
| 2 | `Hob.Step.Memory.020` | HOB | It held, then broke when I pressed it with a thumb. That means it's ready to work. |
| 3 | `Hob.Step.Memory.030` | HOB | Your mother taught me that, in this field. I'd forgotten it was her. |
| 4 | `Hob.Step.Memory.040` | HOB | There are gaps in what I remember. This one came back whole. I'm certain of it. |
| 5 | | | *The reply list opens, line 4 still showing.* |
| 5a | `Hob.Step.Memory.Reply.Else` | PLAYER | What else do you remember? |
| 5b | `Hob.Step.Memory.Reply.Show` | PLAYER | Show me how she did it. |
| 5c | `Hob.Step.Memory.Reply.Silent` | PLAYER | (Say nothing.) |
| 6a | `Hob.Step.Memory.Answer.Else` | HOB | Not much yet. Her sleeves were rolled up and her hands were muddy to the wrist. I won't guess at the rest. |
| 6b | `Hob.Step.Memory.Answer.Show` | HOB | Take a handful. Squeeze. If it holds, then breaks when you press it, it's ready. Those were her words. |
| 6c | `Hob.Step.Memory.Answer.Silent` | HOB | All right. I'll keep it somewhere safe. I'd like to keep it. |
| 7 | `Hob.Step.Memory.050` | HOB | The field is coming back. Perhaps more of this will, too. |

- No reply changes anything; none is saved. "(Say nothing.)" is an action; keep the parentheses in translation.
- The squeeze test is about the soil's texture, not its contamination, so line 2 stays true on a tile Sinkwort has cleaned only one level.
- **When the full opening is built** ([O-05](Phase-01-Opening.md#o-05---first-family-trace), [E-11](Phase-02-Early.md#e-11---family-traces-continue)), this stays the first returning memory and those traces come after it; O-05's "I remember your mother laughing" already fits a Hob who has remembered her once.

## H-08 - Warmer greetings

**When:** from the "I'd like to" step, as the first talk of a day when no step talk or topic plays. **Recommended:** the warm lines join the H-02 pool for their hour (H-02's housework is still true), never yesterday's line; the designer decides whether they join or replace. After the memory, the three lines of the last pool join every hour's pool.
**For:** a Hob who now says what they like, in small doses.

| Pool | Line ID | Speaker | Text |
|---|---|---|---|
| Morning | `Hob.Greet.Warm.Morning.1` | HOB | Morning. I've been watching the light come over the field. I've decided I like that part. |
| Morning | `Hob.Greet.Warm.Morning.2` | HOB | Good morning. The kettle's on. I'd like to say it was for you, but I like the sound. |
| Afternoon | `Hob.Greet.Warm.Afternoon.1` | HOB | Afternoon. I've been deciding which of your rows looks best. I haven't decided. I'm enjoying it. |
| Afternoon | `Hob.Greet.Warm.Afternoon.2` | HOB | Good afternoon. Stand in the shade a minute, if you like. I'd like the company. |
| Evening | `Hob.Greet.Warm.Evening.1` | HOB | Good evening. Your bed is turned down. A habit, and also I wanted to. |
| Evening | `Hob.Greet.Warm.Evening.2` | HOB | Evening. I like this hour. The field goes quiet, and so do I. |
| After midnight | `Hob.Greet.Warm.Late.1` | HOB | Still up? So am I. I'd rather keep you company than keep the door. |
| After midnight | `Hob.Greet.Warm.Late.2` | HOB | Past midnight. I won't tell you to sleep. I'd only like you to know the bed is ready. |
| After the memory | `Hob.Greet.Memory.1` | HOB | I tried a handful from the near rows again. It held, then broke. Still ready. |
| After the memory | `Hob.Greet.Memory.2` | HOB | I've been keeping that memory close. It's good to have one I'm sure of. |
| After the memory | `Hob.Greet.Memory.3` | HOB | Hello. I like seeing someone's hands in that soil again. |

- None states the weather, a crop or what the player did today, so each is true on any day of its pool. "Your rows" is true from the first day: the field is the player's (H-01).

## H-09 - Mornings Hob watered

**When:** the first talk of a day on which Hob watered at least one tile that dawn. **Recommended:** it plays in place of the day's greeting; on the first such morning it should play even if a topic is due (the topic waits a day), because that morning is when the player links the dark soil to H-06. The designer decides the priority.
**For:** the help is visible on the farm; this line says who did it, without fuss.

| Line ID | Speaker | Text |
|---|---|---|
| `Hob.Watered.1` | HOB | I watered a few by the door at first light. The rest are yours. |
| `Hob.Watered.2` | HOB | The ones nearest the door have had their drink. I did them before you were up. |
| `Hob.Watered.3` | HOB | A few of yours were dry, so I watered them. I enjoyed it, which still surprises me. |

- No number: Hob may water fewer than four when fewer are dry or the store runs short.

## Length check and counts

- Longest HOB lines: `Hob.Step.Wish.020` and `Hob.Step.Memory.Answer.Else`, 106 characters each; longest reply `Hob.Topic.Sinkwort.Reply.Order`, 36. All inside 110 and 40.
- Lines: H-04 69 (9 openings, 26 replies, 34 answers), H-05 5, H-06 7, H-07 11, H-08 11, H-09 3: **106 lines**.
- **For translators:** "It isn't going anywhere" (`Hob.Topic.Well`) means "it will still be there later"; "good for the spirits" means "cheering". "That's a new sentence for me" is Hob noticing their own words; keep it literal.
