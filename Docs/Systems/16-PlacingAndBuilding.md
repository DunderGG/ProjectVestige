# 16. Placing objects and building

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** planned (sprinklers and the shipping bin in milestone 2, building and crafting later).

**References:** Epic's "Instanced Static Mesh Component" and "Hierarchical Instanced Static Mesh" pages for many placed copies; the farm grid in [Architecture.md, section 13](../Architecture.md#13-systems-and-ownership).

**Rules:**

- Placeable objects are content: a placeable item fragment ([section 8](08-Inventory.md)) names the actor class or mesh to place, its footprint in tiles and its rules.
- **One owner of what is placed where**: the grid (or a placement subsystem if placement extends beyond the farm grid) records occupancy by grid coordinates, checks placement and saves placed objects by ID and coordinates.
- Placement preview is presentation: a ghost mesh that reads the placement check, never a real spawned object until confirmed.
- Objects with behavior (a sprinkler) are actors that listen to `OnNewDay`; purely decorative objects (fences, paths) are instances in an instanced static mesh component, not actors.

**Pitfalls:**

- Spawning and destroying actors every frame for the preview.
