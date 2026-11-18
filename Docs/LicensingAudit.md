# Licensing audit

**Reviewed:** 2026-10-26  
**Scope:** A critical review of the project's licensing documents and stated goals: accept code and creative contributions, allow the maintainer to sell ready-built copies, and prevent a closed-source fork from redistributing the covered game code. This is a document and policy audit, not legal advice or a legal opinion on enforceability.

## Executive assessment

**Overall: 3/10.** The project has made a serious effort to identify separate code, asset and Epic-material licenses, and it recognizes that Unreal Engine needs a special GPL permission. However, the current terms do not cleanly express the stated goals. The code is GPL-licensed, which permits commercial redistribution when GPL obligations are met; the assets are CC BY-NC-SA, which restricts commercial use by the public; and the contributor terms separately give the maintainer broad rights to sublicense contributions under other terms.

As written, it is not accurate to summarize the whole project as simply “open source.” The code has a copyleft open-source license; the art and other non-code material have a noncommercial license. Nor is the policy as written a simple guarantee that nobody can ever make a closed-source version: the contributor grant gives the maintainer rights to relicense contributions, while the treatment of template material and the scope of the Unreal linking permission need careful handling.

Scores use a 1–10 scale: 1 means materially conflicting or unclear; 5 means a plausible direction with significant unresolved policy/legal details; 10 means clear, internally consistent terms reviewed for the intended distribution model.

| Area | Score | Assessment |
|---|---:|---|
| Fit with open-source contribution goals | 4/10 | Code is copyleft; art and documentation are noncommercial, so the repository as a whole is not open for all uses. |
| Permission to sell official builds | 5/10 | A separate maintainer grant is intended to authorize this, but the public asset terms and inbound rights need a clear, durable exception. |
| Protection against closed-source forks | 4/10 | GPL is the right general mechanism for distributed covered code, but the broad maintainer sublicensing grant and unclear component boundaries weaken the stated policy. |
| Contributor consent and rights chain | 3/10 | Terms rely on submission implying agreement; scope, timing and evidence of assent are underspecified. |
| License notice and delivery | 3/10 | The GPL text is present, but the claimed section 7 permission appears only in LICENSING.md, not in LICENSE itself. |
| Third-party and Epic material boundaries | 4/10 | Some exclusions are named, but the file-level inventory and compatibility process are incomplete. |
| Clarity for distributors and storefront users | 3/10 | The plain-language summary conflicts with the licenses' different permissions and does not explain source delivery obligations in enough detail. |
| Legal readiness | 2/10 | The model is complex (GPL + linking permission + CC-NC-SA + individual contributor license); it warrants specialist review before public release or store distribution. |

## Goal-by-goal assessment

### 1. Anyone can contribute code or art

**Partially supported, with material conditions.** The contribution guide welcomes code and creative work and proposes inbound license terms. But art and “everything else” are under CC BY-NC-SA, which does not grant the public general commercial-use rights. The contributor must also have authority to give both the repository license and the maintainer's additional commercial sublicense. The documents do not define a clear intake process for contributors who cannot or do not want to accept both grants.

### 2. The maintainer can sell ready-built copies

**The intention is documented; the permissions chain needs review.** LICENSING.md says only the maintainer sells official builds and CONTRIBUTING.md requests a separate commercial license from contributors. The original copyright holder can generally grant permissions beyond a public license for their own work, but the document should not rely on a reader inferring how the exception interacts with the CC BY-NC-SA grant. Third-party material, template material and any contributions whose submitters did not validly accept the terms remain separate rights questions.

### 3. A fork cannot sell a closed-source version

**GPL supports this goal for covered code distributed in a derivative work, but not a ban on commercial forks generally.** GPL-3.0 permits charging money for copies; recipients of covered code must receive the GPL freedoms and applicable source. A fork may therefore be sold, but its covered GPL code cannot simply be distributed under closed-source terms.

That distinction should be made explicit: the project can require source availability and GPL rights for covered code; it cannot use GPL to prevent all commercial forks or prevent someone from independently creating a similar game without copying protected material. The separate maintainer sublicense grant is broader than the stated “no closed-source version” goal and should be resolved deliberately.

## Findings

### LIC-01 — CC BY-NC-SA directly restricts public commercial use of the game assets

**Severity: Critical**

LICENSING.md applies CC BY-NC-SA 4.0 to art, models, textures, materials, Blueprints, levels, audio, music, game data and documentation. Its own summary says the public cannot sell those assets or a game containing them, while the maintainer alone sells official builds. CC BY-NC-SA is a noncommercial license; it does not provide general commercial rights to downstream users.

**Why it matters:** This conflicts with an unrestricted “anyone can contribute” message for contributors who expect their art to be usable in a commercially distributed game, and it means the full project is not open-source/open-content in the ordinary no-field-of-use-restriction sense. The commercial exception for official builds is a separate permission and does not automatically extend to fork maintainers, other distributors, or every platform/storefront use.

**Recommended change:** Decide whether assets should be commercially reusable by everyone under an appropriate attribution/share-alike license, or whether commercial asset use is intentionally reserved to the maintainer. If reserved, state that restriction prominently and design an explicit rights grant for official commercial distribution. Do not describe the entire repository as open source without qualifying the asset license.

### LIC-02 — Contributor terms may authorize the maintainer to relicense work under proprietary terms

**Severity: Critical**

CONTRIBUTING.md grants the maintainer a “perpetual, worldwide, non-exclusive, royalty-free, irrevocable” right to “sublicense” contributions, including “under other license terms if the project needs them.” LICENSING.md repeats that the maintainer can use contributions in official builds. The text does not limit alternative terms to platform compliance or require the resulting game source to remain available under GPL.

**Why it matters:** The wording may permit the maintainer to distribute a version incorporating contributed material under non-GPL terms, depending on its interpretation and the contributor's assent. That is broader than the stated goal that a closed-source version must not be made from the repository, and can undermine contributors' expectation that their work will stay copyleft.

**Recommended change:** Decide whether the maintainer needs only a commercial-use exception for official builds that remain GPL/source-available, or a genuine dual-licensing right. State the chosen policy precisely. If the intent is to prevent closed-source releases by anyone, including the maintainer, remove or narrowly constrain the alternative-license/sublicense language after legal review.

### LIC-03 — The additional GPL section 7 permission is not included in the GPL license file

**Severity: High**

LICENSE contains the standard GPL text. The Unreal/platform additional permission is quoted in LICENSING.md, which says it is part of the code license terms, but the permission is not appended to LICENSE or otherwise clearly attached to each covered code distribution. The template source files also carry Epic copyright notices and are explicitly excluded from the project GPL.

**Why it matters:** A source user or downstream distributor may receive or inspect LICENSE without receiving the additional permission. The statement that the code is GPL “with” this exception is only useful if the permission is granted by the relevant copyright holders and accompanies the covered work in a way that clearly identifies what it applies to. A malformed or incomplete exception could leave the Unreal combination question unresolved.

**Recommended change:** Have counsel validate the exact exception, who can grant it, its compatibility with the relevant engine/platform terms, and its scope. Put the approved additional-permission text next to the GPL terms and add clear notices to covered source files or a consistent project-level licensing manifest. Do not assume one web page cures missing notices in downstream source archives.

### LIC-04 — “By submitting, you agree” is not a robustly specified consent process

**Severity: High**

CONTRIBUTING.md says submission of a pull request, file or “any other contribution” means agreement to contributor terms, then asks the contributor to add an agreement line to their first pull request. It does not say what happens for contributions through issues, discussions, email, code review suggestions, earlier submissions, or contributors who submit without the line. It also does not specify how consent is recorded for non-code assets.

**Why it matters:** The repository relies on a separate maintainer license grant beyond the standard public license, particularly to commercialize contributed assets. Unclear assent weakens the rights chain the policy depends on and can create uncertainty years later when releasing a paid build.

**Recommended change:** Use an explicit, recorded acceptance step before merging or importing contributions. Define exactly which contribution channels are covered, link the terms at the point of submission, record the accepted version, and require the contributor to confirm they own or control the rights. Consider a lightweight CLA/DCO-style workflow chosen with legal advice; a line in a first PR should not be assumed to solve all cases.

### LIC-05 — The contributor's public license and private grant are not clearly reconciled

**Severity: High**

Contributor clause 2 licenses assets under CC BY-NC-SA; clause 3 separately grants the maintainer broad commercial use and sublicensing. The documents do not explain whether the public may use those assets noncommercially while the maintainer has an additional commercial license, whether the commercial grant is limited to official FarmSim builds, or how contributors can withdraw or amend mistaken submissions.

**Why it matters:** Contributors and downstream users may reasonably read the CC license as the complete asset permission, while the maintainer relies on a separate contract grant for sales. The scope difference is the core of the licensing scheme, not a detail.

**Recommended change:** State the two grants separately in plain language and in the binding terms: (a) public license and its limits, and (b) the exact additional permission to the maintainer, including which products, media, platforms and sublicensing/contractors it covers. Make clear whether the official build remains GPL for code and what license applies to assets in that build.

### LIC-06 — “Open,” “open source,” “official build” and commercial redistribution are ambiguous

**Severity: High**

LICENSING.md says FarmSim “is open,” while the README calls the code GPL and assets CC BY-NC-SA. It also says “Nobody can take the code and ship a closed-source game,” then “Only the maintainer sells official builds.” These statements mix separate claims: public access, an OSI-style open-source license, permission to make commercial copies, and the project's own official store distribution.

**Why it matters:** Contributors may believe they can contribute art to any commercial fork, or players may think paid copies are the only legal way to obtain the game. GPL permits selling covered code if the distributor complies with GPL; it does not reserve sales to the maintainer. The asset restriction, rather than GPL, is what tries to reserve commercial asset use.

**Recommended change:** Replace “FarmSim is open” with a precise summary of code and asset permissions. Distinguish “official builds sold by the maintainer” from what the public license permits. Explain that GPL-covered code may be commercially redistributed with GPL obligations, while the current asset license separately restricts commercial use.

### LIC-07 — Code/asset boundaries and included third-party material need a maintained inventory

**Severity: High**

LICENSING.md assigns code to `Source/`, `.uproject`, `Config/`, build/tool scripts, and assets plus “everything else” to CC BY-NC-SA. It separately lists several Epic template source files and content folders under the Unreal Engine EULA. This leaves potential ambiguity for generated files, editor configuration, data assets, shaders/material graphs, project scripts, sample code, and files that combine project-authored and Epic-authored material.

**Why it matters:** License compliance depends on knowing which terms apply to each distributed file and which upstream terms govern embedded content. The project also welcomes contributions and third-party material, increasing the chance of mixed provenance.

**Recommended change:** Maintain a file/path-level licensing manifest with copyright holder, license, source/provenance and whether the file ships in a packaged build. Mark mixed-origin files and modifications explicitly. Define a review step for imports and generated or derived files, and ensure the manifest matches repository contents and store distribution.

### LIC-08 — “Compatible license” is not defined for incoming art and code

**Severity: Medium**

CONTRIBUTING.md gives examples such as CC0 or CC BY for assets and MIT or BSD for code, while requiring contributions to be licensed under the destination area's license. It does not define what “compatible” means when the contributor cannot relicense third-party material, when attribution requirements must be preserved, or when an imported asset is only permitted for noncommercial use.

**Why it matters:** A contributor may submit an asset under a license that permits their own use but not redistribution in a paid game or not sublicensing under the maintainer's extra grant. The examples alone are not an adequate clearance process, especially for models, fonts, music, textures and generative outputs.

**Recommended change:** Require a source and license record for each third-party component, not just a broad statement in the PR. Define acceptable licenses separately for code, assets and documentation; preserve required attribution notices; and reject material where commercial distribution or the necessary sublicense is uncertain. Treat AI-generated output as requiring provenance and rights review, not as automatically clear.

### LIC-09 — The Unreal/platform permission's scope and source obligations need specialist validation

**Severity: High**

The proposed permission covers combinations with Unreal Engine and “libraries and SDKs of a game distribution platform or console” and excludes those components from Corresponding Source. This is a custom GPL exception with a broad, important effect; the documents state that it is needed, but do not show that it has been reviewed against GPLv3 section 7, Unreal's EULA, platform/console SDK terms, or the exact source-distribution model.

**Why it matters:** The exception must be both effective for the intended combination and narrow enough not to exempt FarmSim's own code. Console and storefront SDK terms can impose redistribution or confidentiality constraints, and those obligations must not conflict with GPL obligations for the covered FarmSim code.

**Recommended change:** Obtain review from counsel experienced with GPL and Unreal distribution before publishing or selling builds. Validate each target platform separately, document which SDK files are excluded and which FarmSim source remains available, and avoid claiming that one general permission automatically handles every console/store.

### LIC-10 — GPL versioning and the “GPL-3.0-or-later” claim need consistency checks

**Severity: Medium**

LICENSING.md and the README identify code as GPL-3.0-or-later, but CONTRIBUTING.md describes it as GPL-3.0 without consistently saying “or later.” The supplied LICENSE file is the standard GPLv3 text and does not itself select the project's application version in the excerpts reviewed; the custom permission also does not clearly say how it applies if a recipient elects a later GPL version.

**Why it matters:** Users need to know which license version governs each contribution and whether the additional permission follows later versions. This becomes especially important when the exception is the reason a user believes Unreal distribution is allowed.

**Recommended change:** Use one precise designation everywhere, add an SPDX-style identifier and copyright/license notice for each project-authored code file, and specify whether the additional permission applies with GPLv3 only or with later GPL versions too. Have counsel review version interaction.

## Recommended correction order

1. Decide and document the actual policy for commercial asset use and official-build exceptions (LIC-01, LIC-05, LIC-06).
2. Resolve whether any party—including the maintainer—may distribute a closed-source version, then narrow the contributor sublicense grant accordingly (LIC-02).
3. Have the custom GPL/Unreal/platform permission reviewed and attach it correctly to the covered code (LIC-03, LIC-09, LIC-10).
4. Establish a recorded contributor acceptance process before relying on the additional commercial grant (LIC-04).
5. Build a provenance/licensing manifest and a clear incoming-material clearance policy (LIC-07, LIC-08).

## Review limits

This audit compares the checked-in license and contribution documents with the stated goals. It does not establish whether a particular contributor accepted terms, whether FarmSim has sufficient rights to any individual asset, or whether the proposed exception is legally effective. It is not a substitute for legal advice. Before opening the repository publicly, accepting external contributions, or distributing paid builds, obtain specialist review of the complete rights chain and platform-specific obligations.

## Status (2026-10-28)

Changes made in response to this audit. None of them replaces the lawyer's check, which is still open (see `Docs/Roadmap.md`, section 6).

| Finding | Status | What changed |
|---|---|---|
| LIC-01 | Addressed | Kept CC BY-NC-SA 4.0 for assets on purpose. `LICENSING.md` now says plainly that commercial asset use is reserved to the maintainer, that forks may sell GPL code but not the assets, and no longer calls the whole repository "open". |
| LIC-02 | Addressed | @DunderGG chose the narrow grant. Contributor terms (version 2) let the maintainer use contributions in official builds and accept store terms, but official builds keep their code under the GPL, so nobody (the maintainer included) can make a closed-source version. |
| LIC-03 | Addressed, needs legal review | The additional permission is now in `LICENSE-EXCEPTION.txt` next to `LICENSE`, with a scope section. Our source files carry a copyright line and SPDX identifier pointing to it. |
| LIC-04 | Addressed | Contributions are accepted only as pull requests; issues and ideas are not contributions. The PR template has a terms checkbox naming the terms version; PRs merge only when it is ticked, and the PR is the record. |
| LIC-05 | Addressed | `LICENSING.md` and the contributor terms state the public grant and the maintainer's grant separately, including what it covers (stores, consoles, porting studios and publishers working for him) and what license applies to code and assets in an official build. |
| LIC-06 | Addressed | New summary distinguishes free code, non-commercial assets and official builds; explains that the GPL allows selling copies and that building from source is legal. |
| LIC-07 | Addressed | `LICENSING.md` has a path-level table (copyright holder, license, origin, whether it ships) that must be updated with new folders or third-party files; mixed-origin template files are marked. How the `Images/*-CGPT.png` images were made is still to be recorded. |
| LIC-08 | Addressed | `CONTRIBUTING.md` lists accepted and rejected licenses for code, assets and docs (no third-party GPL/LGPL code, nothing NC/ND/BY-SA), requires source and license per file in the PR and `CREDITS.md`, and treats AI-generated material as third-party. |
| LIC-09 | Open | Needs a lawyer experienced with the GPL and Unreal Engine, and a check of each store and console's terms before release there. Listed under "Still to check" in `LICENSING.md`. |
| LIC-10 | Addressed, needs legal review | "GPL-3.0-or-later" used everywhere; the permission states it applies with later GPL versions too; SPDX identifiers added. |
