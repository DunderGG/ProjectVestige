# Project Vestige agents

Claude Code subagents that take the specialist roles in Project Vestige's work, so the person they work for (the maintainer or a contributor) doesn't have to. The rules for the flow are in `AGENTS.md`, "How work is done", and how Claude Code uses these agents is in `CLAUDE.md`, "Specialist subagents". This file only gives the overview; when they differ, those two files win.

| Agent | Role | Writes |
|---|---|---|
| [`vestige-game-designer`](agents/vestige-game-designer.md) | Advises on how the game plays: prepares the design questions with options and a recommendation, proposes tuning values, reviews the design document; the user decides | `Docs/GameDesign.md` (decisions the maintainer made) |
| [`vestige-architect`](agents/vestige-architect.md) | Designs each change and owns the technical decisions | `Docs/Systems/` guides, `Docs/Architecture.md` (the ownership table), `Docs/DecisionLog.md` |
| [`vestige-reviewer`](agents/vestige-reviewer.md) | Senior Unreal C++ reviewer of designs and code, and of the audio designer's sounds; reviews again until it approves | Nothing (read-only) |
| [`vestige-art-reviewer`](agents/vestige-art-reviewer.md) | Technical artist who reviews the artist's work: looks at the renders, measures the mesh, reads the scripts; reviews again until it approves | Nothing (read-only) |
| [`vestige-implementer`](agents/vestige-implementer.md) | Writes the C++ from the approved design and builds it clean | `Source/`, temporary editor scripts |
| [`vestige-test-author`](agents/vestige-test-author.md) | Writes the CQTest tests, runs the whole suite, keeps the manual checks | `Source/Vestige/Tests/`, `Docs/PlayTest.md` |
| [`vestige-artist`](agents/vestige-artist.md) | Makes Blender models from scripts, exports and imports them, keeps the licensing records | `Art/`, `Content/Vestige/`, `CREDITS.md`, `LICENSING.md` |
| [`vestige-audio-designer`](agents/vestige-audio-designer.md) | Plans the sound for each feature, makes sounds from scripts, finds recordings and library sounds under an accepted license, imports them; can't hear, so it measures and every sound gets a listening check; reviewed by `vestige-reviewer` | `Art/Audio/`, `Art/Export/*.wav`, `Content/Vestige/Audio/`, `EditorScripts/SetUpAudio.py`, `CREDITS.md`, `LICENSING.md`, listening checks in `Docs/PlayTest.md` |
| [`vestige-writer`](agents/vestige-writer.md) | Writes the lore, story, characters, dialogue, item texts, found texts and names; keeps the spoiler documents consistent and spoilers out of everything else; decides line-level writing and minor canon itself, and asks the user only about big canon | `Docs/Spoilers/` |

The main Claude session runs the flow, because a subagent can't ask the user questions (current Claude Code versions do let subagents start subagents, but this flow doesn't use that): it hands each agent its brief, passes the results on, takes game feel and design questions to the user, and commits. Every agent questions the user's ideas rather than just agreeing. Working for a contributor, nothing in the flow stands in for the maintainer's review of the pull request.

## The flow

![The agent flow, from a task being picked up to done](agent-flow.png)

The diagram's source is [`agent-flow.puml`](agent-flow.puml) (PlantUML). After changing it, render the image again: in VS Code, open the file and press Alt+D to preview it, or right-click > Export Current Diagram; from a terminal in `.claude/`, `java -jar <path to plantuml.jar> -tpng agent-flow.puml`. Activity colours use the current `:text; <<#colour>>` syntax, which needs a recent PlantUML: the `plantuml.jar` shipped with the VS Code extension (1.2024.3) is too old for it, so render with the extension set to the PlantUML server, or a current jar.

To see where a task's tokens went, run `python Tools/measure_agent_tokens.py` for the last sessions, one line each, or `python Tools/measure_agent_tokens.py --session latest` for one session split by the roles in the diagram; it reads Claude Code's transcripts on this PC and prints numbers only.

**Lanes.** The main session picks a lane before starting any agent: A (trivial) is one agent and a code review; B (a change inside an established system) is the implementer, the test author when a rule changes and a code review, with no architect or design review; in both, review findings go back to the author until the reviewer approves; C (a new design, meaning a tripwire in `AGENTS.md`, "Lanes", applies) is the full flow. Unsure about a tripwire means C; unsure about anything else means B.

**Writing.** Lore, story, dialogue and other text with a voice go to the writer and skip the flow. Text that goes into the game is marked Ready by the writer, then entered by the implementer as `FText` data and code-reviewed in its lane.

The agents need Claude Code; they are picked up from `.claude/agents/` when Claude Code runs in this repository. With another AI tool, follow the same roles from "How work is done" in `AGENTS.md`. The reasons for the workflow are in the decision log entry of 2026-11-11 in `Docs/DecisionLog.md`. Personal Claude Code settings go in `.claude/settings.local.json`, which Git ignores.
