# 18. Localization

One of the [system guides](../Systems.md); read [How to use this document](../Systems.md#how-to-use-this-document) first.

**Status:** not started; player text is `FString` today ([CodingGuidelinesAudit C-10](../CodingGuidelinesAudit.md)).

**References:** Epic's "Localization" pages (`FText`, `LOCTEXT`/`NSLOCTEXT`, string tables, the Localization Dashboard, `FText::Format`, `FText::AsNumber`, culture-aware formatting).

**Rules:**

- **Every string the player reads is `FText`**: `LOCTEXT` in C++ with a `LOCTEXT_NAMESPACE` per file, `FText` properties in data assets, string tables for shared UI strings.
- **Build sentences with `FText::Format`** and named arguments (`{Day}`), never by concatenating strings, so translators can reorder words.
- Numbers, dates and the clock go through culture-aware formatting; the 24-hour clock stays the default.
- Set up the Localization Dashboard's gather once there is a menu, so missing text shows up early. It gathers from assets as well as code: the input actions' descriptions and mappable display names are asset text since H6 (2026-11-07), so the gather includes `Content/Vestige/Input`, and the feedback texts are asset text since M4 (2026-11-08, `DA_FeedbackResponses`), so it includes `Content/Vestige/UI`, and the seasons' names are asset text in the calendar (`Content/Vestige/DA_GameData`, [section 4](04-Time.md)).

**Pitfalls:**

- `FText::FromString` on literal text, which makes it untranslatable.
- Text baked into textures.
