# 13. Animation

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** the template's mannequin and animation Blueprint, with the held tool attached to `hand_r`.

**References:** Epic's "Animation Blueprints", "Linked Animation Layers", "Property Access" and "Animation Optimization" pages; Lyra's animation Blueprint, which uses linked animation layers per held item and thread-safe update functions.

**Rules:**

- **No game logic in animation Blueprints.** They read state (speed, the tool in hand, exhausted or not) and play animation.
- **Thread-safe animation updates.** Read gameplay state through property access in `BlueprintThreadSafeUpdateAnimation` so the update can run off the game thread; avoid the event graph's per-frame logic.
- **Linked animation layers per tool type**, as Lyra does per weapon, so a new tool animation set doesn't touch the main graph.
- Gameplay timing that matters (when a hoe hits the ground) comes from the game, not from an animation notify, so rules still work without animation and in tests. Notifies trigger cosmetics only (sounds, dust).
- Robots far away use animation update rate optimization; crowds use the Animation Budget Allocator plugin if needed.

**Pitfalls:**

- Casting to the character every frame in the animation Blueprint; cache it in `NativeInitializeAnimation`.
