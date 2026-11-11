# 3. Gameplay tags

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** in use for seasons (`Vestige.Season.*` in `Config/DefaultGameplayTags.ini`) and, as native tags in `AbilitySystem/VestigeGameplayTags.h`, for the stamina system ([section 9](09-AbilitySystem.md)) and input (`Vestige.Input.*`, [section 5](05-Input.md)).

**References:** Epic's "Gameplay Tags" page; Lyra's native tags (`LyraGameplayTags.h/.cpp`, declared with `UE_DECLARE_GAMEPLAY_TAG_EXTERN` and defined with `UE_DEFINE_GAMEPLAY_TAG_COMMENT`).

**Rules:**

- Tags are for open-ended categories and states (seasons, item categories, input actions, ability states), not for individual pieces of content, which have `ContentId`s ([Architecture.md, section 5](../Architecture.md#5-data-and-content)).
- Our tags live under `Vestige.`; mods add their own root.
- Tags that C++ refers to are **native tags**, declared in one `VestigeGameplayTags.h/.cpp` with `UE_DEFINE_GAMEPLAY_TAG_COMMENT`, so a typo is a compile error and every tag has a comment. Tags only content refers to stay in the `.ini`.
- Use `FGameplayTagContainer` and `HasTag` / `HasAny` queries; restrict editor pickers with `meta = (Categories = "Vestige.Season")`.

**Pitfalls:**

- `FGameplayTag::RequestGameplayTag(TEXT("..."))` scattered through code: a renamed tag fails at run time. Use the native tag.
- Tags as a replacement for enums that code switches on exhaustively ([Architecture.md, section 12](../Architecture.md#12-designing-for-mods), rule 2).
