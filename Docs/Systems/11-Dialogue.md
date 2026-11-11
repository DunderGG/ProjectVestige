# 11. Dialogue and relationships

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** planned for Hob in milestone 2.

**References:** no built-in Epic dialogue system. The design is ours: data assets, gameplay tags for conditions, and `FText` for all lines.

**Rules:**

- **Dialogue is data**: data assets (or data tables imported from a spreadsheet) of lines with speaker, `FText`, conditions and outcomes. Conditions and outcomes are small, named, data-selected types (instanced `UObject` fragments or tags), not code per conversation.
- **Relationship state** (points, flags, seen lines) lives in one owner, a relationship component on the player state or a world subsystem, keyed by character `ContentId`. Robots don't store how the player feels about them.
- Dialogue UI is presentation: it shows lines it is given and reports choices.
- Saves store character IDs, points, flags and seen line IDs.
- Every line is localizable `FText` with a stable key ([section 18](18-Localization.md)).

**Pitfalls:**

- Writing lines in C++ or Blueprint string literals.
- Branching dialogue as a big Blueprint graph per character; it can't be reviewed, localized or modded.
