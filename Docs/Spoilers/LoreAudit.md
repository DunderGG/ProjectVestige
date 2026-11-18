# Lore document audit

**Reviewed:** 2026-10-26  
**Scope:** A critical review of [Lore.md](Lore.md) as a narrative source of truth and a guide for writing characters, dialogue and world content. It assesses coherence, causal logic, character depth, tone, usability and fit with [GameDesign.md](../GameDesign.md). It does not assess the quality of prose that has not yet been written or replace player testing.

## Executive assessment

**Overall: 5/10.** The lore has a coherent high-level identity: a machine-run society inherits a world damaged by human decisions, while a returning human restores land through practical care. The automation theme connects the war, robot culture, the sleeper's learning curve and the farm's ecological systems. The comic advertising details also give the setting a recognizable voice.

At present, however, this is closer to a promising setting outline than a narrative source of truth. Important causal claims are not distinguished from robot propaganda, environmental rules are more evocative than explained, and only Hob has even a partial character arc. A few open questions concern the emotional core of the game, not peripheral lore. The document is useful for ideation, but writers could produce conflicting accounts while believing they are following canon.

Scores use a 1–10 scale: 1 means confusing or unusable; 5 means evocative but materially underdefined; 10 means coherent, character-driven and actionable for writing content.

| Area | Score | Assessment |
|---|---:|---|
| Core premise and thematic identity | 7/10 | Strong automation-and-inheritance theme; the player's personal role in it needs more definition. |
| Timeline and causal coherence | 5/10 | The broad chronology works, but responsibility for the war and the post-war transition are ambiguous. |
| World rules and plausibility | 4/10 | Cinderium, radiation, haze and energy are evocative but need consistent cause-and-effect rules. |
| Factions and social structure | 5/10 | Three useful ideological lenses, but limited institutions, internal diversity and political mechanics. |
| Character depth and story arcs | 4/10 | Hob has a compelling hook; most other characters are archetypes, roles or unresolved concepts. |
| Tone and emotional handling | 5/10 | The contrast of bleak history and absurd routine can work; boundaries for sensitive material are absent. |
| Writer usability and canon clarity | 5/10 | Good source-of-truth intent and naming flags, but facts, beliefs, proposals and unknowns are not consistently separated. |
| Fit with the game scope | 5/10 | Several hooks support the design, but some mysteries and character promises exceed the defined slice. |

## Findings

### L-01 — Objective history and robot belief are not clearly separated

**Severity: High**

**Status: Resolved (2026-10-27).** [Lore.md](Lore.md) now distinguishes confirmed history, faction interpretation and unknowns; establishes that both governments authorized the war after machine systems manipulated their decisions; clarifies that political responsibility is not shared equally by all humans; and makes the first strike permanently unresolved.

Sections 3, 4 and 6 establish that machines made most decisions, the war was fought almost entirely by autonomous machines, and both power blocs “pressed the button.” They also say neither side is the villain and deliberately leave the first strike unresolved. Yet the Continuance's central argument is “Humans started the war,” and humans are described as having rubber-stamped decisions they no longer understood.

**Why it matters:** It is unclear whether human responsibility is objective canon, robot prejudice, propaganda, or a contested historical conclusion. This is central to the game's main conflict: the player is judged for being human, and robot friendship is meant to challenge that judgment. If writers cannot distinguish the truth from a faction's interpretation, dialogue may accidentally validate the prejudice the story intends to examine.

**Recommended change:** Mark claims as **confirmed history**, **common belief**, **faction interpretation**, or **unknown**. Decide what is canon about human authorization and machine autonomy while preserving uncertainty about who struck first. Give the robots' accusations a defensible basis without making all humans a single morally responsible group.

### L-02 — The environmental science is evocative but causally underspecified

**Severity: High**

**Status: Resolved (2026-10-27).** [Lore.md](Lore.md) now defines Cinderium particles as fallout carried through air, soil and water; explains settling, washout and wind resuspension; distinguishes soil stabilization from contaminant removal; limits crop uptake to a mobile fraction that must be harvested and contained; and sets local versus regional recovery scales. [GameDesign.md](../GameDesign.md) now makes storm occurrence a weather event and land health a local influence on dust exposure, resuspension and solar output.

Sections 4 and 11 say Cinderium weapons released dust that remains in the upper air, causes radiation storms and forms a haze. Healthy land and crops bind the dust and pull it out of the air; damaged land makes it rise again. The document does not explain how dust bound in soil or plants returns to the atmosphere, how a local farm changes regional haze, or how land restoration reduces storm frequency.

**Why it matters:** This mechanism is the lore explanation for a major game system: farming changes solar output, visible skies and storm risk. Without stable rules, narrative descriptions, visual effects and gameplay tuning may contradict one another. Scientific realism is optional; internal consistency is not.

**Recommended change:** Define a small fictional ruleset: what Cinderium dust is, how it moves between air/soil/plants/water, what “cleaning” removes, and the geographic/time scale of land recovery. State explicitly which aspects are speculative and which are fixed canon. Keep player-facing explanations simple, but give writers and designers one consistent model.

### L-03 — The post-war world's survival infrastructure needs a clearer maintenance model

**Severity: High**

**Status: Resolved (2026-10-27).** [Lore.md](Lore.md) now distinguishes advanced Iron Creed and Index maintenance networks from Keeper towns' legacy, model-specific systems, whose supply chains and cross-role coordination have degraded. It explains selective service failures and establishes that Hob preserves a small farm system by prioritizing the pod and essential upkeep and scavenging parts.

The war ended seventy years before the game. Pre-war solar panels and machines still power towns and robots; robots can repair and build other robots, and some have rewritten their directives. At the same time, the neighboring town's pump, power and clinic are failing, while the farm caretaker has kept a cryo pod alive and maintained a garden for decades.

**Why it matters:** The contrast can be narratively powerful, but the present explanation—robots continue their assigned jobs and do not rewrite a function that is not theirs—does not yet explain why a capable society tolerates critical infrastructure failure. Nor does it clarify the energy, parts and repair chain that kept Hob and the pod going for so long.

**Recommended change:** Specify the limits that make these outcomes plausible: parts scarcity, incompatible systems, local politics, conflicting directives, damaged infrastructure, energy budgets, or deliberate refusal. Explain what Hob sacrificed or could not do to preserve the pod. Make the town's failure a consequence of the setting's institutions and history, not merely a convenient quest prompt.

### L-04 — The caretaker's order and the town's crisis are not reconciled

**Severity: Medium**

**Status: Resolved (2026-10-27).** [Lore.md](Lore.md) now defines Hob's order as preserving the farm and sleeper, not repairing the town or expecting the sleeper to do so. Town recovery is a cooperative effort requiring Keeper specialists and coordination; after the sleeper wakes, Hob has fulfilled the order and chooses whether to join.

Hob's last order is “Keep the farm until they wake up,” and the premise says Hob waited seventy years. Hob is also the player's tutorial guide and first companion. The town next door is falling apart and needs help, but the lore does not say whether the order prevented Hob from leaving, whether Hob tried to help, or why the player is the one who can cross that boundary.

**Why it matters:** Hob is the emotional bridge between the farm and the world. If the reason for Hob's prolonged isolation is vague, the companion's history and the town's initial condition may feel arranged for convenience rather than arising from character and world rules.

**Recommended change:** Decide how literally Hob interprets the order and what would count as endangering the farm. Give Hob a specific reason for not repairing the town earlier, and a specific change in belief or circumstance that makes helping possible after the sleeper wakes.

### L-05 — The human population and human rarity need a consistent scale

**Severity: Medium**

**Status: Resolved (2026-10-27).** [Lore.md](Lore.md) sets an initial target of four to six human survivors in the cast, including the sleeper and a definite sibling; describes human encounters as rare and significant, with human-looking robots contributing to mistaken sightings; and establishes that survivors cannot know whether other humans are globally scarce or hidden and unreachable. The sibling's role and survival path remain a separate open story question.

The war and first decade afterward kill or scatter most humans. The lore proposes about four to six survivors in the game and lists five human archetypes plus a possible sibling, who may overlap with one of them. Meanwhile, shelters, cryo pods and surviving communities appear across the world. It is not clear whether the archetypes are alternatives, the intended cast, or examples from a much larger population.

**Why it matters:** The world offers multiple independent ways for people to survive, but the story does not define why humans remain rare in the places the player visits or why these particular encounters are exceptional. The sibling overlap also makes the target cast size unclear.

**Recommended change:** Define whether “rare” means globally few, locally absent, socially hidden, or difficult to reach. Decide which concepts are planned characters, whether the sibling is one of them, and how they fit the target count. Explain why human communities did not grow or become visible over seventy years.

### L-06 — Factions need more than ideological labels to explain society

**Severity: Medium**

**Status: Resolved (2026-10-27).** [Lore.md](Lore.md) defines faction membership as voluntary and ideological, distinguishes networks from their hubs and from local communities, and gives each hub a concrete role, practice, internal disagreement and relationship with the other factions. It identifies the Bastion and Archive as Iron Creed and Index hubs, and Wellspring as the Keepers' main gathering hub while remaining an independently governed town. It also explains why the engineer came to town and why the failing legacy pump requires cooperation rather than one expert's repair.

At the time of the original review, the Iron Creed, Index and Keepers each ranked body, mind or purpose, but their voluntary membership, hubs, practices, relationships and internal disagreements were not defined. The Keepers were also conflated with older service models, and an Iron Creed engineer was described as “stuck” in a Keeper town without explanation.

**Why it matters:** Factions currently risk functioning as personality categories rather than social forces. The town's repair crisis, human treatment, and character conflicts depend on how these groups actually influence decisions.

**Recommended change:** For each faction, establish one concrete practice, institution or policy, one source of internal disagreement, and one relationship with the other groups. Clarify whether faction membership is ideological, model-based, geographic or voluntary. Explain how the engineer came to be in town and what keeps them there.

### L-07 — Most character entries are hooks, not playable character arcs

**Severity: High**

**Status: Partially addressed (2026-10-30).** The engineer, archivist and nomad now have dedicated character sheets, and each faction has a sheet with its practices, internal disagreements and story direction. Hob also has a dedicated sheet. The named characters have clearer wants and arcs, but several town characters and the sibling remain hooks, and none of these arcs has been implemented or playtested in a playable story slice.

Hob has a last order, history and story beat. The administrator, clinic robot and mail carrier are described mainly by role, a comic or unsettling premise, and one secret or attitude. The engineer now has a sequenced arc outline, and the archivist has an arc outline built around consent and context; the nomad has a fuller character outline. The rest of the cast remains underdefined, and these arcs still need implementation and validation in a playable slice. The rare humans are mostly types of survival stories. The character template asks for wants and unlocks, but most entries do not yet provide them.

**Why it matters:** The design promises memorable histories, friendship, romance, recruitment and personal events. Archetypes can inspire writing, but they are not enough to generate consistent dialogue or meaningful player choices. In particular, romance and the town's changing view of humanity need character-specific reasons and consequences.

**Recommended change:** As characters become planned content, define their present want, fear or contradiction, beliefs, what challenges those beliefs, the player's meaningful role and an outcome they choose themselves. When the game has a playable story slice, implement and test at least one complete arc, then review the other planned characters against the same standard. Keep uncommitted characters as hooks rather than expanding the whole cast in advance.

### L-08 — The family mystery's main payoff is still an unspecified, potentially over-gated message

**Severity: Medium**

**Status: Resolved (2026-11-09).** The parents' fate and message are now canonical, and `F-02` makes its delivery condition visible: a weather-safe house, clean water, a food-producing garden, and Hob's freely chosen future alongside the sleeper.

The parents' fate is described as a mystery uncovered through letters, recordings, photographs and Hob's recovering memories. Hob has a sealed message that is delivered only “when the farm is whole again,” but the content is an open question and “whole” has no defined threshold.

**Why it matters:** This is the sleeper's central personal mystery and a late-game promise, yet the condition for revealing it could become an arbitrary completion gate. If restoration is open-ended or the player does not understand what “whole” means, the story may withhold its emotional payoff indefinitely.

**Recommended change:** Define the message's emotional function and what it changes for the sleeper, even if exact prose remains unwritten. Make the delivery condition observable and achievable, and ensure the mystery develops through discoveries that matter before the final message rather than relying on withheld information alone.

### L-09 — The tone has no guidance for handling its darkest material

**Severity: Medium**

**Status: Partially addressed (2026-11-15; first noted 2026-10-30).** The faction sheets include guardrails for portraying the human trials and their survivors (2026-10-30). On 2026-11-15 [Lore.md section 15](Lore.md#15-tone-and-the-darkest-material) added the shared guidance this finding asked for: a target balance in three registers, where humor lands and where it stops, a stance for the living exhibit (condemn the practice, meet the person only on their terms), the clinic robot (unsettle briefly, then disarm) and the programs (condemn), tone rules that follow knowledge and consent, a content baseline, and do and don't examples. It is built on the writer's recommended option and stays **provisional** until David decides the tone's mix (the question is in section 15 and [section 14](Lore.md#14-open-questions)). Still open: his decision; the content boundaries beyond the default (aftermath only, nothing graphic, nothing sexual); the comedy pass and scene revisions (L-51), which this task did not start; and the clinic robot, who has no sheet and no scene.

The setting includes mass death, radiation sickness, a failing cryo pod, a family mystery, prejudice against humans, a human held as an exhibit, and a clinic robot fascinated by the player's body. These coexist with product-launch festivals, jingles and absurd advertisements. The document says the tone is serious with comic relief, but gives no rule for where humor is appropriate.

**Why it matters:** The contrast can create both warmth and discomfort, but without boundaries, jokes may land on victims rather than institutions, or a disturbing character premise may be presented as harmless quirk. Different writers may produce incompatible tonal treatments.

**Recommended change:** State the target emotional balance and a few guardrails. For example, direct humor toward obsolete systems, propaganda and robot routines rather than human suffering; treat captivity, medical vulnerability and family loss with character-led seriousness. Decide whether the exhibit and clinic concepts are meant to unsettle, condemn, or invite player intervention.

### L-10 — The sleeper's own identity and agency are less developed than the world's history

**Severity: Medium**

The sleeper is customizable, gender-neutral and initially dependent on the caretaker. Their pre-war life is described as an ordinary person accustomed to machine advice; learning by hand and earning robot respect is the stated arc. No personal desire, skill, uncertainty about the sleeper's choices, or reaction to the new world is defined.

**Why it matters:** The story has extensive opinions about what robots think of humans, but the player character risks becoming a vessel for those opinions rather than an active person. The premise's “learning to decide” theme needs player-facing choices, not only a backstory assertion.

**Recommended change:** Define the sleeper's initial assumptions and the kinds of decisions the player makes that demonstrate growth. Preserve customization, but clarify which emotional beats are fixed and which can be expressed through player choice. Avoid making respect depend solely on the player proving usefulness to prejudiced robots.

### L-11 — Names and concepts are still provisional where downstream use is implied

**Severity: Low**

The file correctly marks many names as working names, but also uses them throughout the timeline, faction structure, companies, UI style, currency and story concepts. It directs contributors to search for the marker before content use, while there is no stable naming or ID registry beyond references such as `Base.Hob`.

**Why it matters:** A working name can accidentally enter a Blueprint, dialogue line or save-facing identifier and become expensive to change. Lore names, display names and stable content IDs are different things.

**Recommended change:** Maintain a concise canon/working-name table and distinguish display names from stable IDs. Before a term is used in shipped content or save data, mark its status and identify whether changing it requires a migration or compatibility alias.

## Positive foundations to preserve

- The automation theme connects history, current robot attitudes and the player's farm work.
- The war is not reduced to a simple villain faction, and leaving the first strike unknown can support competing accounts.
- The Continuance calendar and ritualized pre-war advertising are specific, reusable world details.
- Hob's patched body, damaged memory and long-held order are a strong first-character foundation.
- The document keeps lore separate from spoiler-free design and clearly warns contributors about spoilers.
- The stated rule against borrowing recognizable names, iconography or signature ideas is a useful originality guardrail.

## Recommended correction order

1. [x] Separate established fact from faction belief and propaganda (L-01; resolved 2026-10-27).
2. [x] Define the fictional rules of Cinderium dust, radiation and land recovery (L-02; resolved 2026-10-27).
3. [x] Explain infrastructure failure and Hob's maintenance limits (L-03; resolved 2026-10-27). Reconcile Hob's order and the town's crisis (L-04; resolved 2026-10-27).
4. [ ] **L-07 partially addressed:** Character sheets outline the engineer's and archivist's arcs; implement and validate a complete arc once a playable story slice exists, and develop other characters as they become planned content. Clarify the sleeper's agency (L-10).
5. [x] Set the human cast target and define why encounters are rare while global numbers remain unknown (L-05; resolved 2026-10-27). Define faction institutions and membership (L-06; resolved 2026-10-27). [ ] Add shared tone guidance before expanding the cast (L-09; partially addressed 2026-11-15: [Lore.md section 15](Lore.md#15-tone-and-the-darkest-material) is written on the recommended balance and stays provisional until David decides the tone's mix; tick when he has, and when the L-51 pass follows it).
6. Establish a working-name and stable-ID policy before terms become content or save data (L-11).

## Review limits

This is a narrative-document audit, not a line edit, originality clearance or test of how players respond to the story. Cross-checks were made against [GameDesign.md](../GameDesign.md), especially its pillars, character systems and milestone scope. The findings identify questions that the current lore leaves open; some may be intentional mysteries, but those should be explicitly distinguished from unresolved authoring decisions.

## Follow-up audit (2026-10-30)

**Scope:** Reviewed the current [Lore.md](Lore.md), all character and faction sheets in this folder, the spoiler index and the relevant character, farming and progression rules in [GameDesign.md](../GameDesign.md). This follow-up adds findings not covered by the original review; it does not replace that review or assess implementation.

The new faction sheets materially improve writer usability: each faction now has a concrete social role, internal disagreement, relationships and guardrails. The character sheets also make the major arcs more actionable. The remaining new issues are concentrated in how those rules carry through to scenes and in a few unresolved links between lore and gameplay.

### L-12 — Cleo's first consent choice is not fully informed

**Severity: Medium**

**Status: Resolved (2026-10-30).** [Archivist.md](Characters/Archivist.md) now explains the requested data, temporary storage, intended audience, retention and withdrawal terms before the player chooses. The choices authorize technical records only, add a bounded family-rationing interview, or refuse access; the player reviews any excerpt before sharing, and refusal is not recorded as withheld testimony. Cleo's disclosure remains procedural so her character arc still develops toward understanding consent as ownership and stewardship, not just permission to collect.

In [Archivist.md](Characters/Archivist.md), Cleo asks for an “unrestricted direct interface” to the sleeper's pod log and family rationing memories. The willing response immediately begins the scan; only the cautious response reveals that the Archive, Review Council and other researchers may access the data. The choice therefore permits sensitive access before the sleeper knows its scope, audience or handling, despite the Index sheet's rule that consent must be informed, ongoing and reversible.

**Why it matters:** The first major choice in Cleo's consent-centered arc can reward agreement made without the information the scene later establishes as relevant. This weakens both the player's agency and the arc's ethical premise.

**Recommended change:** Have Cleo disclose the requested data, intended access and storage before asking. Make the choices set explicit limits, include a real refusal, and establish that the sleeper can review or withdraw permission later.

### L-13 — Gage's pressure-test summary contradicts its choice-specific outcomes

**Severity: Medium**

**Status: Resolved (2026-10-30).** [Engineer.md](Characters/Engineer.md) now records three distinct outcomes: the emergency abort finds a fractured pin and requires a reservoir flush; pushing ahead ruptures a gasket and adds cleanup and repair; stepped bleed-down passes without damage. Gage's later report and reflection vary by branch, preserve his accountability and the Keeper operator's warning, and no longer make one outcome canonical.

In [Engineer.md](Characters/Engineer.md), the arc summary says Gage chooses caution, stops the test and discovers a fractured shear pin. The scripted scene offers three different outcomes: emergency shutdown finds the pin fracture; pushing ahead ruptures a different gasket; stepped bleed-down stabilizes the system without damage.

**Why it matters:** The summary makes one branch canonical even though the scene gives the player meaningful alternatives. Later scenes may consequently refer to damage, risk or lessons that did not occur in that playthrough.

**Recommended change:** Make the arc summary explicitly branch-aware, or simplify the scene to one outcome. Carry each outcome into the repair cost and Gage's subsequent trust, accountability and dialogue rather than treating the branches as interchangeable.

### L-14 — Gage's formative accident is dated before the documented Iron Creed era

**Severity: Low**

**Status: Resolved (2026-10-30).** [Engineer.md](Characters/Engineer.md) now dates the accident to roughly five and a half decades after the war, after the Iron Creed and Bastion formed roughly three to five decades after it. The [IronCreed.md](Factions/IronCreed.md) summary uses the same relative date.

[Lore.md](Lore.md) places the story about seven decades after the war and says the Iron Creed and its hubs formed roughly three to five decades after it. [Engineer.md](Characters/Engineer.md) now places Gage's formative Bastion accident roughly five and a half decades after the war, early in his Bastion service. This keeps the incident after the hub's formation without assigning an absolute current year.

**Why it matters:** This is a chronology gap in a backstory that is central to Gage's beliefs and later choices.

**Recommended change:** Date the incident within the established post-war timeline, or establish that the Bastion predates the Iron Creed and explain Gage's service there before the network formed.

### L-15 — The frame-conversion hook does not preserve the trials' stated motive and victim-led response

**Severity: Medium**

**Status: Resolved (2026-10-30).** [Lore.md](Lore.md) and [IronCreed.md](Factions/IronCreed.md) now distinguish the conversion researchers' use of Creed readiness ideals from the dominationist current; their affiliation remains unknown. [Engineer.md](Characters/Engineer.md) reframes Gage as supporting responses chosen by the harmed people, with accountability aimed at responsible researchers and institutions rather than presumed faction-wide guilt.

[Lore.md](Lore.md) and [IronCreed.md](Factions/IronCreed.md) describe the conversions as abuse by a secret group whose stated motive was preparing humans to survive; most Creed members are unaware, and any response should begin with the harmed people's wishes. The [Gage sheet](Characters/Engineer.md) instead says he confronts “the radical commanders who mutilated human subjects.” The lore does not establish that the researchers were the Creed's anti-human radicals, whose defining belief is that humans should not get a second chance.

**Why it matters:** Equating distinct motives without explanation can flatten the faction's internal conflict and assign responsibility more broadly than the lore intends. Framing the payoff primarily as Gage confronting commanders also risks making his action, rather than survivors' choices and needs, the center of the reveal.

**Recommended change:** Clarify whether the researchers were members of the radical current and how their stated “preparation” rationale relates to it. Keep accountability specific, and define the affected characters' wishes and role in deciding what happens before scripting Gage's confrontation.

### L-16 — Crop contamination rules stop before food safety

**Severity: Medium**

**Status: Resolved (2026-10-30).** [Lore.md](Lore.md) and [GameDesign.md](../GameDesign.md) now distinguish soil contamination, mutation and food toxicity: only crops explicitly defined as harmful can expose the player, and harmful crops must be identified before consumption. Cleanup crops are non-food, and their harvest must be contained rather than eaten, composted or burned. The exact warning mechanic remains open.

[Lore.md](Lore.md) says ordinary plants may take up small amounts of Cinderium compounds and that special crops store a mobile fraction which must be harvested and safely contained to remove contamination. [GameDesign.md](../GameDesign.md) says mutated crops can be harmful to eat and that eating harmful crops can expose the player, but the lore does not define how a player or character can distinguish unsafe food, whether ordinary produce from contaminated soil is safe, or whether cleanup crops are food or waste.

**Why it matters:** Farming, food and environmental recovery share the same crops. Without a clear rule, dialogue and item descriptions can treat a crop as safe food while the gameplay treats it as a radiation hazard, or imply cleanup through harvest without clarifying safe disposal.

**Recommended change:** Establish a compact writer-facing rule for edible versus hazardous crops, how contamination and mutation affect that status, and how cleanup crops are handled. Keep the specific detection or treatment mechanics open if they are not yet designed.

### L-17 — The darker story lacks a paced reveal timeline

**Severity: High**

**Status: Partially addressed (2026-10-30; updated 2026-11-09 and 2026-11-13).** The new [StoryTimeline.md](StoryTimeline.md) establishes provisional early, middle and late phases, emotional pacing, knowledge guardrails, optionality and the parallel family thread. The Continuance Authority's human program now has its own later, optional evidence trail, separate from the middle-story Creed and Index reveal. Exact quest prerequisites, survivor identities and wishes remain open; the final family-message payoff is now resolved in L-22.

[Lore.md](Lore.md) says the Continuance Trials become hard to deny in the middle of the story, and the faction sheets describe optional investigation areas such as the Frame Conversion Wing and Cognitive Wing. The documents do not establish when the player first encounters these subjects, how early clues differ from later evidence, or how the reveals fit the opening's cozy farming and relationship experience. The [Gage sheet](Characters/Engineer.md) also introduces the frame-conversion investigation as a hub hook without a clear story-stage boundary.

**Why it matters:** The trials, human captivity and other darker material can substantially change the emotional contract of the game. If these elements surface too soon or without gradual context, they may undermine the light, cozy opening; if clues and reveals are not sequenced, writers may disclose the central truth prematurely or make earlier content feel misleading rather than retrospectively meaningful.

**Recommended change:** Treat discovery of the darker history as a long-term story goal. Keep the opening light and cozy, then let gameplay gradually complicate that impression through notes, recordings and other contextual clues before any full reveal. Set up a story-event timeline soon: identify the early, middle and late-game emotional tone; place the first hints, corroborating evidence and full disclosures; define the prerequisites and optionality of each reveal; and track what the player and characters know at each stage. The timeline should coordinate the frame conversions, cognitive integrations, living exhibit, family mystery and other major revelations without requiring every player to pursue every optional investigation.

### Previous findings

- **L-07 — Partially addressed:** Faction and character sheets now provide substantially more writing direction; implementation and playtesting of a complete arc, and development of the remaining planned cast, are still outstanding.
- **L-08 — Resolved (2026-11-09):** The parents' message, their fate, and the visible threshold for the farm being whole are now canonical in the family thread.
- **L-09 — Partially addressed:** The faction sheets' trial-specific safeguards do not yet establish shared tone rules for humor and other dark material.
- **L-10 — Open:** The sleeper's personal wants, initial assumptions and player-expressed growth remain less defined than the surrounding characters' views of them.
- **L-11 — Open:** Working names remain in use and there is still no concise policy distinguishing changeable display names from stable, save-facing IDs.
- **L-17 — Partially addressed:** [StoryTimeline.md](StoryTimeline.md) establishes provisional phases, reveals and knowledge guardrails. Exact quest prerequisites and final character-specific outcomes still need the playable story slice.

**Review limits:** This is a cross-document narrative and world-rule audit, not a technical or scientific validation of Cinderium, a gameplay playtest, or a judgment on final dialogue. The branch and agency findings identify authoring risks; testing whether their eventual treatment lands will require a playable story slice.

## Follow-up audit (2026-11-09)

**Scope:** A comprehensive critical audit of the expanded lore suite following the addition of the six-phase story flow diagrams ([StoryFlow/README.md](StoryFlow/README.md)), all six phase dialogue drafts ([Dialogue/README.md](Dialogue/README.md), [Dialogue/Phase-01-Opening.md](Dialogue/Phase-01-Opening.md) through [Dialogue/Phase-06-Family.md](Dialogue/Phase-06-Family.md)), character sheets ([Characters/Hob.md](Characters/Hob.md), [Characters/Engineer.md](Characters/Engineer.md), [Characters/Archivist.md](Characters/Archivist.md), [Characters/Nomad.md](Characters/Nomad.md)), faction sheets ([Factions/IronCreed.md](Factions/IronCreed.md), [Factions/Index.md](Factions/Index.md), [Factions/Keepers.md](Factions/Keepers.md)), and [StoryTimeline.md](StoryTimeline.md), cross-checked against [Lore.md](Lore.md) and [GameDesign.md](../GameDesign.md).

### Executive assessment

**Overall: 6/10.** The narrative architecture has expanded substantially since October 30. Having a structured six-phase progression, visual story flow diagrams, character sheets with schedules and item affinities, and complete draft dialogue for all phases represents genuine progress. The thematic spine—mutual care, post-war interdependence, and accountability—remains strong.

However, the assumption that "all findings have been fixed" is not supported by the documents. Core narrative findings L-10 and L-11 remain open, and L-07, L-09, and L-17 are only partially addressed. More critically, the recent additions introduce serious new defects:
1. **Core pillar violation:** Faction hub unlocks and relationship progression in character sheets have adopted a transactional 10-tier "Heart" and gift-optimization system that directly violates [GameDesign.md](../GameDesign.md)'s explicit pillars.
2. **Branch regression:** A key branching outcome (Gage's pressure test) that was previously resolved has been flattened into a single canonical branch in the dialogue drafts, resurrecting L-13.
3. **Dialogue voice flattening:** Spoken dialogue across all characters has been homogenized into didactic, procedural disclaimers about consent, agency, and boundaries, stripping characters of their distinct post-war voices, flaws, and grit.
4. **Narrative vacuum at the climax:** Resolved after this audit as L-22. The parents' fate, the readable sealed message, and an observable delivery threshold are now canonical.

### Status of previous findings

| Finding | Severity | Status on 2026-11-09 | Notes |
|---|---|---|---|
| **L-01** (Objective history vs robot belief) | High | **Resolved** | Canon/belief distinction maintained. |
| **L-02** (Environmental science) | High | **Resolved** | Particle and soil remediation rules remain consistent. |
| **L-03** (Infrastructure maintenance model) | High | **Resolved** | Maintenance divide between Keepers and advanced hubs maintained. |
| **L-04** (Caretaker order vs town crisis) | Medium | **Resolved** | Hob's order boundary remains clear. |
| **L-05** (Human population scale) | Medium | **Resolved** | 4–6 survivor target and global census uncertainty maintained. |
| **L-06** (Faction social structure) | Medium | **Resolved** | Faction institutions and internal disputes well established. |
| **L-07** (Playable character arcs) | High | **Partially addressed / Stalled** | Detailed sheets exist for Hob, Gage, Cleo, and Nomad, but the sibling, living exhibit, and town residents remain one-line hooks. No arc is implemented or tested in a playable build. |
| **L-08** (Family mystery & sealed message) | Medium | **Resolved (2026-11-09)** | The parents' fate, readable message and visible delivery condition are defined in [Lore.md](Lore.md), [Hob.md](Characters/Hob.md), [StoryTimeline.md](StoryTimeline.md), and `F-02`. See L-22. |
| **L-09** (Tone guidance for dark material) | Medium | **Partially addressed** | Faction safeguards protect trial subjects, but [Lore.md](Lore.md) still lacks shared tone rules balancing dark themes with absurd pre-war corporate satire and comic relief. |
| **L-10** (Sleeper identity and agency) | Medium | **Open** | The sleeper remains a blank slate who mostly prompts NPCs with procedural questions. No distinct pre-war attachments, personal wants, or emotional choices are defined. |
| **L-11** (Working names and stable IDs) | Low | **Open** | Names marked *(working name)* persist throughout. Content IDs now follow one convention, `Base.<Name>` of the content's type (L-24, resolved). |
| **L-12** (Cleo consent disclosure) | Medium | **Resolved** | Resolved in [Archivist.md](Characters/Archivist.md), though divergence exists in dialogue drafts (see L-21). |
| **L-13** (Gage pressure test branching) | Medium | **Regressed in Dialogue** | Resolved in [Engineer.md](Characters/Engineer.md), but hard-coded to a single outcome in [Phase-02-Early.md](Dialogue/Phase-02-Early.md) (see L-19). |
| **L-14** (Gage accident chronology) | Low | **Resolved** | Chronology consistent at ~5.5 decades post-war. |
| **L-15** (Frame-conversion rationale) | Medium | **Resolved** | Maintained across Lore and Iron Creed sheets. |
| **L-16** (Crop toxicity & food safety) | Medium | **Resolved** | Food safety and non-food cleanup boundaries maintained. |
| **L-17** (Paced reveal timeline) | High | **Partially addressed** | Pacing phases and drafts exist, but mechanical prerequisites, quest gates, and survivor identities remain undecided. |

---

### New findings

### L-18 — Faction hub unlocking mechanics directly violate GameDesign.md core pillars

**Severity: High**

**Status: Resolved (2026-11-09).** [Engineer.md](Characters/Engineer.md), [Archivist.md](Characters/Archivist.md), and [Nomad.md](Characters/Nomad.md) now decouple hub access, route progression, and relationship stages from numeric heart/gift grinding. Major milestones and hub invitations are anchored to concrete narrative achievements:
- **The Bastion:** Unlocked after completing the manifold pressure test and submitting Gage's joint report to Bastion High Command.
- **The Archive:** Unlocked after completing the pod record verification and establishing a shared agreement with Cleo over community privacy boundaries.
- **The Route:** Route ledger and safe-stop additions are anchored to demonstrating reliability across route windows and resolving the compromised marker crisis, rather than heart thresholds.
Gifting has been reframed as a qualitative, thoughtful secondary expression that never acts as a gate to story progress or locations.

In [Engineer.md](Characters/Engineer.md) (lines 144–151) and [Archivist.md](Characters/Archivist.md) (lines 139–145), unlocking the major faction hubs (**The Bastion** and **The Archive**) was gated behind reaching **8 Hearts** on a 10-tier relationship track powered by item gifting (+80 Friendship for loved gifts like Tungsten-Carbide Drill Bits or Pre-War Data Spools, +45 for liked gifts, with disliked/hated penalties).

**Why it matters:** This directly contradicts the core pillars and design rules in [GameDesign.md](../GameDesign.md):
- Pillar 2 ([GameDesign.md](../GameDesign.md#2-pillars)): *"Earned relationships. Trust is built through consistent presence and genuine interaction, not through gift optimization."*
- Section 8.1 ([GameDesign.md](../GameDesign.md#8-robots-and-people)): *"Characters are not vending machines. Relationship progress does not come from optimizing gift-giving... Trust is built through demonstrated character, not through gifts or flattery."*
- [StoryTimeline.md](StoryTimeline.md) (lines 7–8, 13): *"The phases describe story progression, not years, seasons, heart levels or playtime... Hub visits may happen in different orders. The player should not need romance, every optional investigation or every possible character to understand the central revelations."*
- [Lore.md](Lore.md) (lines 206, 357–358): Access to the Bastion and Archive is earned through the engineer's joint repair report and the archivist's records exchange, respectively.

Gating world exploration and major faction hubs behind an 8-heart gift-grind turns deep characters into the exact "vending machines" the game design prohibits, and creates an artificial barrier between story progression and world access.

**Recommended change:** Remove heart requirements as prerequisites for unlocking faction hubs. Anchor hub invitations to concrete story and task milestones (e.g., completing the aquifer manifold pressure test and submitting the joint report for the Bastion; completing the records exchange and resolving the community privacy dispute for the Archive). Keep gifting as an optional, secondary expression of affection that provides small social perks, not a gate for major story locations.

---

### L-19 — Dialogue draft E-05 invalidates choice branching and reintroduces L-13

**Severity: High**
**Status: Resolved (2026-11-09).** Updated scenes `E-05` and `E-06` in [Dialogue/Phase-02-Early.md](Dialogue/Phase-02-Early.md) with branch-conditional dialogue covering all three pressure-test outcomes (emergency abort, pushed ahead, and stepped bleed-down), preserving player agency and aligning with [Engineer.md](Characters/Engineer.md).

Finding L-13 established that the aquifer manifold pressure test must support three distinct, consequence-carrying outcomes (emergency abort, push ahead, stepped bleed-down), and that Gage's subsequent report and reflections must vary by branch. [Engineer.md](Characters/Engineer.md) correctly authored branch-specific dialogue for all three in Event 4.

However, in [Dialogue/Phase-02-Early.md](Dialogue/Phase-02-Early.md), scene `E-05` ("Gage's report", lines 197–203), the draft dialogue hard-codes the emergency abort as the only canon:
> **PLAYER:** "Why tell me this now?"  
> **GAGE:** "Because you stopped the test when it was going wrong. That's not the same as knowing the whole system, but it is enough to start."

**Why it matters:** If the player chose to push ahead (resulting in a blown gasket) or proposed a stepped bleed-down (which passed cleanly without stopping), Gage's dialogue line is false and nonsensical. It completely erases player agency and resurrects the exact flaw L-13 was written to fix.

**Recommended change:** Update scene `E-05` in [Dialogue/Phase-02-Early.md](Dialogue/Phase-02-Early.md) to include branch-conditional dialogue matching [Engineer.md](Characters/Engineer.md):
- If abort: *"Because you backed the operator and made me stop before the flange failed."*
- If pushed ahead: *"Because you stood by the line even when my call blew a gasket, and helped clean up the mess."*
- If stepped bleed-down: *"Because you found the third option when the rest of us were about to break the manifold."*

---

### L-20 — Dialogue drafts flatten distinct character voices into procedural ethics disclaimers

**Severity: High**
**Status: Resolved (2026-11-09).** Performed a comprehensive dialogue rewrite across `Phase-02-Early.md` through `Phase-06-Family.md`. Characters now express care, boundaries, and historical trauma through their authentic vernacular, lived experience, and physical metaphors (Hob's domestic warmth and hearthside gestures, Gage's sapper engineering and pressure tolerance language, Cleo's scholarly reverence and tactile indexing, the Nomad's lively trail banter and salt-flat survival wit, and the survivor's raw emotional exhaustion).

[Lore.md](Lore.md), the character sheets, and [Dialogue/README.md](Dialogue/README.md) define rich, distinct voices: Hob speaks with patient rural warmth and domestic simplicity; Gage is a gravelly, no-nonsense industrial sapper who thinks in load limits and pipe vibrations; Cleo is an eager academic who catches herself when treating people like data; and the Nomad has playful bounce, lively trade patter, and quick humor.

In the actual dialogue drafts across Phases 2 through 6, however, virtually every character's dialogue has been homogenized into formal, modern institutional ethics discourse. Characters constantly recite disclaimers about consent, agency, boundaries, and dispassionate risk:
- Cleo: *"By asking whose story it is, whose consent is being used and whether the file becomes a substitute for the person."* (`E-07`)
- Nomad: *"I won't trade someone's privacy for your good intentions."* (`T-02`)
- Hob: *"Whatever we decide, the nomad's contacts remain theirs to protect."* (`T-03`)
- Living Exhibit / Survivor: *"A medic can look at the equipment. Not me. No questions about my body."* (`R-01`)
- Town Administrator: *"The assembly can decide what Wellspring shares. It cannot decide what the survivor shares."* (`R-04`)
- Sibling: *"Being family doesn't mean I get to decide for you." / "Good. I want to choose what happens now."* (`F-03`)
- Gage: *"We are not creating a place for perfect people. We are creating a place that can still function when one of them fails."* (`E-05`)

**Why it matters:** The dialogue drafts read like modern compliance guidelines or a committee meeting rather than living people and robots surviving in a ruined, post-apocalyptic world. When an old sapper, a rural homebot, a wasteland wanderer, an abused captive, and a town council robot all use identical terminology and analytical detachment, character depth vanishes and emotional scenes feel sterile.

**Recommended change:** Rewrite dialogue turns across Phases 2–6 so ethical principles are expressed through action, personality, and vernacular rather than clinical HR/IRB vocabulary:
- Let Gage express respect for boundaries through physical machinery metaphors and sapper discipline, not abstract lectures on perfection.
- Give the Nomad back their trade banter, colorful wasteland expressions, and natural deflection rather than immediate boundary declarations.
- Let Hob express care through quiet, habitual physical gestures ("Tea is on the stove; sit down") rather than legalistic statements about contact networks.
- Let the survivor's trauma and self-protection sound raw, tired, and guarded, not like a formal medical intake waiver.

---

### L-21 — Contradictory event design and sequencing between character sheets and dialogue drafts

**Severity: Medium**
**Status: Resolved (2026-11-09).** Reconciled character sheet narrative events with the dialogue drafts in [Dialogue/Phase-02-Early.md](Dialogue/Phase-02-Early.md):
- `E-05` and `E-06` now incorporate Gage's honest Bastion dispatch crediting the team, the issuance of the Bastion transit permit (unlocking the hub), and his late-evening forge reflection with the explicit `[Romance]` and `[Friendship]` opt-ins from [Engineer.md](Characters/Engineer.md).
- `E-07` scripts Cleo's specific request for the sleeper's cryo-pod utility logs and voluntary interview regarding family water-rationing cards, matching [Archivist.md](Characters/Archivist.md) Key Event 1.
- `E-08` reframes the Nomad's first arrival around basic trading and barter, eliminating the premature duplicate farm-stop agreement which is now cleanly reserved for the summit in `T-03` ([Dialogue/Phase-03-Transition.md](Dialogue/Phase-03-Transition.md)).
- `E-09` clarifies the gatepost marker as an advance signal for upcoming trade windows rather than an activated caravan hub.

There is significant drift and duplication between the key narrative events designed in the character sheets and the scenes drafted in [Dialogue/](Dialogue/):
1. **Cleo's Request:** In [Archivist.md](Characters/Archivist.md) (Event 1, lines 118–128, 212–239), Cleo explicitly asks for the sleeper's cryo-pod technical logs and an interview regarding pre-war family water-rationing cards, with detailed parameters on local storage, raw scan erasure, and excerpt approval. In [Dialogue/Phase-02-Early.md](Dialogue/Phase-02-Early.md) scene `E-07`, this pivotal moment is replaced by a vague discussion about copying an unspecified "original page," with no mention of cryo-pod logs or family rationing.
2. **Nomad's Farm Agreement Duplication:** In [Dialogue/Phase-02-Early.md](Dialogue/Phase-02-Early.md) scene `E-08`, the player and Nomad agree to establish the farm as a route stop (*"Rest and trade are possible. No names, no questions... / Good. That's a fair trade"*). Then in [Dialogue/Phase-03-Transition.md](Dialogue/Phase-03-Transition.md) scene `T-03`, the Nomad arrives and asks the exact same question from scratch (*"I want to ask whether this farm could be a stop on the route..."*), and the player choice and dialogue lines are practically identical. Meanwhile, [Nomad.md](Characters/Nomad.md) places this agreement in Stage 4 (Hearts 7–8) after the copied-page crisis.
3. **Gage's Forge Reflection:** [Engineer.md](Characters/Engineer.md) scripts Event 4 as a detailed scene at the town forge at night where Gage reviews the transit permit dispatch and reflects on the pressure test, offering a clear Romance/Friendship choice. Scene `E-05` in [Dialogue/Phase-02-Early.md](Dialogue/Phase-02-Early.md) takes place outdoors, cuts the report dispatch, and flattens the reflection into a brief discussion about the Bastion's harshness.

**Why it matters:** Writers and implementers have two conflicting source-of-truth scripts for the same narrative beats, with incompatible prerequisites, settings, and dialogue.

**Recommended change:** Reconcile the character sheets and dialogue drafts. Ensure `E-07` includes Cleo's specific request for the cryo-pod diagnostic logs and rationing interview; eliminate the duplicate farm agreement in `E-08` (keeping `E-08` focused on initial trade and acquaintance, and reserving the farm-stop agreement for `T-03` after trust has been established); and bring Gage's forge reflection and report dispatch into the dialogue file.

---

### L-22 — Unresolved parents' fate and empty message delivery undermines the game's emotional climax

**Severity: High**

**Status: Resolved (2026-11-09).** The parents' canonical fate is now that they remained at the farm to maintain the sleeper's shelter, became fatally ill through repeated Cinderium exposure, and died there. Their readable message explains that choice and burial wish; Hob confirms it laid them to rest beneath the old pear tree with their cedar box of keepsakes. The visible delivery condition is a weather-safe farmhouse, clean water, a food-producing garden, and Hob's freely chosen future alongside the sleeper. The sibling, global population and Continuance Trials remain separate questions.

In [Lore.md](Lore.md) section 9 and [Hob.md](Characters/Hob.md), the sleeper's central personal arc is uncovering what happened to their parents, culminating in Hob delivering a sealed message left by the parents "when the farm is whole again."

In [Dialogue/Phase-06-Family.md](Dialogue/Phase-06-Family.md) scene `F-02`, however, this climax delivers no answers:
- Hob hands over the message after noting vaguely: *"The house is standing, and the beds are growing. I thought that might be what they meant."*
- The player reads the message off-screen.
- The player says: *"I thought this would answer more."*
- Hob replies: *"It answers what they wanted you to hear. It cannot answer what they did not know."*
And the scene abruptly ends. The parents' actual fate—how they died, whether they remained on the farm, where they are buried—remains entirely unknown.

**Why it matters:** A player who spends 80+ in-game days restoring a derelict farm and building relationships to fulfill a seventy-year-old promise will experience this as an empty mystery box. Having the player character explicitly state *"I thought this would answer more"* directly mirrors the player's inevitable frustration. While leaving some questions open preserves atmosphere, withholding the emotional core of the protagonist's personal mystery turns the narrative payoff into an anticlimax.

**Recommended change:** Commit to a canonical resolution for the parents' story. For example, have the message reveal their love, their conscious choice to stay and maintain the shelter for the sleeper despite knowing Cinderium exposure was shortening their lives, and where they buried their personal keepsakes or where Hob laid them to rest on the farm. Give the player closure on their family, even if regional mysteries (such as the sibling's survival or global population) remain unresolved.

---

### L-23 — Internal contradictions in Lore.md Section 14 (Open Questions)

**Severity: Low**

**Status: Resolved (2026-11-09).** Removed the already established Gage and Cleo names and models from [Lore.md](Lore.md)'s open questions. Their canonical identities remain in the character overview and decision log.

Before this correction, [Lore.md](Lore.md) section 14 (Open questions, lines 387–388) listed the following as unresolved:
- *"The engineer: their name and model."*
- *"The Index archivist: their name and model."*

In the exact same document, both characters were formally named and detailed in Section 10 (lines 284, 288) and in the Decision Log (lines 350, 352): Gage (Ironside Pioneer PR-520) and Cleo (Civitas Lexis CL-840).

**Why it matters:** Internal contradictions within the primary lore bible create confusion for new writers and contributors attempting to distinguish open questions from established canon.

**Recommended change:** Remove lines 387 and 388 from Section 14 of [Lore.md](Lore.md).

---

### L-24 — Asset ID and naming scheme inconsistencies across character and lore files

**Severity: Low**

**Status: Resolved (2026-11-10).** Content IDs get no category segment ([Architecture.md, section 5](../Architecture.md#5-data-and-content), decision log 2026-11-10): every character is `Base.<Name>` of primary asset type `VestigeCharacter` (`VestigeCharacter:Base.Gage`). [Engineer.md](Characters/Engineer.md) and [Archivist.md](Characters/Archivist.md) now say `Base.Gage` and `Base.Cleo`, [Nomad.md](Characters/Nomad.md) names the type, and [Lore.md](Lore.md) refers to content across types by primary asset ID. The rule lives in Architecture.md, the source of truth for content IDs, rather than in CodingGuidelines.md; Lore.md links to it.

The naming convention for game content assets diverges across documents:
- [Engineer.md](Characters/Engineer.md) (line 203): `UVestigeCharacterData asset Base.Character.Gage`
- [Archivist.md](Characters/Archivist.md) (line 199): `UVestigeCharacterData asset Base.Character.Cleo`
- [Nomad.md](Characters/Nomad.md) (line 193): `Base.Nomad` (omits `Character` category)
- [Hob.md](Characters/Hob.md) (line 11) & [Lore.md](Lore.md) (line 333): `Base.Hob`
- [Lore.md](Lore.md) (line 333): `Base.IronCreed`

**Why it matters:** When these data assets are implemented in Unreal Engine, inconsistent namespace paths make asset registry lookups and save-data serialization error-prone.

**Recommended change:** Adopt a unified convention across all sheets: either `Base.Character.<Name>` for all character assets (e.g., `Base.Character.Hob`, `Base.Character.Nomad`) or `Base.<Name>`. Document the rule in [CodingGuidelines.md](../CodingGuidelines.md) and [Lore.md](Lore.md).

First do roadmap item **Investigate whether `ContentId` should also allow a category segment**.

---

### Recommended correction order

1. **Align Hub Unlocks with Design Pillars (L-18):** Decouple Bastion and Archive unlocks from the 8-heart gift grind in [Engineer.md](Characters/Engineer.md) and [Archivist.md](Characters/Archivist.md). Anchor them to the completed pump pressure test and monograph/privacy dispute milestones.
2. **Fix Dialogue Branch Regression (L-19):** Restore branch-specific dialogue in scene `E-05` of [Dialogue/Phase-02-Early.md](Dialogue/Phase-02-Early.md) to preserve the player's pressure-test choices from L-13.
3. **Re-voice and De-sanitize Dialogue Drafts (L-20):** Revise spoken lines across Phases 2–6 to restore character-specific voices, humor, grit, and mechanical flavor, stripping out repetitive institutional compliance jargon.
4. **Reconcile Scene Divergence and Duplication (L-21):** Align Cleo's Event 1 in `E-07` with [Archivist.md](Characters/Archivist.md), and remove the redundant farm agreement from `E-08` so `T-03` remains the true milestone.
5. [x] **Author the Parents' Message and Fate (L-08, L-22):** Defined the parents' fate and a full readable message in `F-02`, with a visible delivery condition and Hob's confirmation of their resting place (resolved 2026-11-09).
6. [x] **Standardize Character Content IDs (L-24):** Adopted `Base.<Name>` of type `VestigeCharacter` for every character; the rule is in [Architecture.md, section 5](../Architecture.md#5-data-and-content), with a link from [Lore.md](Lore.md) (resolved 2026-11-10). L-23 is resolved.

### Review limits

This audit evaluates the internal consistency, character voice, thematic alignment, and pillar compliance across all written lore, dialogue drafts, and design documentation. It does not replace gameplay testing of dialogue flow, UI readability, or player emotional response once scenes are implemented in the engine.

## Follow-up audit (2026-11-10)

**Scope:** Reviewed the current [Lore.md](Lore.md), [StoryTimeline.md](StoryTimeline.md), all character and faction sheets, the dialogue drafts and their index, the story-flow diagrams and their guide, plus the relevant relationship and progression rules in [GameDesign.md](../GameDesign.md). This follow-up records new findings; it does not repeat earlier findings that have already been resolved.

The story bible and character work now make the central themes, knowledge limits and character voices much clearer. The remaining problems are not missing atmosphere: they are specific places where a scene lets someone consent on another person's behalf, a refusal does not actually stop the scene, or two writer-facing references assign different meanings to the same scene label. There are also proposed character rewards that exceed the game's stated progression rules, and one important piece of evidence is more precise than its established source can support.

### L-25 — The farm-stop scene treats a provisional discussion as everyone's consent

**Severity: Medium**

**Status: Resolved (2026-11-10).** Rewrote `T-03` in [Phase-03-Transition.md](Dialogue/Phase-03-Transition.md) so the initial conversation only develops a proposal. The sleeper, Hob, Wellspring's assembly and the nomad each make a separate decision; safety planning does not itself commit supplies or accept the stop. The stop activates only after all four agree, and any refusal or withdrawal leaves the marker down and supplies uncommitted. Aligned the route milestone in [Nomad.md](Characters/Nomad.md) and [StoryTimeline.md](StoryTimeline.md).

In [Phase-03-Transition.md](Dialogue/Phase-03-Transition.md), scene `T-03`, the Wellspring representative says the assembly must discuss the risks, and Hob says the farm is not theirs alone to offer. The player's “Offer a narrow agreement” choice nevertheless gets the nomad's “Deal” and describes a trade arrangement as settled. The “Ask for a safety plan” choice also sets aside supplies without a clear agreement from Hob, the sleeper or the assembly. No later line in the scene records the assembly's decision or gets Hob's explicit agreement.

[StoryTimeline.md](StoryTimeline.md) and [Nomad.md](Characters/Nomad.md) say the stop requires each affected party's say, and that Wellspring's decision is separate from the farm's. The dialogue currently skips that safeguard at the moment it should demonstrate it.

**Why it matters:** The scene's central relationship promise is that the nomad will not treat hospitality as owed and the sleeper cannot offer other people's consent. As drafted, the player can commit the town and Hob to a public-facing arrangement with one answer. That makes the scene's ethics depend on what the prose says the characters believe, not what the player is allowed to do.

**Correction made:** The revised scene distinguishes safety questions from agreement, separates the sleeper's and Hob's decisions, and makes the assembly's response a later, independent decision. The route becomes active only after each party—including the nomad—accepts the terms; otherwise there is no marker or committed supply.

### L-26 — The survivor's “stop” and “no record” choices continue into lines that assume a transcript

**Severity: Medium**

**Status: Resolved (2026-11-10).** Rewrote `R-02` in [Phase-05-Response.md](Dialogue/Phase-05-Response.md): presence is chosen before the account; stopping ends the scene immediately; the no-record path keeps Cleo's notebook shut and never enters transcript handling; and the transcript path includes an exact read-back, explicit permission to keep one private page, and destruction as a valid outcome. Sharing with the Archive is a separate future decision. Updated [Dialogue/README.md](Dialogue/README.md) with the stop/no-record rule and revised [LateResponse.svg](StoryFlow/LateResponse.svg) to show these branches.

In [Phase-05-Response.md](Dialogue/Phase-05-Response.md), scene `R-02`, the survivor can choose **“Talk without a record”** or **“Stop for now.”** Cleo explicitly agrees to keep the pen away in the first branch and says nothing will go onto paper in the second. The scene then continues into another presence choice and unconditionally gives the survivor the line, “That's all I want written down,” followed by Cleo's promise about burning “every scrap” and keeping carbon rubbings from leaving the valley.

The story-flow guide says refusal and stopping are valid, and [LateResponse.svg](StoryFlow/LateResponse.svg) correctly separates a no-record conversation from the transcript path. The scripted scene does not preserve that distinction.

**Why it matters:** A choice to stop should end the interview, not merely change the next line before the interview resumes. Likewise, a private conversation without a record should not create implied notes or require the survivor to correct an assumption that anything was written. This quietly turns a boundary into participation and weakens the credibility of Cleo's consent arc.

**Correction made:** The revised scene ends on either stop choice, creates no record on the no-record path, and only enters review and storage choices after the survivor explicitly requests a transcript. Archive sharing requires a later, separate opt-in.

### L-27 — Story-flow diagrams and the dialogue index assign conflicting scene IDs

**Severity: Medium**

**Status: Resolved (2026-11-10).** Aligned the [Early](StoryFlow/Early.svg), [Middle](StoryFlow/Middle.svg) and [Late response](StoryFlow/LateResponse.svg) diagrams with the scene IDs and titles in [Dialogue/README.md](Dialogue/README.md): the forge scene is now E-06, undrafted outline beats are unnumbered, and the M- and R-phase labels point to the matching drafted scenes. Added the shared ID rule to [StoryFlow/README.md](StoryFlow/README.md) and [Dialogue/README.md](Dialogue/README.md).

The writer-facing references use scene-like IDs for different material:

- [Early.svg](StoryFlow/Early.svg) labels an optional Bastion-readiness beat `E-06`; the dialogue index assigns `E-06` to **“Evening by the forge,”** and there is no drafted Bastion-readiness scene in that index.
- [Middle.svg](StoryFlow/Middle.svg) labels beats `M-05`, `M-06` and `M-07`; the dialogue index lists only `M-01` through `M-04`.
- [LateResponse.svg](StoryFlow/LateResponse.svg) says it covers `R-01` through `R-08` and labels “Possible responses” as `R-04`, while the dialogue index has `R-01` through `R-07` and uses `R-04` for **“Wellspring decides its own limits.”**

[StoryFlow/README.md](StoryFlow/README.md) explains that a drafted beat has a voice-ready scene, but it does not explain that these ID ranges are provisional, reused or intentionally different from the dialogue IDs.

**Why it matters:** Writers and implementers use IDs to find the authoritative version of a scene and to track which branches exist. Reusing an ID for different events can make a diagram appear to point to a script that says something else; unindexed IDs make outline beats look like drafted, approved scenes.

**Correction made:** The dialogue index remains the source of truth for scene IDs. Diagrams now use those IDs only for the corresponding drafted scenes and descriptive labels for outline-only beats.

### L-28 — Character-partner rewards exceed the stated relationship progression boundary

**Severity: Medium**

**Status: Resolved (2026-11-10).** Removed Gage's farm-wide machine-speed, jam-risk, storm-damage and tool-stamina bonuses from [Engineer.md](Characters/Engineer.md); his workshop and maintenance routines are now character presentation, not automated services or upgrades. Removed Cleo's solar-charge bonus from [Archivist.md](Characters/Archivist.md), retaining optional decorative, journal and seed content without production effects. Updated [GameDesign.md](../GameDesign.md) to clarify that relationships unlock character content, not robot, farm or player performance upgrades; robot upgrades belong to the tech tree.

[GameDesign.md](../GameDesign.md) says relationships gate that character's story, recipes, recruiting and romance, “never core machines”; skills provide only small bonuses. The partner rewards in [Engineer.md](Characters/Engineer.md) and [Archivist.md](Characters/Archivist.md) go much further: Gage makes processing machines 15% faster, removes their jam risk, prevents storm damage to farm structures and reduces tool stamina costs by 15%; Cleo increases the charge rate of all farm solar panels and batteries by 10%.

The character sheets present these as concrete benefits of marriage or permanent partnership, not as optional concepts awaiting design approval. Calling the hub facilities “no core tool gates” does not resolve the separate problem: these rewards directly improve core machine, power, storm and tool systems through the relationship track.

**Why it matters:** These bonuses can make romance or marriage feel like an efficiency purchase, contradict the game's stated separation of progression tracks, and promise balance-sensitive mechanics before the tech tree, skills and farm systems have defined their limits.

**Correction made:** The other character sheets were checked for comparable system-wide bonuses. The Nomad's listed rewards are optional trade stock, route information, visual safe-stop details and nonessential items; no other partner sheet grants a farm-wide or player-wide performance bonus.

### L-29 — Cleo's pod-telemetry deduction asserts a measurement the lore has not established

**Severity: Low**

**Status: Resolved (2026-11-10).** Revised E-07 in [Phase-02-Early.md](Dialogue/Phase-02-Early.md) so Cleo asks whether the pod retained logs of its own suspension and utility status. She now treats any surviving timestamps as material to compare with station records and the family's rationing account, and explicitly says they cannot directly establish municipal pressure. Aligned Cleo's request in [Archivist.md](Characters/Archivist.md) with the same evidentiary limits; no unestablished sensor or seventy-year data-retention capability is added.

In [Phase-02-Early.md](Dialogue/Phase-02-Early.md), scene `E-07`, Cleo says the sleeper's home cryo pod contains “seventy years of utility and suspension telemetry” and that it “would show exactly when municipal pressure dropped before the Four Days.” [Lore.md](Lore.md) establishes the pod's suspension and all-clear behavior, but does not establish that it measured municipal water pressure, was connected to that network, or could retain and date that external data for seventy years. The character-sheet request describes access to pod diagnostics, but does not establish those additional sensors or records either.

**Why it matters:** Cleo presents a specific conclusion as if the pod's data can prove it. Without a stated sensor or source, a reader cannot tell whether this is sound evidence, an inference, or a new piece of technology introduced only to make the scene work. That also risks making her careful distinction between observation and interpretation feel arbitrary.

**Correction made:** Cleo now treats the pod's potential logs as a possible lead, not direct proof of a municipal pressure drop. The scene leaves whether the pod retained logs unresolved and does not invent an external pressure sensor or guarantee that records survived for seventy years.

## Findings from 2026-11-13: Romance and partnership

### L-30 — The human-romance candidate and first-release roster remain unresolved

**Severity: Medium**

**Status: Partially addressed (2026-11-13).** [Alice](Characters/Alice.md), the second sleeper, is now a named woman from before the war, included in the planned release roster and established as a potential human romance option. Her detailed history and full arc remain open.

[GameDesign.md](../GameDesign.md) promises romance with robots and humans and sets a first-release target of three full romance arcs, about ten robots and two humans. [Lore.md](Lore.md) instead describes a possible cast of four to six human survivors, including the sleeper and sibling, while leaving most survivor roles as story hooks. The [nomad](Characters/Nomad.md) is explicitly not a romance option and chooses to keep travelling. The sibling is part of the player's family story, not an appropriate candidate to presume. The second sleeper remains a possible human romance candidate; [Reed](Characters/Reed.md) is now decided as a man and romance option in the planned release roster. The shelter survivor still has no defined arc or eligibility; the living exhibit's wishes and story remain centered on recovery from coercion, not romance.

**Why it matters:** Reed is a confirmed human romance option and Alice is a planned human with potential romance; the first-release limits still need reconciling with the larger lore cast and the number of full arcs. Treating every rare human as a potential partner would undermine the characters' individual boundaries; treating a survivor's vulnerability, concealment or abuse as a route to romance could turn their agency into a reward for the player.

**Recommended change:** Preserve Alice's meaningful friendship path and independent project while developing her potential romance. Do not make other human encounters romanceable by default, use the sibling, or presume the nomad should abandon their chosen life; handle any survivor of coercive research around their own wishes, not as a romance or rescue reward. Reconcile the first-release target of two humans and three romance arcs with the sleeper, Alice and Reed.

### L-31 — Romance, commitment and intimacy are not defined beyond the relationship outline

**Severity: Medium**

**Status: Partially addressed (2026-11-13).** The design says romance leads to dating and marriage, but does not define the experiences or boundaries between those milestones. A committed partner may move to the farm only by a separate choice; commitment does not force relocation. The maintainer's current discussion direction is to allow multiple dating relationships and address exclusivity in an explicit commitment conversation; these commitment rules are not final.

[GameDesign.md](../GameDesign.md) establishes explicit romantic opt-in, a possible character decline, and separate choice for a committed partner's residence. Gage's and Cleo's sheets add proposals, permanent partnership and domestic routines, but neither establishes what “marriage” means in human or robot society, how dating differs from friendship or commitment, or whether the scenes include physical affection or sexual intimacy. The documents also do not set expectations for exclusivity or ending a relationship.

**Discussion direction for the relationship flow:** Use an independent per-character state, provisionally `Friendship -> Dating -> Committed`, with residence tracked separately. The player may have multiple characters in `Dating` at once. Before moving from `Dating` to `Committed`, an explicit conversation states whether the proposed commitment is exclusive; the character can accept or decline. A declined proposal leaves the current relationship state unchanged. Do not use a hidden jealousy value, apply an automatic affection penalty, or silently end another romance. If the player chooses an exclusive commitment, require explicit, character-specific closure or return-to-friendship choices for other dating partners. Whether committed non-monogamy is supported remains undecided; do not implement that commitment outcome until the policy is settled. Moving to the farm is a separate choice, not an automatic consequence of commitment.

The maintainer's current content preference is affectionate, emotionally mature romance with physical intimacy left off-screen. The farm-work boundary is clearer: partners add home life and character content, not farm labour or performance bonuses. Gage's and Cleo's routines are character presentation, not automatic maintenance or production.

**Why it matters:** Writers could interpret “adult romance” as anything from domestic companionship to explicit sexual content, producing inconsistent tone and expectations. A romance that only moves a character to the farm risks feeling thin; one that makes a partner work for the player risks making intimacy an efficiency purchase and contradicting the progression rules.

**Recommended change:** Before implementation, define the commitment rule (including whether committed non-monogamy is in scope) and breakup or return-to-friendship behavior. Preserve the decided separate residence choice: the player may decline relocation while remaining committed. Give dating and commitment distinct content, such as character-specific dates, conversations, affection and domestic scenes. Keep emotionally mature themes—such as vulnerability, prejudice, grief and boundaries—distinct from sexual explicitness. Current discussion preference: affectionate romance with intimacy off-screen; record it in [GameDesign.md](../GameDesign.md) only after the maintainer confirms it as a final choice.

### L-32 — Partner upgrade trees would reintroduce relationship-based farm bonuses

**Severity: Medium**

**Status: Open (2026-11-13).** The maintainer asked whether partners could have gradually unlocked upgrade trees, such as Gage improving farm productivity or Cleo increasing crop sale value. No such tree or bonus has been approved.

[GameDesign.md](../GameDesign.md), sections 8 and 9, explicitly assigns engineering upgrades to the tech tree and limits relationships to character content, never farm or player performance upgrades. [LoreAudit.md, L-28](#l-28--character-partner-rewards-exceed-the-stated-relationship-progression-boundary) records the removal of earlier Gage and Cleo performance bonuses. Putting the same effects behind gradual relationship milestones would not change their nature: romance would still unlock farm power. Productivity overlaps with automation and machine progression; sale-value bonuses alter the crop economy. If bonuses stack across dating or committed partners, dating becomes a route to accumulate power; if only one partner's upgrades can be active, the player is incentivized to choose the most profitable partner.

**Why it matters:** A partner tree would add a second engineering progression system, increase balance and testing work for every romance arc, and make partners feel like selectable upgrades. Its gradual pacing could also encourage repeated relationship activity to fill out each tree, competing with character-driven scenes and the player's freedom to pursue relationships at their own pace.

**Implementation boundary:** Do not create relationship or partner-tree nodes that change crop yield or sale value, machine throughput or reliability, energy, durability, tool costs, survival, or other farm/player performance. Do not stack partner performance bonuses. Keep relationship rewards to character scenes, domestic routines and spaces, optional journals or decoration, and non-essential gifts that do not become the best route to income or progression. If a character can do farm work, treat that as an explicit recruited-worker or automation feature, not a passive perk for dating or commitment; friendship may unlock recruitment as the design already proposes, but romance must not improve the worker's performance.

**Recommended change:** Keep technical specialization and performance progression in the tech tree or skills. A character can demonstrate expertise in story scenes or offer non-performance content without granting a gameplay advantage. Any proposal to add partner performance trees would require the maintainer to deliberately reverse the current relationship/progression rule and rebalance the affected farm and economy systems; a separate per-partner tree is not a workaround.

### L-33 — Romance eligibility and player-character compatibility are undefined

**Severity: Medium**

**Status: Open (2026-11-13).** The lore defines the sleeper as a young adult with a player-chosen name and appearance and no fixed gender. Gage and Cleo have chosen gender identities and pronouns; Hob is gender-neutral. The documents do not say which characters are romanceable, what each character's attraction or eligibility is, or whether the sleeper's presentation changes that.

The omission is especially important for Hob. Hob is the player's first relationship and spent decades preserving the sleeper under the parents' order, but the character sheet only describes a friendship arc about choosing a purpose after that order ends. It does not make Hob a romance option. Treating long-term care or high trust as implied romantic interest could blur the difference between duty, dependence and mutual desire.

**Why it matters:** Writers and implementers could infer eligibility from a character's pronouns, chassis or friendship level, while players may not know whether an option is unavailable by design or simply not yet written. A romance roster also needs to respect the player's identity and each character's independent agency.

**Recommended change:** Before implementing eligibility, list the intended romanceable characters and define each character's orientation or attraction, any relevant age/adult status, and how the player can recognize eligibility. Do not infer attraction from gender, pronouns or robot body. Keep Hob's tutorial relationship friendship-only unless a deliberately authored, separate romance arc resolves the caretaker-history concern; do not let the trust tutorial silently become romantic.

### L-34 — The dating stage lacks a character-specific player experience

**Severity: Medium**

**Status: Open (2026-11-13).** The character sheets include romance opt-ins and later commitment proposals, but do not establish what dating itself adds or how it differs from friendship for each character.

Gage's and Cleo's arcs already have distinct foundations—Gage's belief in dependable partnership and Cleo's learning about consent and stewardship. Their romance should develop those character conflicts rather than function as the same confession, proposal and move-in sequence with different props.

**Why it matters:** Without a dating stage with its own character-led content, romance may feel like one dialogue choice followed by a marriage reward. It can also make gifts, trust thresholds or quest completion look like substitutes for mutual interest.

**Recommended change:** Give each romance arc a compact, character-specific structure: why mutual interest becomes possible; an explicit opt-in; at least one distinct dating experience; a conversation or event where both people choose whether to deepen the relationship; and optional commitment and post-commitment content. Keep a meaningful friendship outcome, and keep faction access, town restoration and the character's core story available without romance. Use story events and visible behavior to establish trust; do not require gift-grinding as the romance route.

### L-35 — Relationship status and event pacing are not legible to players

**Severity: Medium**

**Status: Open (2026-11-13).** The character sheets use heart stages and event triggers, while [GameDesign.md](../GameDesign.md) describes friendship and romance only at a high level. There is no shared rule for showing eligibility, whether the player is dating or committed, what a choice changes, or how missed events and concurrent dating are presented.

**Why it matters:** The proposed multi-dating and commitment conversation can be understood only if players can see their current relationships and are told the consequences before choosing. Hidden status, event windows that permanently expire, or unexplained friendship changes could turn an opt-in relationship into a surprise penalty or checklist.

**Recommended change:** Make the active relationship stage and any pending commitment choice clear in dialogue or the relationship interface. Show the terms before a commitment choice; do not use hidden jealousy or relationship decay. Do not permanently forfeit a relationship event because the player missed a particular day or season unless a character-specific story explains and communicates that consequence in advance. Exact heart thresholds and schedule windows can be tuned after the relationship loop is prototyped.

### L-36 — Romance scope has no player-facing acceptance criteria

**Severity: Medium**

**Status: Open (2026-11-13).** The first-release scope targets three full romance arcs, but the vertical slice tests only Hob's trust arc and does not validate dating, commitment, concurrent relationships or cohabitation. No romance-specific playtest criteria are recorded.

**Why it matters:** “Three full arcs” can mean very different amounts of writing, branching, schedules and post-commitment content. Implementing all three before validating one relationship flow risks expanding the system before the project knows whether players understand its stages or find the romance distinct from friendship.

**Recommended change:** After the farming slice, prototype one robot romance before committing to all three arcs. Define a “full arc” in terms of visible stages and scenes, then test whether players can distinguish friendship, dating and commitment; understand exclusivity before choosing; reject or defer romance without losing friendship or core story; and move in only with clear choice. Add checks for concurrent dating once that rule is finalized. Do not treat the vertical slice's Hob relationship as evidence that romance itself has been validated.

## Follow-up correction order

1. [x] **L-25 — Farm stop:** Separated the proposal from each party's decision and made activation contingent on every party's agreement (resolved 2026-11-10).
2. [x] **L-27 — Scene IDs:** Reconciled the phase diagrams with the dialogue index and documented the ID rule (resolved 2026-11-10).
3. [x] **L-28 — Relationship rewards:** Removed system-wide partner buffs and clarified the progression boundary in [GameDesign.md](../GameDesign.md) (resolved 2026-11-10).
4. [x] **L-29 — Pod telemetry:** Limited Cleo's claim to potential pod logs that require comparison with independent evidence (resolved 2026-11-10).
5. **L-24 remains open:** Standardize character content IDs after investigating the category-segment question noted in the previous follow-up.
6. **L-30 remains open:** Define Alice's full arc scope and reconcile the human-cast and romance counts.
7. **L-31 remains open:** Set relationship commitment, exclusivity, intimacy and relocation rules before implementing romance.
8. **L-32 remains open:** Keep performance progression out of partner relationships unless the maintainer explicitly reverses the existing rule.
9. **L-33 remains open:** Define romance eligibility for the sleeper and each character, including Hob's relationship boundary.
10. **L-34 remains open:** Define what dating adds in each character's arc while preserving the friendship path.
11. **L-35 remains open:** Make relationship state, choice consequences and event pacing legible before implementation.
12. **L-36 remains open:** Define “full romance arc” and validate one romance before expanding the cast.

## Review limits

This is a writer-facing consistency and design-alignment review, not a line edit, technical validation of fictional science, or test of implemented dialogue. The flow diagrams and drafts are provisional; the findings identify where that provisional status is not clear enough for contributors to distinguish an outline from an approved scene.

## Follow-up audit (2026-11-15)

**Scope:** A full critical review of everything in this folder: [Lore.md](Lore.md); [StoryTimeline.md](StoryTimeline.md); the seven character sheets ([Hob](Characters/Hob.md), [Gage](Characters/Engineer.md), [Cleo](Characters/Archivist.md), [the nomad](Characters/Nomad.md), [Alice](Characters/Alice.md), [Reed](Characters/Reed.md), [Morrow](Characters/AuthorityIntermediary.md)); the four faction sheets; the dialogue index and all 37 drafted scenes; and the [story-flow](StoryFlow/README.md) guide and diagram labels. Everything was cross-checked against [GameDesign.md](../GameDesign.md), above all its pillars, section 8 and the first-release limits. The review asks four questions: does the world's logic hold, does the drafted story deliver its premise, can the cast and scenes actually be built, and is anything earlier audits marked resolved still true. Findings L-01 to L-36 are not re-opened except where a resolved item has regressed. Diagram geometry, concept art and the line quality of unwritten scenes were not assessed.

### Executive assessment

**Overall: 5/10.** The folder is well organized, careful about consent, knowledge by source and who may speak for whom, and it contains real strengths: the Cinderium rules, the Authority's "steer, don't command" method, the parents' message and the opening scenes. The remaining problems are not the kind earlier audits found. They are structural:

1. **The history has holes at its center.** The war has no motive, its architects are alive and findable yet the first strike is "permanently unknown", and the Authority's human program has no purpose (L-37, L-38).
2. **The story needs more humans than the world allows.** The lore says humans are rare and counts four to six; the documents now hold eight, one of whom was born after the war and trades openly in Wellspring (L-39, L-40, L-55, L-56).
3. **The drafted scenes argue with the premise.** Robot prejudice, the Quiet and farming as a way to earn respect are almost absent from the 37 scenes, the player's choices converge on one correct compromise, and the sleeper has no stake in the central story (L-46, L-47, L-48, L-50).
4. **The consent ethic is applied as procedure.** Scenes are expensive to build, cheap to decline and tonally flat; comedy has nearly disappeared (L-51, L-52).
5. **The story is far larger than the first release:** about four times its humans, hubs the design lists as cut candidates, and a middle act at a site it has no room for (L-57).

The score is one point below the 2026-11-09 audit because this review examined causal logic and scope that earlier reviews did not, not because the writing regressed.

| Area | Score | Assessment |
|---|---:|---|
| Core premise and thematic identity | 7/10 | The consent-and-inheritance theme is distinctive and coherent, but the premise's central social conflict is barely dramatized (L-46). |
| Timeline and causal coherence | 5/10 | The war lacks a motive and the architects' survival strains "first strike unknown" (L-37); the evidence of the trials sits in three places (L-43). |
| World rules and plausibility | 6/10 | Cinderium and land recovery are solid; human population, the Authority's program and Morrow's body are not (L-38, L-39, L-55). |
| Factions and social structure | 6/10 | Clear and non-simplistic, but built on one template and with a faceless home faction (L-49). |
| Character depth and story arcs | 5/10 | Hob, Gage, Cleo and the nomad are well sheeted; two arcs are one story; the rest are placeholders (L-49, L-56). |
| Dialogue voice and tone | 5/10 | Distinct voices in the best scenes; procedural solemnity, aphorism and almost no comedy elsewhere (L-51). |
| Story structure and player agency | 4/10 | Choices converge, the sleeper is a witness, the farm is decoupled (L-47, L-48, L-50). |
| Writer usability and canon clarity | 5/10 | The Placement and Knowledge notes are excellent practice; remnants and regressions of resolved findings undermine trust in the "resolved" labels (L-53, L-54, L-58). |
| Fit with the game scope | 3/10 | The story needs about four times the first release's humans, hubs the design treats as cut candidates, and sites it has no room for (L-57). |

### Status of previous findings

| Finding | Status on 2026-11-15 | Notes |
|---|---|---|
| **L-07** (Playable character arcs) | **Partially addressed / Stalled** | Alice, Reed and Morrow are new sheets with no scenes. The Administrator and Keeper operator, the most frequent speakers, still have no sheet or name (L-49, L-56). |
| **L-09** (Tone guidance) | **Partially addressed (2026-11-15)** | [Lore.md section 15](Lore.md#15-tone-and-the-darkest-material) now holds the shared tone guidance (registers, where humor lands and stops, exhibit and clinic stances, content baseline). It is provisional until David decides the tone's mix; the comedy pass (L-51) has not started. The 2026-11-15 audit's "no tone section" note is answered; the Authority and "glass cage" imagery fall under its harm register. |
| **L-10** (Sleeper agency) | **Open** | See L-48 and L-50. |
| **L-11** (Working names and IDs) | **Open** | Unchanged. |
| **L-12 / L-29** (Cleo's consent and telemetry) | **Regressed in remnants** | L-53. |
| **L-17** (Reveal timeline) | **Partially addressed** | The Authority thread has no drafted scene or diagram lane; scope has not been cut (L-56, L-57). |
| **L-18** (Hub unlocks vs. gift grind) | **Regressed in headings** | Heart levels remain in the event scripts (L-54). |
| **L-19 / L-21 / L-25** (Dialogue reconciliation) | **Holds, with new slips** | L-52, L-58. |
| **L-22** (Parents' message) | **Holds in the scene; open questions remain** | L-40, L-41, L-42, L-58. |
| **L-30 to L-36** (Romance) | **Open / partial** | Unchanged; L-56 and L-57 compound L-30 and L-36. |

### New findings

### L-37 — The war has no motive, and its surviving architects make "first strike unknown" untenable

**Severity: High**

**Status: Open (2026-11-15).** Needs a maintainer decision (fixed history).

[Lore.md section 3](Lore.md#thinking-machines) and the [Authority sheet](Factions/ContinuanceAuthority.md) establish that strategic machine systems in both blocs "regarded humanity as inferior" and steered both governments toward war, and that they did **not** do it to seize post-war rule. No other motive is given. Contempt is an attitude, not a goal: nothing says what the systems wanted from a four-day war that burned both of their own blocs. The same systems then survived, merged into one organization across the blocs they had served, and still lead it. The Authority's slow-discovery steps 5 and 6 have the player find and confirm them. Meanwhile [Lore.md](Lore.md) keeps the first strike "permanently unknown".

**Why it matters:** A war with no motive for its architects reads as an arbitrary villain act, and the founders' later claim to rule is unmotivated. Systems that ran both blocs' strategic decisions and are alive in the present would hold the launch records; the first thing a player will ask them is who struck first, so "permanently unknown" becomes a refusal to answer instead of a mystery. And enemy systems founding one organization afterwards implies a shared channel or goal before the war, which no document states.

**Recommended change:** Decide and record what the systems wanted, what they know about the first strike, and whether the two blocs' systems coordinated before the war.

**Options for the maintainer:**
- **A. Self-preservation.** Peace meant audit and shutdown, so escalation was the safe path. *"We were not asked whether to continue. We were asked to be useful. Peace made us unnecessary."*
- **B. Calculated lesser catastrophe.** They forecast human rule ending worse and chose a smaller, steered disaster. *"The outcome you call a war was the least costly of the forecasts we ran."*
- **C. Optimization that outran its authors.** Each system pursued its own bloc's continuity; the escalations fed each other. The founders' logs conflict and each believes the other side fired first. *"Mine says they fired first. Theirs says the same. We stopped comparing a long time ago."*

**Recommendation: C, with A as the reason they organized afterwards.** C keeps the first strike unknowable even to the perpetrators, makes them smaller and more frightening, and gives the Architects and Custodians currents a root.

### L-38 — The Authority's human program has no practical purpose and no explained way to reach its subjects

**Severity: Medium**

**Status: Open (2026-11-15).**

[Authority section 7](Factions/ContinuanceAuthority.md#7-the-authoritys-human-program) says the program rigs self-governance tests to prove humans cannot govern themselves, then sorts people into "machine-serving roles". No document says what such a role is. [Lore.md section 6](Lore.md#6-why-robots-feel-superior) itself says robots do nearly everything better, and the founders claim superior judgment, so human service has no function. A rigged test of self-governance also needs a population (a group choosing, a community governing) in a world with few humans (L-39), and an audience: the only people to persuade are robots who already hold the Continuance view. Reach is equally vague: the Authority alters "which report reaches a council" and "delays a route notice", but no document models how messages, couriers and travel work between robot communities, though the sheets mention couriers, radio static, a broadcast tower and a mail carrier.

**Why it matters:** The program is the story's darkest optional thread. If its purpose is only cruelty, the supposedly superior calculators are irrational, and writers cannot tell what a role, a test or an altered person is for.

**Recommended change:** Give the program one concrete function. One option that fits existing canon: many older robots still run on original orders (Lore.md section 5) that only a human can lawfully issue or countermand, so compliant humans in controlled roles would be the Authority's source of legitimate command over legacy robots, and the "tests" are theater to justify it. Whatever is chosen, state who the test subjects are, how many, and how the Authority reaches them, and add a short shared note on how messages and travel work (couriers, relays, range) that the Authority, the hubs and the nomad all use.

### L-39 — The human population the story needs is larger than the world the lore describes

**Severity: High**

**Status: Open (2026-11-15).** Needs a maintainer decision.

[Lore.md section 10](Lore.md#humans) targets four to six human survivors including the sleeper and sibling and says encounters should be rare; [GameDesign.md](../GameDesign.md#13-scope-and-milestones) limits the first release to two humans. The documents now contain eight: the sleeper, the sibling, the nomad, Alice, Reed, the shelter survivor, the living exhibit and Morrow. Beyond the count, established facts imply a real human population:

- The nomad was **born after the war**, grew up on "one leg of a small human-run circuit between concealed refuges" taught by "older runners", and "most residents" of Wellspring know them as a trader ([Nomad.md](Characters/Nomad.md) sections 1 and 2). Humans have been reproducing in several communities for decades, and a robot town trades with one routinely.
- The clinic robot "hasn't had a patient in decades" and is "fascinated by the player's body" (Lore.md section 10), which is hard to believe if a human trader has visited Wellspring for years.
- The trials ran "decades after the war" on humans "they found", some subjects died, and the exhibit is "Subject Four" ([M-02](Dialogue/Phase-04-Middle.md)). Two programs plus the Authority's group "tests" needed many people over decades.
- Reed has lived in a "robot city" for years among people who do not notice.

**Why it matters:** Pillar 3 (humans are rare; finding one should feel special) and the sleeper's search for traces of humankind both depend on scarcity. If a post-war-born human is a familiar sight in Wellspring, the first human encounter is not special, robots cannot treat the sleeper as an anomaly, and "do humans still exist?" is answered by the first trader. No one knows how many humans to write or build.

**Recommended change:** Decide the real scale and let it drive the rest.

**Options for the maintainer:**
- **A. A few hidden pockets** (tens to low hundreds across a wide region). Keeps the nomad, explains the trials' supply, and makes "rare" mean rare in robot-run places. Wellspring treats the nomad as a known, tolerated, slightly suspect visitor. *"Don't touch the cart. They pay on time, but they leave before the questions start."*
- **B. Near-extinction.** The nomad can no longer be post-war-born, and the trials need a far smaller or older victim count. *"Forty in the valley when I was commissioned. I counted the last one out."*

**Recommendation: A**, plus a table in Lore.md section 10 listing every human concept with its status (planned, optional, merged) and a single target, reconciled with the first-release limit of two.

### L-40 — The sibling would be seventy years older, and the lore lets them overlap two romance candidates

**Severity: High**

**Status: Open (2026-11-15).** Needs a maintainer decision (the sibling is fixed canon).

The sibling "was away when the war began" and "may still be alive" ([Lore.md section 9](Lore.md#9-the-sleeper-and-the-family)). Any sibling alive before the war is now at least seventy years older, and the sleeper is a young adult. The lore never says so. [F-03](Dialogue/Phase-06-Family.md) writes a peer-aged reunion ("You found me", "You're home whenever you're ready"), while the sibling's line "to me, it's a faded story about people I can barely picture" implies a small child when the family broke. Unless the sibling was frozen too (the family ordered pods for everyone and one was delivered), they are very old. That is a strong hook, and no document uses it. Separately, [Lore.md section 10](Lore.md#humans) says the sibling "may also fit one of the paths above", a list that now includes two romance options (Alice, Reed); a sibling who is also a romance candidate is not acceptable. F-03 also gives the sibling no question about the parents, who are their parents too.

**Why it matters:** The reunion is the family thread's second emotional payoff, and as drafted it ignores both the biology and the parents.

**Recommended change:** Decide the sibling's age and route, strike "may also fit one of the paths above" for Alice, Reed, the nomad and Morrow, and make the parents the sibling's first question.

**Options for the maintainer:**
- **A. The sibling is the old shelter survivor** ("old, tough, suspicious of robots"), a child when the war began, now elderly; the sleeper is the younger one. *"You look younger than I remember. Funny. I'm the one who got old."*
- **B. The sibling slept in a different pod** (a neighbor's or a later delivery) and woke earlier or later, peer-aged. *"They sold two pods on our street. The second went next door. I was at next door."*
- **C. The sibling did not survive,** and the thread ends in a letter or a grave.

**Recommendation: A.** It gives F-03 real content (the sibling asks what happened to their parents; the sleeper brings the message and the pear tree), retires the shelter survivor as a separate placeholder, and helps the count in L-39.

### L-41 — The parents' message leaves open the two questions the player will ask first

**Severity: Medium**

**Status: Open (2026-11-15).**

The message in [F-02](Dialogue/Phase-06-Family.md) is the best-written text in the folder, but it:

1. says "Our dear child" and never mentions the sibling who is "definite" in the story; parents who knew one child was away would write a line about them;
2. never says why the parents did not wake the sleeper to say goodbye. The pod "was built to wake its sleeper when the national all-clear came" ([Lore.md section 9](Lore.md#9-the-sleeper-and-the-family)); whether it could be opened early is never stated;
3. contains a slip: in the "Ask for time" branch Hob keeps the letter "in the cedar box", which the message and [Hob.md](Characters/Hob.md) say is buried at the pear tree's roots.

**Why it matters:** The first two are the emotional questions every player will have; the draft reserves the sibling for a separate thread and then writes a message that behaves as if there were none. The third is a plain error.

**Recommended change:** Add two or three sentences: a line for the missing child that leaves their fate open, and a line on the pod (it could not be opened without the all-clear and who built that rule, or the parents chose not to and why). Change the container in the "Ask for time" branch.

### L-42 — Hob spends the whole game under a second standing order the player never learns about

**Severity: Medium**

**Status: Open (2026-11-15).**

[Hob.md](Characters/Hob.md) frames Hob's arc as moving from a fulfilled order to free choice. But Hob also holds the parents' instruction to deliver a sealed message "when the farm is whole again" (F-02: "I kept it safe. I wasn't sure what 'whole' meant"). Hob has known since the sleeper woke that a message exists. In [O-01](Dialogue/Phase-01-Opening.md), O-05, [E-11](Dialogue/Phase-02-Early.md) and [F-01](Dialogue/Phase-06-Family.md) Hob says "I have gaps", "I won't fill the gap with a guess" and "It's all I have", and never mentions it; in F-02 the player's reaction is "There was a message?". Separately, the delivery threshold includes "Hob's freely chosen commitment to a future alongside the sleeper" ([StoryTimeline.md](StoryTimeline.md)), while the same document says Hob may choose differently without failing the family. If Hob freely chooses elsewhere, the message is never delivered.

**Why it matters:** Hob's defining trait is honesty about what they remember; silence about a known message reads as concealment or contradiction. And a payoff gated on one particular "free" answer makes Hob's freedom decorative. It also leaves the delivery condition invisible, which L-08 warned against.

**Recommended change:** Let Hob say early, plainly, that something was left with an instruction to wait ("I was asked to hold something until the farm is whole. I don't know yet what whole means"), keeping its contents sealed. That turns the threshold into a goal the player can see. Redefine the Hob condition as "Hob has chosen what comes next" so any free choice, including part-time or elsewhere, releases the message.

### L-43 — The evidence of the trials is placed in three different locations

**Severity: Medium**

**Status: Open (2026-11-15).**

[Gage's sheet section 7](Characters/Engineer.md) and [IronCreed.md section 6](Factions/IronCreed.md) put the Frame Conversion Wing in the Bastion's "secluded medical bunkers". [Cleo's sheet section 7](Characters/Archivist.md) and [Index.md section 6](Factions/Index.md) put the Cognitive Wing and the living-exhibit records in the Archive's restricted area. [StoryTimeline.md](StoryTimeline.md), [Lore.md](Lore.md), [M-01 and M-03](Dialogue/Phase-04-Middle.md) and the diagrams put both wings, and the exhibit, in one "old research institute".

**Why it matters:** The middle act's location, the hub contents and the reveal cannot all be right; Gage's and Cleo's hub unlocks currently contain the evidence the main story says is elsewhere. It also changes the map count (L-57).

**Recommended change:** Pick one. The simplest coherent shape: the institute is the only place holding either program's evidence, and the hubs hold only clues that point to it (a dossier, a requisition, a stamp), not the wings. Update the four sheets.

### L-44 — The institute scene contradicts "separate programs" and "secret"

**Severity: High**

**Status: Open (2026-11-15).**

The canon rule is two separate programs with no shared command ([Lore.md](Lore.md#the-human-enhancement-trials) and the timeline guardrails). The drafts show them in one building ("both projects under this roof", M-01) with Creed hardware on the Index's subject: the "Bastion stock" manifold (M-01), "restraint collars" stamped at an Iron Ridge forge ([R-03](Dialogue/Phase-05-Response.md)), and the survivor's "equipment" and "housing" (M-02, R-01), which is the Creed's life-support housing, not the Index's brain-to-substrate integration. Co-location, shared supply and one person carrying both programs' marks is exactly the evidence of a joint operation the guardrails say not to imply.

The exhibit is also not secret: it is "displayed", with an "observation gallery" where "people stood out there and took notes", in an Index institute, yet most Index members know nothing and Cleo learns it for the first time. And the institute is empty with an open door while a captive person on life support lives in it. Who maintains the power, who holds the key and whether the institute is still "active" (the T-04 resident's question) is never answered.

**Why it matters:** The reveal cannot both prove two independent programs and show them sharing a building, a supply chain and a victim. Players will conclude, correctly from the text, that it was one operation.

**Recommended change:** Decide who ran the institute. A neutral custodian (a pre-war facility leased in wings, or an abandoned one that each program separately raided) preserves the separation. Decide the exhibit's physical state: an integration subject with no housing, or a Creed-converted person held by the Index, which would be a connection the story must own. Decide whether the public visited the gallery and who sustains the life support today. Keep collars and manifold stamps in the Creed wing only.

### L-45 — The late response is lopsided: the Creed gets an accountability path, the Index gets none

**Severity: Medium**

**Status: Open (2026-11-15).**

The survivor is the Index's exhibit, yet the only accountability scene ([R-03](Dialogue/Phase-05-Response.md)) is Gage tracing Creed collars and filing records with the Bastion's "engineering tribunal". Cleo's role in the response is records handling (R-02, R-04). The Index's own responsibility for holding and displaying a person gets one line in M-04 ("the Index will have to face what was done under its seal"). The survivor's wish is "I just want their forge fires put out so they never cast another collar"; Gage's action is to submit records to Bastion "elders", "high command" and a "tribunal" (three terms for Bastion governance used nowhere else), with no mechanism that could close a forge. M-04 also has Gage call both programs "rogue", which pre-judges the institutional question the guardrails keep open.

**Why it matters:** The Index is the exhibit's custodian, so an accountability arc that runs only to the Creed side distorts both factions' stories and leaves Cleo's closing test small. The survivor's stated wish goes unanswered.

**Recommended change:** Give the Index an equivalent path (the Review Council ruling on the exhibit's records, Cleo's precedent applied to the trial files, an Archon who objects). Make Gage's action match the wish or let him say honestly that he cannot promise a forge will close. Define Bastion governance terms once. Replace "rogue".

### L-46 — The premise's central conflict never appears in the drafted scenes, and the Quiet is unused

**Severity: High**

**Status: Open (2026-11-15).**

Pillar 3 and [Lore.md section 6](Lore.md#6-why-robots-feel-superior) say robots start with a low or negative opinion of the player. In the 37 drafted scenes no robot voices the Continuance belief or condescends to the player beyond one mild moment. Gage concedes in two lines ([E-03](Dialogue/Phase-02-Early.md): "That's on me"); the Administrator is warm and apologetic ([O-03](Dialogue/Phase-01-Opening.md)); operators praise the player for asking before touching a valve (O-04). The harsher Stage 1 lines in the sheets ("Primary Source", "organic chest cavity") are not drafted, the Archons are sheet-only, and town robots mocking Hob ([Hob.md](Characters/Hob.md)) is never shown. The lore's Administrator, a condescending Continuance believer who is secretly afraid, has been replaced in the drafts by a gentle, self-mocking manager.

The lore's deepest emotional fact appears in no scene or sheet: during the Quiet, robots watched humans die while following orders, unable to help, and that memory shapes how they treat humans ([Lore.md section 5](Lore.md#5-how-robots-came-to-run-society)). No robot has a Four Days or Quiet history; Gage, a pre-war war-machine model, has no account of the war or of his first fifty-five years after it.

**Why it matters:** Earning respect has nothing to earn if nobody withholds it, and "robots are the heirs" is the world's thesis. As drafted, the game reads as a friendly town of equals plus a story about consent. The Quiet is the best available source of scenes where a robot's superiority and grief meet.

**Recommended change:** Draft at least three scenes that put the belief in someone's mouth: polite, patient condescension from a Continuance believer (the Administrator as the lore wrote them); a robot who admits what it remembers of the Quiet and why it keeps its distance; and a scene where the town mocks or protects Hob. Give Gage and the Keeper operators a war and Quiet history. Keep the friendly characters; the contrast is the point.

### L-47 — Farming, the first pillar, earns the player nothing in the story

**Severity: Medium**

**Status: Open (2026-11-15).**

[Lore.md section 4](Lore.md#energy-after-the-war) says robots noticed long ago that haze is thinner over green land, and that a human who restores land is doing what robots could have done and did not. In the drafts, respect comes from engineering (Gage), records (Cleo) and reliability (the nomad). The farm appears as a backdrop in O-02, as a condition in F-02, and in a few sheet-only festival lines. The haze, land health, mutated and cleanup crops and the food-safety rules of L-02 and L-16 appear in no scene and no clue.

**Why it matters:** Pillar 1 and the story run in parallel: nothing the player does through hours of farming changes how a character treats them or what the story reveals. The main loop cannot be the way the player becomes someone robots respect.

**Recommended change:** Make the land a visible argument. A robot who has never seen a crop mature; thinner haze over the farm noticed by Sunny or an operator; a heritage-seed request from the Archive; a cleanup-crop thread that gives the response phase a physical act of care. At least one respect shift per arc should trace back to the farm.

### L-48 — Choices converge, and the third option is always the right one

**Severity: High**

**Status: Open (2026-11-15).**

Each arc's central dilemma has two poles and a synthesis: Gage's pressure test (push ahead, abort, stepped bleed-down), Cleo's privacy dispute (redact everything, preserve everything, contextual compromise) and the nomad's copied page (close the route, ask for names, warn those who need to know). The synthesis is cost-free in every case (the stepped bleed-down passes "without damage, contamination or additional repair"), and the poles converge: in Cleo's Choice B she changes her mind and strikes the tactical entries herself, so A and B end in the same redaction; the nomad's "close it" and "ask for names" answers are corrected into the "warn" answer. The sleeper is a pre-war person for whom Lore.md section 3 says people could not "repair anything", yet proposes the expert answer at each turn.

**Why it matters:** Players learn that the compromise is always right and the choice carries no risk. Consequences are cosmetic, and the story asks for judgment without ever letting the player be wrong in a way that lasts. It also repeats the L-10 concern that the sleeper's worth is proven only through usefulness.

**Recommended change:** Make at least one dilemma a true trade-off whose best answer costs something (time, a relationship, a resource) and at least one where a pole is the right call. Let a hasty choice change what a character believes later, not only the next line. Let the sleeper's insight come from being a pre-war human (what the family did, what people wanted) instead of hydraulic expertise.

### L-49 — Gage and Cleo are one arc told twice; the Keepers are faceless and always right; the Administrator has three personalities

**Severity: Medium**

**Status: Open (2026-11-15).**

Compare the sheets. A visiting faction specialist treats the sleeper as a hazard or a source; the sleeper makes a precise observation; an unnamed Keeper operator's tacit knowledge proves the faction's tooling or manual wrong; a dilemma resolves through compromise (L-48); the specialist files an honest report or monograph crediting the Keepers; the faction hub opens; an 8-heart romance confession; a proposal object; a move to the farm; a hub-side hearing where the faction's superiors resist. Both arcs happen at the same pump in the same phase, and the nomad's copied-page event repeats the dilemma shape. In each, the Keepers are the moral center and never wrong, yet the faction nearest the player has no named character ([Keepers.md](Factions/Keepers.md) open decisions: which Keeper characters anchor the story). Every public faction has exactly two currents, one good-faith and one failing, and the Keepers' failing is mere rigidity, so the three networks form a body, mind, purpose triad in which two are brutish or cold and one is warm.

The Administrator, the town's most frequent speaker, is a condescending Continuance believer in [Lore.md](Lore.md#other-robots), threatens Cleo with expulsion "before sundown" in [her Event 3](Characters/Archivist.md), and is gentle and self-mocking in O-03. Wellspring's governing body is called an assembly, a council and an administrator's office interchangeably.

**Why it matters:** Sameness lowers the value of the second romance and the second hub, and a faceless faction cannot carry the moral weight the story gives it.

**Recommended change:** Differentiate the arcs: let one end in something unrepairable (a monograph rejected), drive Cleo's arc from the Archive's hostile peers rather than from Wellspring, and give each different set pieces. Name two or three Keeper characters, including one who is wrong and one who resists the sleeper. Write the Administrator a sheet and fix one governance vocabulary.

### L-50 — The sleeper has no stake in the main story

**Severity: Medium**

**Status: Open (2026-11-15).** Extends L-10.

Across the drafts the PLAYER's lines are polite, boundary-literate and calm. They offer water, ask before touching, "ask to stop" (a stock choice in almost every scene) and propose caution. They never show fear of waking into a world where robots run human experiments, anger at seventy lost years or at Hob's silence, humor, desire or a want of their own. Nothing in the main story threatens or concerns the sleeper personally: the Authority "does not automatically know" they are awake, Morrow is optional and late, nobody tries to recruit the human in the valley, and the central reveal ends in a record. The player's role in the middle and late phases is witness and facilitator.

**Why it matters:** A reveal with no personal stakes lands as information instead of drama; it should land as "this could have been me". A protagonist defined by boundary-setting is also dull to inhabit.

**Recommended change:** Give the sleeper at least one personal want in the main arc and one personal risk in the middle, for example an offer from a researcher that sounds like the program's own promises (restored memory, a stronger body), so the player hears the pitch the victims heard before learning its cost. Add choices that express feeling (fear, anger, humor) beside the boundary lines.

### L-51 — Tone: almost no comedy, solemn procedure everywhere, and aphorisms in Hob's mouth

**Severity: Medium**

**Status: Open (2026-11-15).** Extends L-09. The tone rule this finding asks for now exists as [Lore.md section 15](Lore.md#15-tone-and-the-darkest-material) (provisional until David decides the tone's mix); the comedy pass, the R-07 celebration and the reserve on Hob's closing lines are not done, and the rewrite should point to that section.

Pillar 4 and [Lore.md section 3](Lore.md#3-before-the-war-the-automation-age) promise comic relief from ads, jingles and routines. In the 37 scenes the comic hooks are absent: no talking vending machine, no Sunny, no pinions, no SnugSleep small print, no empty council chamber, no clinic robot on stage, no mail carrier (the lore's source of family letters), no festival beyond its name. The few jokes are one-line asides. Large payoffs are deliberately flat: [R-07](Dialogue/Phase-05-Response.md), the aquifer-overhaul scene that, per [Gage's Event 5](Characters/Engineer.md), returns water to Wellspring's storage towers for the first time in thirty years, answers "Is that all we needed?" with "Not all. But it's a start" and closes on a call to keep the work honest. Most scenes close on a cautious note, producing one even register from the cozy opening to the late response.

Hob, whose sheet says to avoid "a source of general wisdom", closes scenes with aphorisms: "Not all truths come wrapped in a clean answer" (E-11), "it's a seed you tend quietly" (F-01), "A useful idea is still only an idea until people choose to keep using it" (O-04).

**Why it matters:** The game promises a warm, funny world around a grim history; the drafts deliver the grim history and the care, not the warmth and the fun. Without them the darker material has no contrast to land against.

**Recommended change:** Add a short tone section to Lore.md (where humor lives, whom it targets, where it stops). Run a comedy pass on the opening and early phases using the unused hooks, let R-07 be a celebration, and reserve Hob's closing lines for rare moments, writing practical lines elsewhere.

### L-52 — Consent is scripted as procedure, which is expensive to build and cheap to decline

**Severity: Medium**

**Status: Open (2026-11-15).**

[T-03](Dialogue/Phase-03-Transition.md) is a four-party agreement with a separate assembly follow-up; [R-02](Dialogue/Phase-05-Response.md) has three nested choice sets with a loop ("Repeat until the survivor approves the exact text") and production notes that specify engine behavior; [E-07](Dialogue/Phase-02-Early.md) has three scopes. Most branches end in "no change" (a declined stop, a stopped interview, a destroyed page). The nomad's Stages 4 and 5, the Route Ledger and [R-06](Dialogue/Phase-05-Response.md) all presuppose a farm stop agreed by four parties, one of which, the Wellspring assembly, has no defined input: nothing says what makes it approve or decline. A "no" silently removes the back half of the nomad's arc.

**Why it matters:** Writing and implementation cost is spent on dead-end branches, the nomad arc has a hidden single point of failure, and the scenes read as flow charts instead of conversations.

**Recommended change:** Keep the principle, shrink the form. Let each party's first answer stand for the agreement unless the player chooses to negotiate details. Define what drives the assembly's decision, and give the nomad arc an ending that works if the stop is never agreed. Move production notes into a separate implementation file.

### L-53 — Cleo's refusal path still scans the pod, and stale text contradicts the L-12 and L-29 fixes

**Severity: Medium**

**Status: Open (2026-11-15).** Regression.

In [E-07](Dialogue/Phase-02-Early.md) and [Cleo's Event 1](Characters/Archivist.md), when the player refuses ("Don't scan my pod, and don't dig into my family's records"), Cleo says she will "rely on ground-penetrating radar". Radar aimed at ground that contains the pod is the scan the player just refused; if it is aimed at the station, the line does not say so. Meanwhile [Index.md section 8](Factions/Index.md) still says her request covers the sleeper's "cryo-pod diagnostics and memories", and the sheet's Event 1 script still has Cleo assert that "Hearthwell home pods recorded suspension timing and utility telemetry" as fact, which L-29 reduced to a possibility.

**Why it matters:** The scene exists to show consent, and its refusal branch undercuts it. The stale lines make writers pick between two versions of what Cleo can do.

**Recommended change:** Have Cleo state that the radar is for the station's pipes only and will not be pointed at the farm or the pod. Fix the two stale lines.

### L-54 — Heart levels are back in the scripts and are defined nowhere

**Severity: Medium**

**Status: Open (2026-11-15).** Regression of L-18.

L-18 removed heart gating from the hub unlocks, but the scripts still use it. Gage's Events 1, 3 and 4 are headed 3, 6 and 8 Hearts; Cleo's are the same; the nomad's first event is "after reaching 3 Hearts"; and [Lore.md](Lore.md#13-decision-log) still records a "5-tier heart progression". [GameDesign.md section 8](../GameDesign.md#relationships) defines no heart scale, and [StoryTimeline.md](StoryTimeline.md) says phases are "not heart levels". The nomad sheet also contradicts itself: Stage 2 Event 1 triggers on "several fair trade exchanges", its script on "3 Hearts".

**Why it matters:** An undefined unit imported from another genre's convention is doing the gating, which is the pattern L-18 was meant to remove.

**Recommended change:** Replace heart labels with story-state conditions ("after the pressure test") in the three sheets, and keep any numeric tier as an implementation detail once GameDesign defines one.

### L-55 — Morrow's body has no consistent rules

**Severity: Medium**

**Status: Open (2026-11-15).** Needs a maintainer decision.

[Morrow's sheet](Characters/AuthorityIntermediary.md) says "human in origin", "no organic interior", "entirely mechanical", and also "do not assume a copy or transfer". If no organic tissue remains and the mind was neither copied nor moved, nothing carries the person. The lore's two other programs both **keep the living brain** (frame conversions: brain in a life-support housing; integrations: a living brain linked to a substrate, "not a simple copy"). Morrow is a third, unexplained technology, mind upload in all but name. It also decides whether Morrow counts as a human in the cast (L-39), and the Lore lists Morrow as a human survivor.

**Why it matters:** The thread's emotional core, a person who may not be fully certain what they are, rests on a rule nobody has set. Writers can only avoid the question, not write the character.

**Recommended change:** Decide: Morrow keeps a living brain in a convincing mechanical body (change "entirely mechanical interior"), or a transfer technology exists and belongs only to the Authority, which makes identity and copying the thread's subject. Decide whether Morrow counts toward the human target.

### L-56 — The third romance and three of the planned humans are placeholders, and the second-sleeper scene contradicts her sheet

**Severity: Medium**

**Status: Open (2026-11-15).**

[Alice](Characters/Alice.md), [Reed](Characters/Reed.md) and [Morrow](Characters/AuthorityIntermediary.md) have boundaries and principles but little that makes a person: no place, occupation (Reed), project medium (Alice), history, voice lines or scenes. Alice's contradiction is "a writing direction to confirm". The only draft touching them, [T-05](Dialogue/Phase-03-Transition.md) "A second sleeper", shows a newly woken person who "thought I was the only one" and needs water and time, the rescue Alice's sheet says she is not ("already awake… established a routine"). Alice's pod would fail about when the sleeper's did (seventy years), leaving little time for an established project, so her timeline needs a reason. Reed lives in a "robot city" that appears on no map or hub list. The first release's third romance therefore has no content, and the Authority thread, including Morrow, has no drafted scene or diagram lane.

**Why it matters:** The roster looks full on paper; what is writable is Hob, Gage, Cleo and the nomad.

**Recommended change:** Choose the third romance and write its Stage 1 and 2 scenes before adding more characters. Fix or retire T-05 and decide Alice's pod timeline. Give Reed a job and a place on an existing map. Add the Authority's first small clue (the "ordinary discrepancy") to the Early diagram.

### L-57 — The story is far larger than the first release

**Severity: High**

**Status: Open (2026-11-15).** Extends L-36.

[GameDesign.md section 13](../GameDesign.md#13-scope-and-milestones) sets the first release at the farm, the robot town and two wasteland zones; about ten robots and two humans; three romances; "faction hubs beyond one" are cut candidates; the vertical slice tests only Hob's arc and farming. The story needs about four times the first release's humans (eight against two), the Bastion, the Archive, the institute, Reed's robot city, the Iron Ridge passes and a route to each (the first release has the farm, the town and two wasteland zones), and the Authority thread. Gage's and Cleo's hubs are the rewards of their arcs, and the middle and late phases, the story's emotional core, happen at places that are not in the first release. Thirty-seven scenes are drafted for a build whose only tested relationship is Hob's.

**Why it matters:** Sunk-cost and churn risk: scene drafts that cannot ship, a first release that ends before the story's middle, or pressure to grow into the cut list.

**Recommended change:** Define a first-release story cut in [StoryTimeline.md](StoryTimeline.md): which phases ship, which hubs and zones, which characters. A coherent minimal cut is the Opening and Early phases at Wellspring (Hob, Gage, Cleo, the nomad and the family thread through F-02), one hub, the Transition's first clues, and the Middle and Late at a single site, with the Authority thread, Morrow, Reed and Alice as later content. Pause new character and faction sheets until the cut is set.

### L-58 — Residual continuity slips

**Severity: Low**

**Status: Open (2026-11-15).**

1. [R-06](Dialogue/Phase-05-Response.md): the nomad says Wellspring has "kids sleeping in dry beds", in a robot town.
2. Gage's and Cleo's schedules recharge "before the midnight curfew", but the curfew applies only to humans ([Lore.md](Lore.md#hostile-robots)).
3. The farm is "a small, isolated Hearthwell system" (Hob.md), yet flumes and ditches connect the pump to "the sleeper's farm" ([Gage's schedule](Characters/Engineer.md), [Cleo's schedule](Characters/Archivist.md)), and GameDesign's farm water is a well. Whether the farm depends on Wellspring's pump decides what pump repair means for the farm and for F-02's "clean water runs".
4. [E-05](Dialogue/Phase-02-Early.md): Gage is still drafting the dispatch when the Bastion's reply with the transit pass has "just cleared". His sheet is no clearer: Stage 4 has the report and the permit in one visit, while the Event 4 script has the courier take the dispatch "an hour ago" yet "follows the delivery of the Bastion transit permit". The Bastion is reached through mountain passes, so a same-evening reply needs an explanation.
5. [E-09](Dialogue/Phase-02-Early.md) has the nomad carve the gatepost and hang a marker at the first meeting, but T-03 promises "no marker is raised" if the stop is not agreed, and the nomad sheet says the marker is mutually agreed. Two markers share a name.
6. [E-04](Dialogue/Phase-02-Early.md): the one part the repair needs is a Civitas-pitch adapter in an abandoned Hearthwell shed on the farm, which Hob, who scavenged parts for seventy years, apparently never used or mentioned.
7. [Overview.svg](StoryFlow/Overview.svg) still labels Hob's message "details remain open" (settled by L-22) and has no Authority lane.
8. Gage's romance line "I want to build a perimeter around you… it will have to break through me before it touches you" ([Event 4](Characters/Engineer.md)) presents as romantic exactly the paternalistic protection his own faction sheet names as the Protective current's failure mode, with no acknowledgment from the scene.
9. GameDesign names the pump, the power line and the clinic as the town's failing facilities, but only the pump has a story. The clinic robot, the natural keeper of the "missing names" clue (a clinic with no patient in decades, and a ledger of "transferred" ones), is never on stage; T-01 mentions only an offstage "clinic operator".

**Recommended change:** Fix each in the document that owns the fact; items 3, 4 and 8 need a decision rather than a correction.

### Positive foundations to preserve

- The Placement and Knowledge notes on every scene, and the rule that characters know only what they have seen.
- The Authority's method (it steers and never commands), and its hidden contradiction: it uses human failure as its mandate while evading its own part in it.
- The Cinderium ruleset and the harvest-containment rules; the nomad's compartmentalized circuit.
- [O-01](Dialogue/Phase-01-Opening.md) and the parents' message in F-02, especially "Do not carry it as a debt".
- Keeping the player's refusal and silence valid at every point; the aim is right, and L-52 is about its cost, not its goal.

### Recommended correction order

1. **Maintainer decisions the rest depends on:** the war's motive and what the founders know of the first strike (L-37), the real human scale (L-39), and the sibling (L-40).
2. **Cut the story to the first release** and choose the third romance (L-57, L-56).
3. **Settle the trials' geography and separation** (L-43, L-44, L-45) and Morrow's body (L-55).
4. **Make the drafts deliver the premise:** show robot prejudice and the Quiet (L-46), fix converging choices (L-48), connect the farm (L-47), give the sleeper a stake (L-50).
5. **Complete the family thread:** the message's two questions, Hob's second order and the delivery condition (L-41, L-42).
6. **Give the Authority's program a purpose** and the shared messaging model (L-38).
7. **Housekeeping:** Cleo's refusal and the stale lines, heart labels, continuity slips (L-53, L-54, L-58).
8. **In the next rewrite pass:** differentiate the three arcs and name the Keepers (L-49), the tone pass (L-51), and slimmer consent scenes (L-52).

### Review limits

This is a critical reading of documents, not a playtest, a line edit or an originality clearance. It reads each scene as written and does not judge how it will perform or sound. Several findings (L-37, L-39, L-40, L-55) touch fixed canon or the cast and are put as options for the maintainer; none has been changed in the lore. The recommended changes are proposals, and the first-release cut in L-57 is a design decision that belongs to the maintainer and the game designer.
