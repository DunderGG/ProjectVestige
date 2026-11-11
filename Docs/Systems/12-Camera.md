# 12. Camera

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** the template's spring arm and follow camera on `AVestigeCharacter`.

**References:** Epic's "Cameras" and "Camera Shakes" pages; Lyra's `ULyraCameraComponent` with a stack of `ULyraCameraMode` objects that blend (third person, aiming, death).

**Rules:**

- Keep the spring arm camera until a second camera behavior is needed (a dialogue close-up, an interior, a building mode). Then adopt Lyra's idea: camera modes as small classes on a stack, blended by the camera component, chosen by whoever needs them (dialogue pushes a dialogue mode).
- The camera only presents; it never changes game state.

**Pitfalls:**

- Several systems setting the camera's transform directly. Everything goes through one camera owner.
