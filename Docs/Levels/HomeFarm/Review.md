# HomeFarm plan review

**Review date:** 2026-11-14
**Scope:** `Docs/Levels/HomeFarm/README.md` and `HomeFarm.svg`; design review only. No level or map was opened in Unreal, so this is not a measured or play-tested review.

## Assumption and verdict

The request names no Roadmap item, so I assumed this is an early blockout review for the vertical slice: the farm work in Roadmap 4.2, caretaker onboarding in 4.4, and the water source, sprinkler, and shipping bin in 4.5. The plan itself correctly says to duplicate `/Game/Maps/L_Farm`, survey it, and treat the diagram as relative rather than measured.

**Verdict:** good and unusually careful blockout brief, but not ready to be treated as a final route plan. Keep the agreed full-walkable footprint and the house beside the shared approach. Improve the plan by treating its “daily loop” as a set of optional, legible connections—not a prescribed itinerary—and by making first-day orientation and the field/service approaches the actual blockout acceptance criteria. No gameplay system or new level feature is needed.

## Player experience and pillar

On the first morning, the player should recognize home, the 8×8 field, and a nearby helper as the immediate farm; they can reach the bed and every field edge without searching or squeezing past props. On later mornings, tending the field and fetching/selling should be short choices, not a compulsory circuit through every zone. This serves *Bring the land back* through a readable work area and practical farm services, and *Your pace* by keeping home and routine work easy to reach. The open yard can provide visual breathing room, but should not become a long, empty walk.

## What is already decided

- **GameDesign.md §2:** farming and land restoration are central; the game should let players choose how to spend limited days.
- **§4 and §9:** tending the farm is part of the daily loop, but the rest of the day is a choice; the slice ends with a sprinkler and season funds, not a larger social-town map.
- **§8:** the caretaker is loyal and has a relationship arc; the level can stage a welcome, but cannot supply that arc or invent dialogue.
- **§10 and §13:** the farm is the largest map; the vertical slice proves the farming core. This does not authorize a town exit or additional gameplay stations.
- **§14, 2026-11-14 entries:** the maintainer chose a fully walkable planned footprint and a farmhouse beside the shared approach, outside the field apron. Preserve these decisions.
- **§15:** no open question currently settles the detailed farm layout.
- **Roadmap 4.2, 4.4–4.5:** contamination/water, caretaker onboarding, and farm services are planned; the plan properly uses nonfunctional placeholders until those systems exist.
- **GameDesignAudit.md:** no finding specifically reviews this level layout. Its broader scope and “Your pace” cautions remain relevant.
- **PlayTest.md:** no failed or commented HomeFarm blockout check exists yet. Existing field-edge and camera-related checks concern the current prototype, not this proposed layout.

## Findings and recommendations

1. **The drawn “daily loop” reads as mandatory travel.** Its named route orders arrival → home → field → service yard → open yard → home, even though a player will not need to visit all of those every day. Repeatedly walking past an irrelevant yard would make farm chores feel longer and fight *Your pace*. Keep the connected route and all destinations reachable, but describe the arrows as circulation options. Validate short direct connections from home to field and from field to services, plus a comfortable optional circuit through the yard. Do not add a gate, objective, or station to make the circuit meaningful.
2. **The opening view is specified, but the first-minute read is not.** A clear sightline to home and field is a strong start. Before accepting the blockout, verify from the actual Player Start that the field entrance and home/bed are distinct, recognizable destinations and that the caretaker’s standing spot is visible or easy to find without standing in the traffic line. Hob’s first-days tutorial is already planned (Roadmap 4.4); do not add tutorial scripting or new signs as a level-only workaround. The bed stays a sleep destination, not a required first interaction.
3. **The service yard’s role is easy to overstate.** The diagram shows water and shipping placeholders together; the README correctly says they may not function yet. Keep them visually neutral and labeled as blockout in the Editor, and do not have the route imply that either is usable. Reserve only the service access and the small sprinkler-test opportunity already requested; do not claim a permanent sprinkler footprint or reserve tiles before placement/range rules exist.
4. **The whole footprint being walkable is not a reason to make the farm oversized.** The full-walkable choice is settled, and an open yard can help the cozy pacing. It should still be close enough that it reads as connected breathing room, not a distant empty district. Since the diagram has no measured scale, decide its actual extent only after measuring the field and walking the duplicate map.
5. **The plan appropriately avoids fake precision, but the field-apron width was still fixed at one tile pitch in the diagram.** The “P” anchor and survey-before-layout are right. Do not copy apparent path widths or convert illustration pixels to centimeters. The diagram also gives no terrain elevations, grades, or camera height; the actual floor and routes must be checked in Play. Check pawn clearance, field targeting, camera visibility, floor support, boundary safety, and interaction approach; the survey alone cannot prove these.
6. **The prototype arrival and story opening are different.** The diagram placed the current outdoor Player Start at the arrival court, but the established story opening is waking from a cryo pod under the farmhouse (GameDesign.md §3; Roadmap story milestone). Treating the outdoor Player Start as the eventual opening would contradict that premise. Preserve it for the present blockout and explicitly reserve the later farmhouse-to-farm route without inventing the unplanned interior or sequence.
7. **The original drawn route crossed the farmhouse massing.** That makes the route read as if it passes through the building and offers no clear arrival at the home. The diagram now keeps the approach outside the massing; the actual entrance and porch remain undecided until the house is designed.
8. **The original optional return path crossed the caretaker's standing spot.** That contradicts the plan's requirement to keep the welcome area out of the walking line. The drawn path now passes below the placeholder pad; verify real clearance after the caretaker's collision size is known.

## Recommendations applied

The user explicitly asked for improvements as appropriate. The HomeFarm plan now:

- Treats routes as optional direct connections and keeps the yard circuit optional, preserving the agreed walkable footprint.
- Requires a first-view and findability check for home, field, and caretaker without adding signs or scripting.
- Sizes the field apron from pawn, targeting, and camera tests instead of assuming one tile pitch.
- States explicitly that the diagram gives relationships rather than dimensions, orientation, elevation, or slope guidance.
- Keeps the arrival-to-field path outside the farmhouse massing and treats the yard as an optional connection, not a required return route.
- Keeps the optional yard route clear of the caretaker's welcome spot in the diagram.
- Distinguishes the current outdoor prototype Player Start from the later cryo-pod opening under the farmhouse.
- Keeps the initial layout compact and avoids assigning gameplay to placeholder zones.

The welcome spot remains a staging area, not a new mechanic. Its exact position is still a visual test against the actual Player Start, not something the relative diagram can prove.

## Numbers and loop budget

No tuning values are proposed. This plan adds no crops, tool uses, money, stamina costs, or growth times, so it does not change tile income, 28-day seed payback, or daily stamina/hour budgets. Before setting any spatial dimensions, measure tile pitch, pawn capsule, and field bounds in the duplicate map as the plan directs. Do not choose a route length or yard size from diagram pixels.

## Changes made

The route choice is recorded in `Docs/GameDesign.md` because the user authorized improvements and the optional-connection recommendation follows the existing *Your pace* pillar. The existing farmhouse placement and full-walkable-footprint decisions are unchanged. The narrative-opening note reconciles the plan with already-established story and roadmap content; it does not design the opening sequence.

No map was opened or play-tested in Unreal. The updated plan is still a proposed blockout, not approval of a finished map.

## Manual checks for PlayTest.md

When the duplicate blockout exists, add only a short feel/read check (the exact sightlines, walkability, and collision checks can be inspected or automated where practical):

- Start a new game at the actual Player Start. Without instructions, identify home/bed and the crop field; then find the caretaker welcome spot without blocking the route.
- Walk directly from home to the field, work each edge and corner tile, and return home. Nothing forces a detour through the service yard or open yard; no collision or camera angle hides the field edge or bed interaction.
- Walk from the field to each clearly marked service placeholder and back without crossing crop rows. Confirm the placeholders do not look functional before their mechanics exist.
- Walk the optional yard circuit once. If it feels like empty commute rather than a useful choice or pleasant breathing room, shorten the route/footprint while keeping it connected and walkable.

These checks support the vertical slice’s first-time-player farming goals in GameDesign.md §13: orient quickly, tend crops, make the farm feel approachable, and keep the player’s daily choices open. Do not claim the slice’s crop-growth, sprinkler, caretaker-relationship, or season-completion goals are validated by a blockout walk.
