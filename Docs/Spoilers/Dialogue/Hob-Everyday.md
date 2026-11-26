# Hob: first meeting and everyday lines

> **Spoilers.** Light ones: Hob's dutiful frame at the start of the slice arc.

The lines for Hob's first item in the game ([Roadmap.md, 4.4](../../Roadmap.md), "The caretaker robot": Hob in the game with data-driven dialogue). Written by the writer on 2026-11-25, on the game designer's recommended content scope (question 7 of the item's design questions, not yet answered by @DunderGG): a first meeting played once, a daily greeting by time of day, and a short line for talking again the same day. Nothing here touches trust, the tutorial or crops at risk; those are later items and add their own lines.

**What these lines are for:** the player meets someone who has kept the farm for them, and comes back most days because Hob is a small, steady part of the day. Hob starts formal and dutiful ([the slice arc](../Characters/Hob.md#vertical-slice-arc)): the farm was kept "as instructed", and Hob speaks of the sleeper as the reason for the keeping, not yet as a person whose views Hob wants. The warmth is in the domestic details.

## Status and how to enter them

- **Status: Ready** for every line in this file: final wording, voice and continuity checked.
- **Speaker:** every line marked HOB shows Hob's display name from the character data (`VestigeCharacter:Base.Hob`). Reply lines are the player's and have no speaker.
- **Line IDs** are the stable keys for the `FText` entries; never reuse or renumber one. The numbers leave gaps so lines can be added between them.
- **Length:** at most 110 characters for a HOB line and 40 for a reply (English), inside the designer's limit of about 150 characters (three lines in the box with room for German at one third longer). The longest HOB line here (`Hob.Meet.020`) is 100 characters; the longest reply is 27.
- **No placeholders** in this set. Hob does not say the player's name yet; that comes with the trust track, when using the name means something.
- **No pronoun for Hob** appears in any line, so the open pronoun question (see [Hob's sheet](../Characters/Hob.md#open-decisions)) changes no text here.
- **Working names in the text:** "Hob" and the maker "Hearthwell" ([Lore.md, section 7](../Lore.md#7-the-robot-makers)) are working names. A rename changes `Hob.Meet.Answer.Name` and the display name, nothing else.

## Knowledge

- **The player** knows nothing yet in the slice. The slice starts outdoors at the Player Start; the cryo-pod opening ([O-01](Phase-01-Opening.md#o-01---waking)) does not exist yet. The lines say nothing about the pod, the war, the time asleep or the family, so they stay true whether the player arrives here from O-01 or straight from a new game.
- **Hob** knows the order and that it is fulfilled now the sleeper is up. Hob says only what concerns the house and the farm today.
- **When O-01 is built,** H-01 is retired: the waking scene becomes the first meeting, and H-02 and H-03 stay as they are. (Recorded in [Lore.md, section 13](../Lore.md#13-decision-log), 2026-11-25.)

## H-01 - First meeting (slice)

**When:** the first talk with Hob ever, at any hour. Played once; afterwards "has met Hob" is set. It counts as the day's talk, so a second talk the same day gives H-03.
**For:** the player learns Hob's name and that Hob kept everything for them; Hob covers the meaning of the moment with duty.

| Order | Line ID | Speaker | Text |
|---|---|---|---|
| 1 | `Hob.Meet.010` | HOB | You're up. Good. I'm Hob. I look after the farm. |
| 2 | `Hob.Meet.020` | HOB | I've kept everything as instructed. The house is aired, the well runs clean, and the field is yours. |
| 3 | | | *The reply list opens, line 2 still showing. The first reply is highlighted.* |
| 3a | `Hob.Meet.Reply.Thanks` | PLAYER | Thank you, Hob. |
| 3b | `Hob.Meet.Reply.Name` | PLAYER | Is Hob short for something? |
| 3c | `Hob.Meet.Reply.Silent` | PLAYER | (Say nothing.) |
| 4a | `Hob.Meet.Answer.Thanks` | HOB | There's no need. It was the order. |
| 4b | `Hob.Meet.Answer.Name` | HOB | Homebody. Hearthwell HB-7. Hob is easier to call across a field. |
| 4c | `Hob.Meet.Answer.Silent` | HOB | All right. |
| 5 | `Hob.Meet.030` | HOB | If you need me, I'll be here by the door. |

- Each reply shows only its own answer (4a, 4b or 4c), then line 5 for all three. The choice changes nothing else and is not saved.
- **(Say nothing.)** is the silent reply: Hob takes it without comment and asks nothing more. The parentheses mark an action, not words spoken; keep them in translation.
- **For translators:** "Homebody" is the model's pre-war product name and plays on the English word for a person who likes to stay at home. Translate it as a homely product nickname, but keep "Hob" unchanged; the line says it is the shortened form, so pick a nickname that "Hob" can plausibly come from, or keep "Homebody" as a brand name.

## H-02 - Daily greeting

**When:** the first talk of each game day, once Hob has been met. The line comes from the pool for the hour, never the same line as the previous day's greeting. One line; the talk ends after it.
**For:** a small, true detail of the house each day, so a visit to Hob is worth a few seconds. The after-midnight lines notice the hour and offer rest without pushing it.
**Hours** (the story's assumption; the game designer sets the exact ones, and the lines still read right if a boundary moves by an hour): morning 06:00 to 11:59, afternoon 12:00 to 17:59, evening 18:00 to 23:59, after midnight 00:00 to 05:59. The game day runs from 06:00, so after midnight belongs to the same day as the evening before.

| Pool | Line ID | Speaker | Text |
|---|---|---|---|
| Morning | `Hob.Greet.Morning.1` | HOB | I opened the shutters at first light. The house likes a bright morning. |
| Morning | `Hob.Greet.Morning.2` | HOB | Morning. I've put your boots by the door. They were there already, but now they're straight. |
| Morning | `Hob.Greet.Morning.3` | HOB | Good morning. The kettle has boiled and the floor is swept. Nothing else to report. |
| Afternoon | `Hob.Greet.Afternoon.1` | HOB | Good afternoon. I'm keeping the door. It doesn't need much keeping, but here I am. |
| Afternoon | `Hob.Greet.Afternoon.2` | HOB | Afternoon. The shade reaches the door about now. A good place to stand. |
| Afternoon | `Hob.Greet.Afternoon.3` | HOB | There's cold water in the jug inside, if you're thirsty. I'm obliged to mention it, and glad to. |
| Evening | `Hob.Greet.Evening.1` | HOB | Good evening. I've turned down your bed. A habit, and I see no reason to break it. |
| Evening | `Hob.Greet.Evening.2` | HOB | Evening. It gets properly dark out here. Mind the step by the door. |
| Evening | `Hob.Greet.Evening.3` | HOB | Good evening. The house is ready for the night, whenever you are. |
| After midnight | `Hob.Greet.Late.1` | HOB | You're up late. So am I, but I don't need the sleep. |
| After midnight | `Hob.Greet.Late.2` | HOB | Past midnight. Your bed is made, whenever you want it. |
| After midnight | `Hob.Greet.Late.3` | HOB | The shutters are closed and the house is quiet. Nothing on this farm needs you until morning. |

- No line states the weather, the crops, a time on the clock or what the player did, so none can be untrue on a given day. The boots, kettle, jug, shutters and step are Hob's housework, told, not objects the player must find.
- "Properly dark" in `Hob.Greet.Evening.2` is a promise the game keeps (nights are properly dark); it is fine while the evening is still light.

## H-03 - Again today

**When:** any later talk the same game day, once the day's greeting (or H-01) has played. A line from this pool, never the one shown last. One line; the talk ends after it.
**For:** Hob is still there and still glad of the visit, without a new topic. Each is a statement, not a question, because the talk ends after it.

| Line ID | Speaker | Text |
|---|---|---|
| `Hob.Again.1` | HOB | Still here, by the door. |
| `Hob.Again.2` | HOB | Back again. The door is still well kept, if you were wondering. |
| `Hob.Again.3` | HOB | I haven't gone anywhere. I rarely do. |

- "Talked today" is not saved, so after a reload the day's greeting plays again. That is harmless: any greeting is true at any point in its hours.
