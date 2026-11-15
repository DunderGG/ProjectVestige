# Home farm master plan

**Status:** forward-looking layout plan for an Editor blockout
**Map to work from:** duplicate `/Game/Maps/L_Farm` as `/Game/Maps/L_Farm_Blockout`
**Scope decisions:** the whole planned farm footprint is walkable in the first blockout; the farmhouse zone is near the shared approach and outside the field work apron. Routes are optional connections between destinations, not a prescribed daily circuit.

![2.5D master layout for the home farm](./HomeFarm.svg)

See the [critical design review](./Review.md) for the risks found, rationale for the route changes, and checks to run when the blockout exists.

## What this plan is for

Use the layout as a guide for building the *whole home farm area*, not as a record of the few actors already in `L_Farm`. It shows where the arrival, home, crops, farm services, and open yard fit together, how the player moves among them, and which spaces to prepare as simple blockout.

This is one connected playable farm map. It is not the robot town or a route to another map. The farm's planned water source, sprinkler, shipping bin, farmhouse, and caretaker are not all implemented yet: show their places with temporary, clearly named blockout shapes, and do not make a placeholder look like a finished or functional feature.

## Scale anchor

The diagram is relative, not a promise of Unreal world-space dimensions or actor transforms.

- Use `P` for the measured center-to-center distance between adjacent farm tiles. The existing field is 8 by 8 tiles, so it is the scale anchor for the rest of the layout.
- In the Editor, inspect the grid's settings and transform; confirm `P` against the positions of neighboring tiles. Record the result in centimeters before laying out paths and pads.
- Check the current pawn capsule radius, tool-targeting reach, and camera view in Play. The field apron needs standable ground beside every edge tile; set its width from those tests, not from the illustration. Shared routes need at least the pawn diameter plus turning clearance. Make the caretaker's welcome spot large enough for two characters to stand without blocking a route.
- Treat the diagram as a relationship map: it has no world-space dimensions, compass direction, or terrain elevations. Use the measured field as a visual ruler to place the zones, then record the proposed zone sizes and route widths in `P` and centimeters before building the full blockout. Do not convert illustration pixels to dimensions.
- Check that the actual ground supports the whole route network. The diagram does not specify slopes, steps, ramps, or camera height; avoid steep or obstructed route transitions until the pawn and camera have been tested in Play.
- Keep the first blockout compact enough that the home, field, and services read as one farm. Add open-yard space only where it improves sightlines, turning room, or a useful route choice; do not expand just to fill the landscape.

## What already exists

- The working field in `L_Farm` is an 8 by 8 grid.
- `BP_Bed` (`AVestigeBed`) is a temporary sleep point along the field's near edge; the plan moves it into the home zone when the farmhouse blockout is ready.
- The current outdoor Player Start, field bounds, bed transform, landscape size, and opening camera view must be surveyed in the Editor. The diagram gives a proposed arrangement, not their current coordinates.
- The current outdoor Player Start is a prototype anchor, not the final story start. The established opening has the player wake from a cryo pod under the farmhouse. That opening is a later roadmap item; preserve room for a clear, walkable route from the farmhouse to the farm, but do not invent its interior, reveal, or scripted sequence in this blockout.

The project already plans the water source, sprinkler, shipping bin, farmhouse, and caretaker robot. A planned feature can be represented by a labeled placeholder, but its final mechanics, mesh, interaction, and exact dimensions are not established by this level plan. See [Roadmap.md](../../Roadmap.md) and [GameDesign.md](../../GameDesign.md).

## Master layout

The labels below use relative sides of the diagram. Rotate or mirror the arrangement to fit the surveyed map; keep the relationships between zones.

1. **Arrival court and welcome spot.** Keep the surveyed Player Start as the prototype blockout's arrival anchor unless the opening sequence is being built as a separate task. Keep the first view open toward the home and field. Make the caretaker robot's welcome spot visible or easy to find from the start, but keep it beside—not in—the walking line. This is a staging location, not a dialogue trigger or a new building.
2. **Home / farmhouse zone.** Place the farmhouse massing near the shared approach, outside the field apron. Keep its ground footprint walkable in the blockout. When the house is ready, move the bed placeholder into the home zone and verify that the sleep interaction still works. Do not invent rooms or a final house footprint yet.
3. **Crop zone.** Keep the current 8 by 8 field as the focal point, with clear ground around it for targeting neighboring tiles. Keep the busiest walking route outside this apron. Leave a few neighboring tiles available to test the planned sprinkler after its placement and range rules exist; do not mark a permanent tile reservation before then.
4. **Service yard.** Group the planned water source and shipping bin beside the shared route, close to the crop zone but outside its work apron. A player should be able to refill water or deliver a harvest without cutting through planted rows. Use simple labeled placeholders until their assets and interactions are ready.
5. **Open farmyard.** Keep the remaining part of the planned footprint walkable and connected. Use it as breathing room and an optional longer connection around the home, field, and service yard; do not leave it as an inaccessible "future area" or make it a required return route. No additional gameplay station is required there.
6. **Farm boundary.** Use temporary terrain shapes or non-blocking markers to make the extent of the farm readable. Keep the boundary continuous for this blockout; do not add a town exit or an unapproved route to another map. The readable edge is a boundary, not a playable cliff or an invitation to enlarge the level.

## Routes and circulation

Treat circulation as a small network of direct choices, not a route the player must complete each day. Show these as clear, optional connections in the ground blockout, not narrow decorative strips:

- **Home to field:** provide a direct route between the home and field apron; reaching the crops must not require crossing the service yard or open farmyard.
- **Home to arrival:** provide a clear, direct connection to the current arrival court.
- **Field to services:** connect the field apron to each planned service placeholder without crossing planted rows.
- **Optional yard circuit:** keep a comfortable return route through the open yard for players who choose to wander or take the longer path. Do not add a station, gate, objective, or detour to justify it.
- **Field-edge access:** keep continuous standable ground around all sides of the field so edge and corner tiles can be targeted. The apron itself provides this access; a separate path strip is not required.

All destinations are reachable in the first blockout. Avoid dead ends, forced backtracking, narrow gaps between props, and collision that hides an interaction prompt. The route lines in the diagram indicate connections only; they do not prescribe direction or a daily itinerary.

## Landmarks and first-play staging

- **From the current arrival:** the home massing and field should read as the two main destinations. The caretaker welcome spot should be visible or easy to find without standing in the traffic line.
- **From the field:** the house should remain a visible homeward landmark; group water and selling placeholders together so the service yard reads as one secondary destination.
- **At the field:** use the grid itself as the work landmark. Preserve visual and physical access to its full perimeter.
- **From the farmhouse:** the home should connect naturally to the farm's shared routes. When the planned cryo-pod opening is implemented, test its start and exit separately; the current outdoor Player Start does not validate that story arrival.
- **At the bed:** the bed prompt, rather than a new sign or scripted cue, identifies the current sleep interaction.

Do not add tutorial scripting, custom landmarks, or decoration in the first blockout pass. The aim is to judge the map's scale, sightlines, and walking routes before art.

## Editor build order

1. **Protect the existing map.** In the Content Browser, duplicate `L_Farm` and work in `L_Farm_Blockout`. Do not move or save over the original while surveying.
2. **Survey and measure.** Record the Player Start, field and bed transforms, grid bounds, tile pitch `P`, pawn capsule size, and the camera's opening view. Use these measurements to scale the diagram.
3. **Mark the full playable boundary.** Block out the farm floor and readable perimeter with temporary shapes. The entire outlined area should be traversable; no exit is implied.
4. **Lay direct connections first.** Connect home to field, home to the current arrival, and field to each service approach. Add the optional yard circuit only after these short routes work. Use broad, simple ground strips or temporary meshes; Play the map before adding zones and widen any path that catches the pawn or caretaker-sized placeholder.
5. **Place zone massing.** Add clearly named placeholder volumes for the home, service yard, arrival court, and open yard. Keep the field apron free. Put the caretaker standing spot next to the approach.
6. **Check the first read.** Start Play from the surveyed Player Start. Without a sign or tutorial prompt, check that home and field are recognizable destinations and the caretaker spot is findable. Adjust the broad layout if the field, home, or walkable area is hidden by the camera.
7. **Walk direct routes and interact.** Go home-to-field and back without crossing the service yard or open yard; test field edges/corners, the bed prompt, and collisions. Walk the optional yard circuit once and shorten it if it feels like empty commute. Later, when features are implemented, verify reaching the water source, sprinkler, and shipping bin from their planned approaches.
8. **Replace blockout only after the layout works.** Add meshes, materials, vegetation, and lighting as a separate pass; do not solve route or scale problems with decoration.

## Reusable Editor tools

The proposed map-independent survey and blockout tools are described in [Reusable Unreal Editor level tools](../Scripts/README.md). HomeFarm can be their first configured layout, but the shared tools should not assume any farm-specific actors or zones.
