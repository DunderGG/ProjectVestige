# 15. Visual effects and feedback

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** no effects yet. Feedback reports are built (M4, 2026-11-08, below): gameplay reports tags and the HUD shows the text `DA_FeedbackResponses` gives each.

**References:** Epic's "Niagara" pages, including scalability and effect types; Lyra's gameplay cues and `ULyraContextEffectComponent` (effects chosen by surface type and context tags).

**Rules:**

- **Effects react to events**, like audio. Game code never spawns a particle system.
- **Niagara systems use effect types** with scalability settings and culling distances, so effects shrink or disappear at lower scalability and distance.
- Short, frequent effects are pooled (Niagara's pooling method on the spawn call).
- Effects are soft references in content data and load with the content that uses them.
- **Gameplay reports what happened; presentation picks the response.** Game code reports a feedback tag (`Vestige.Feedback.*`) with its values through `UVestigeFeedbackSubsystem::Report` and never writes the player's text; the HUD's `DA_FeedbackResponses` turns the tag into text (feedback reports, below). A use that worked reports nothing: it is already an event (`On Tool Used`, `On Crop Harvested`), and its sound and animation come through gameplay cues ([section 9](09-AbilitySystem.md)).

**Pitfalls:**

- Large translucent particles filling the screen (smoke, haze): use fog and post-processing for the haze ([Architecture.md, section 10](../Architecture.md#10-performance-by-design)).
- A sentence for every success ("Tilled the soil.", "Watered."): holding E filled the five message boxes in a few seconds ([CodeReview.md, M4](../CodeReview.md#m4--gameplay-code-writes-the-players-text)).

**Feedback reports** (design for [CodeReview.md, M4](../CodeReview.md#m4--gameplay-code-writes-the-players-text), 2026-11-08, approved after two rounds of review by a senior Unreal C++ reviewer agent, whose approval counts as David's; built the same day as described). Before it, gameplay called `UVestigeFeedbackSubsystem::Notify(World, Text)` with finished `LOCTEXT` sentences from eleven classes (the four tool abilities and their base, the farming and targeting components, the stamina set, the rest component, the game mode and the save subsystem), including one for every successful tool use and "Game saved." every morning. After this change gameplay reports *what happened* as a gameplay tag with named values, and the HUD looks the tag up in a data asset to pick the text.

References: Lyra's `ULyraGameplayAbility::FailureTagToUserFacingMessages` (a `TMap<FGameplayTag, FText>` on the ability, looked up in `NativeOnAbilityFailedToActivate` and broadcast as `FLyraAbilitySimpleFailureMessage` on the Gameplay Message Router) and `FLyraVerbMessage` (a verb tag plus instigator, target, tags and a magnitude, which `ULyraHealthSet` broadcasts on damage); Epic's "Gameplay Tags" and "Text Localization" pages (`FText::Format` with named arguments).

*Take, skip, change from Lyra:*

- **Take:** the reason is a gameplay tag and the text is looked up by tag, so new reasons are data and code never builds a sentence; Lyra's verb message's idea of a tag plus values instead of a finished string.
- **Skip:** the Gameplay Message Router plugin (Lyra's, not the engine's): one channel with one listener needs no bus, and [Architecture.md, section 4](../Architecture.md#4-how-systems-talk-to-each-other) keeps it for when many unrelated senders and listeners appear. Skip the per-player target and replication: single-player, as today.
- **Change:** Lyra keeps the text map on each ability, a gameplay asset; ours is one presentation asset on the HUD, since M4 asks that presentation picks the text, and the reports also come from outside abilities (rest, saving, the day). Our tool failures happen inside an active ability (the target can't be worked), not at activation, so they are reported by the base tool ability, not from `NativeOnAbilityFailedToActivate`.

*Classes:*

- **`FVestigeFeedbackReport`** (plain C++ struct, `Core/VestigeFeedbackSubsystem.h`): `FGameplayTag Tag` and `FFormatNamedArguments Arguments` (the values the text may use, such as `Day`). Not a `USTRUCT`: only native delegates carry it.
- **`UVestigeFeedbackSubsystem`** (world subsystem, `Core/`, keeps its name): `Notify` becomes `static void Report(const UWorld* World, const FGameplayTag& Tag, FFormatNamedArguments Arguments = {})`, and `OnFeedback` carries `const FVestigeFeedbackReport&`. It still logs every report to `LogVestigeFeedback` at `Log`, now as the tag and its values (`Vestige.Feedback.Day.Slept Day=2`), so the log keeps the history whatever the HUD shows. The values are turned into text by a helper in the subsystem's `.cpp` (`Name=` plus `FFormatArgumentValue::ToFormattedString(false, false)` for each, joined by spaces), called inside the `VESTIGE_LOG` arguments, so nothing is built while the category is off ([section 21](21-Logging.md)). An invalid tag logs a warning and is not broadcast.
- **Native feedback tags** in `AbilitySystem/VestigeGameplayTags.h`, with `UE_DEFINE_GAMEPLAY_TAG_COMMENT` (each comment names the reporter and its values), under `Vestige.Feedback` (no parent tag is declared: nothing names it, and the `Categories` meta is a string):

   | Tag | Reported by | Values | Text in `DA_FeedbackResponses` |
   | --- | --- | --- | --- |
   | `Vestige.Feedback.Tool.NoTarget` | the tool ability, on the press | | Stand facing a farm tile. |
   | `Vestige.Feedback.Tool.NothingInHand` | the farming component, on the press | | Nothing in hand to use. |
   | `Vestige.Feedback.Farm.CantTill` | the till ability | | Only grass can be tilled. |
   | `Vestige.Feedback.Farm.NothingToPlant` | the plant ability | | Nothing to plant in hand. |
   | `Vestige.Feedback.Farm.CantPlant` | the plant ability | | Seeds need empty tilled soil. |
   | `Vestige.Feedback.Farm.CantWater` | the water ability | | Nothing to water here. |
   | `Vestige.Feedback.Farm.NothingRipe` | the harvest ability | | Nothing ripe here. |
   | `Vestige.Feedback.Inventory.Full` | the harvest ability | | No room in your bag. |
   | `Vestige.Feedback.Stamina.Empty` | the stamina set | | You're out of stamina. Everything takes longer until you sleep. |
   | `Vestige.Feedback.Rest.GettingLate` | the rest component, at its warning hour | | It's getting late. Going to bed after midnight leaves you exhausted tomorrow. |
   | `Vestige.Feedback.Rest.PastMidnight` | the rest component, at its late-night hour | | It's past midnight. Your stamina drains until you sleep. |
   | `Vestige.Feedback.Rest.WokeExhausted` | the rest component | | You went to bed late and wake up exhausted. |
   | `Vestige.Feedback.Rest.WokeRested` | the rest component, when it clears exhaustion | | A good night's sleep. You're no longer exhausted. |
   | `Vestige.Feedback.Day.Slept` | the game mode | `Day` (the day that begins) | You slept. Day {Day} begins. |
   | `Vestige.Feedback.Day.StayedUp` | the game mode | `Day` | You stayed up all night. Day {Day} begins. |
   | `Vestige.Feedback.Save.Loaded` | the save subsystem | `Day` | Welcome back. Day {Day}. |
   | `Vestige.Feedback.Save.Saved` | the save subsystem | | Game Saved! |
   | `Vestige.Feedback.Save.Failed` | the save subsystem | | Couldn't save the game. The previous save is kept. |
   | `Vestige.Feedback.Save.ContentMissing` | the save subsystem | | Some saved things no longer exist in the game and were left out. |
   | `Vestige.Feedback.Save.Unreadable` | the save subsystem | | The save couldn't be read. It is kept as it is, and the game won't save until you start a new game. |
   | `Vestige.Feedback.Save.TooNew` | the save subsystem | | The save is from a newer version of the game. It is kept as it is, and the game won't save until you start a new game. |
   | `Vestige.Feedback.Save.PartTooNew` | the save subsystem | | Part of the save is from a newer version of the game. It is kept as it is, and the game won't save until you start a new game. |

   The texts are today's, word for word. The four success messages ("Tilled the soil.", "Planted a seed.", "Watered.", "Harvested a crop!") get no tag: a use that worked reports nothing. The design made "Game saved." a silent row (an empty text shows nothing); after the play-test David gave it the text "Game Saved!" in the asset (2026-11-08), which is what the data-driven row is for. An autosave icon can replace it later without a build. "No FarmGrid in the level..." is a setup error the player can't fix, so it stays a log warning only (Systems.md, section 21: logs aren't feedback), and the targeting component's next-tick timer for it goes.
- **`UVestigeFeedbackResponses : UDataAsset`** (`UI/`, `Const`), the asset `Content/Vestige/UI/DA_FeedbackResponses`: `TMap<FGameplayTag, FVestigeFeedbackResponse> Responses` (`EditDefaultsOnly`, `meta = (Categories = "Vestige.Feedback", ForceInlineRow)`), where `FVestigeFeedbackResponse` is a `USTRUCT` with `FText Message` (empty shows nothing). A struct rather than a bare `FText`, because the sound for a report joins the same row when audio arrives ([section 14](14-Audio.md)), as Lyra's second map (`FailureTagToAnimMontage`) shows a reason has more than a text. `FindResponse(const FGameplayTag&)` returns the row or `nullptr`, and the static `FormatMessage(const FVestigeFeedbackResponse&, const FVestigeFeedbackReport&)` formats its text with the report's arguments (`FText::Format(Message, Arguments)`). `IsDataValid` reports a key that is invalid or isn't a tag the tag manager knows (`UGameplayTagsManager::RequestGameplayTag(Name, false)`); whether each text uses only the values its reporter gives is checked by a content test (below). Its texts are gathered for translation from the asset, as any asset text.
- **`AVestigeHUD`** gains `UPROPERTY(EditDefaultsOnly, Category = "HUD") TObjectPtr<const UVestigeFeedbackResponses> FeedbackResponses`, set on `BP_HUD` (a hard reference: the asset is a few dozen texts, needed as soon as the HUD shows). `ShowMessage(const FText&)` keeps its job; a new `ShowFeedback(const FVestigeFeedbackReport&)` is bound to `OnFeedback` in its place: it finds the row, shows its formatted text when the text isn't empty, and does nothing for an empty one. A report with no row logs a warning (`LogVestigeFeedback`, naming the tag and the asset with `GetNameSafe(FeedbackResponses)`) and shows nothing; with no asset set, the HUD logs an error at `BeginPlay`, as for a missing widget class, and shows no messages. Test-only getter: `GetFeedbackResponses()`.

*Reporting in gameplay:*

- **Tool abilities.** `CanUseOn` gains the reason: `virtual bool CanUseOn(const FIntPoint& Tile, FGameplayTag* OutFailureReason) const` (no default argument, since it is virtual), which sets the reason when it returns false and the pointer isn't null. When several reasons apply, each ability reports the one today's `UseOn` says first:
   - Till: no tile state, `Tool.NoTarget`; otherwise `Farm.CantTill`.
   - Plant: no seed in hand first, `Farm.NothingToPlant` (as today, so `WithoutAPlayerStateNothingIsPlanted` gets it), then no tile state `Tool.NoTarget`, then `Farm.CantPlant`. Its `CanUseOn` checks the seed before the tile, in that order.
   - Water: no tile state `Tool.NoTarget`; otherwise `Farm.CantWater`.
   - Harvest: no tile state `Tool.NoTarget`, then not ripe or no crop `Farm.NothingRipe`, then no room `Inventory.Full`.

   `UseOn(const FIntPoint& Tile)` keeps its signature and stops reporting: it does the work and returns whether the farm changed. The base ability's first try after a press now asks `CanUseOn(Target, &Reason)` before `UseOn`, as a repeat already does, with `Reason` empty first. When it is false the base sets `Step = Using` (a listener may end the ability, as during `UseOn` today), reports `Reason` through a private `ReportFeedback(const FGameplayTag&)` (which replaces `Notify`; only the base reports now), returns if the ability is no longer active, and otherwise waits for another target or ends, as a use that changed nothing does today. A `CanUseOn` that returns false with no reason is a bug in that ability: `ensureMsgf` naming the ability's class, and nothing is reported. A repeat passes `nullptr` and stays quiet. Nothing runs between `CanUseOn` and `UseOn`, so `UseOn` returning false after `CanUseOn` passed means the two disagree: the base logs a warning (`VESTIGE_LOG(this, LogFarm, Warning, ...)` with the ability's class and the tile), reports nothing and treats it as a use that changed nothing. "No tile" on the press reports `Tool.NoTarget`. The harvest ability's `UseOn` keeps its check for room before harvesting, so a full bag never destroys a crop. As built, `ReportFeedback` returns whether the ability is still active, and the subclasses set their reason with the base's protected static `FailBecause(OutFailureReason, Reason)`, which returns false.
- **Everyone else** swaps `Notify(World, LOCTEXT(...))` for `Report(World, Tag)` or, with a day, `Report(World, Tag, {{TEXT("Day"), Day}})`. The farming component's `SayNothingInHand` becomes `ReportNothingInHand`. The targeting component loses its `Notify` member, the next-tick lambda, and the feedback and timer manager includes. The save subsystem's `MessagesForPlayer` becomes `TArray<FVestigeFeedbackReport> PendingReports`, still reported once the map has begun play, and `RefuseToSave` takes the reason's tag. Every `LOCTEXT` that was a player message leaves gameplay code, with its `LOCTEXT_NAMESPACE` where nothing else uses it.
- Unchanged: the interaction options' labels ("Sleep", "Harvest") stay with the interactables, which own what they offer ([section 7](07-Interaction.md)), the HUD's own texts (status, stamina, bag, hints), and data validation messages, which are editor text for developers.

*Data:* `DA_FeedbackResponses` is made by a temporary commandlet (as the H7 widgets were, deleted before the commit): it creates the asset with the rows above (the texts are today's source strings; the editor gives each a stable localization key, with the package's namespace, when it saves the asset, so the keys are new, which costs nothing while the game has no localization target), sets `FeedbackResponses` on `BP_HUD`'s class defaults, and saves both packages.

*Tests* (CQTest):

- `Vestige.Feedback.ReportsReachEveryListener` (replaces `MessagesReachEveryListener`): a report reaches a listener with its tag and arguments.
- `Vestige.Feedback.AnInvalidTagIsNotReported`: an empty tag isn't broadcast and logs the warning.
- `Vestige.Farming.AUseThatWorkedReportsNothing`: tilling grass, planting, watering and harvesting each change the tile and report nothing. `ARepeatThatWouldChangeNothingIsSkippedQuietly` now expects no report at all (the till used to report its success).
- `Vestige.Farming.APressReportsWhyAUseChangesNothing`: on the press, the hoe on tilled soil reports `Farm.CantTill`, the watering can on grass `Farm.CantWater`, seeds on grass `Farm.CantPlant`, the hand on an unripe crop `Farm.NothingRipe`, and the hand on a ripe crop with a full bag `Inventory.Full` with the crop still in the ground; each reports exactly one tag and costs nothing. `AnEmptyHandUsesNothing` and `ThePressSaysWhyAUseChangesNothingAndTheRepeatWaitsForTheNextTile` check the tag (`Tool.NothingInHand`, `Farm.CantTill`) instead of a count of texts.
- `Vestige.GameMode.TheReportSaysWhetherThePlayerSleptOrStayedUp` (renamed from `TheMessageSays...`): `Day.Slept` with `Day` 2, then `Day.StayedUp` with `Day` 3. The rest test checks `Rest.GettingLate` and `Rest.PastMidnight`; the stamina tests gain `RunningOutReportsOnce` (`Stamina.Empty` once when stamina reaches zero, not again at zero).
- `Vestige.UI.TheFeedbackResponsesFormatTheReport`: a response "Day {Day} begins." with `Day` 4 formats to "Day 4 begins."; `FindResponse` of a tag without a row is null.
- `Vestige.UI.EveryFeedbackTagHasAResponse`: `BP_GameMode`'s HUD class's `GetFeedbackResponses()` is set (read as `VestigeHudWidgetTests.cpp` reads the widget class) and has a row for each native feedback tag, listed in the test with the argument names its reporter gives (`Day` for `Day.Slept`, `Day.StayedUp` and `Save.Loaded`, none for the rest), and no other row. `RequestGameplayTagChildren` isn't used: it also returns the in-between tags (`Vestige.Feedback.Farm`). Every row has text (since David gave `Save.Saved` its text, 2026-11-08), and each text is a valid format (`FTextFormat::IsValid`) whose argument names (`GetFormatArgumentNames`, case-sensitive) are all among its tag's, so a `{day}` typo fails.
- `Vestige.UI.TheHudShowsNothingForAReportWithoutAResponse`: needs a HUD with a widget, which the test spawner can't make (no local player), so it is a play-test check, as in H7.

*Play-test (PlayTest.md):* holding E across a row of grass tills it with no messages; the press on a tile the tool can't change shows its reason once; sleeping shows "You slept. Day N begins." and no "Game saved." (since then "Game Saved!", David's text); the Output Log shows `LogVestigeFeedback` lines with the tags; editing a text in `DA_FeedbackResponses` shows in the next Play without a build; removing a row logs the warning and shows nothing.

*Known gaps,* recorded as deferrals in the decision log entry: a mod can't add a reason without replacing the base asset, since our tags live under `Vestige.` and mods add their own root ([Architecture.md, section 12](../Architecture.md#12-designing-for-mods)); later, response assets found through the Asset Manager and merged by the HUD. Blueprints can neither report nor listen, as with `Notify`; a Blueprint-callable `Report` and a dynamic twin of `OnFeedback` come with the first Blueprint or mod that needs them.

*Docs in the same commit:* this section; [section 6](06-UserInterface.md) (the HUD follows the feedback subsystem's reports and shows their responses); [section 9](09-AbilitySystem.md) (the `CanUseOn` signature, "with the same messages", and the M4 line under "Not in this change"); [section 18](18-Localization.md) (the gather includes `Content/Vestige/UI`); [section 21](21-Logging.md) (`LogVestigeFeedback` logs reports, "Logs aren't feedback" names `Report`); [Architecture.md](../Architecture.md): the ownership rows of `UVestigeFeedbackSubsystem` and `AVestigeHUD`, a row for `UVestigeFeedbackResponses`, the day-flow sequence diagram and a decision log entry with the known gaps above; [CodingGuidelines.md, section 10](../CodingGuidelines.md#10-errors-logging-and-assertions) (`Report` with a tag instead of `Notify` with `LOCTEXT`); [CodeReview.md](../CodeReview.md): a status line under M4; [Roadmap.md](../Roadmap.md): M4 ticked; [PlayTest.md](../PlayTest.md).
