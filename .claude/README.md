# Project Vestige agents

Claude Code subagents that take the specialist roles in Project Vestige's work, so the person they work for (the maintainer or a contributor) doesn't have to. The rules for the flow are in `AGENTS.md`, "How work is done", and how Claude Code uses these agents is in `CLAUDE.md`, "Specialist subagents". This file only gives the overview; when they differ, those two files win.

| Agent | Role | Writes |
|---|---|---|
| [`vestige-game-designer`](agents/vestige-game-designer.md) | Advises on how the game plays: prepares the design questions with options and a recommendation, proposes tuning values, reviews the design document; the user decides | `Docs/GameDesign.md` (decisions the maintainer made) |
| [`vestige-architect`](agents/vestige-architect.md) | Designs each change and owns the technical decisions | `Docs/Systems/` guides, `Docs/Architecture.md` (the ownership table), `Docs/DecisionLog.md` |
| [`vestige-reviewer`](agents/vestige-reviewer.md) | Senior Unreal C++ reviewer of designs and code; reviews again until it approves | Nothing (read-only) |
| [`vestige-art-reviewer`](agents/vestige-art-reviewer.md) | Technical artist who reviews the artist's work: looks at the renders, measures the mesh, reads the scripts; reviews again until it approves | Nothing (read-only) |
| [`vestige-implementer`](agents/vestige-implementer.md) | Writes the C++ from the approved design and builds it clean | `Source/`, temporary editor scripts |
| [`vestige-test-author`](agents/vestige-test-author.md) | Writes the CQTest tests, runs the whole suite, keeps the manual checks | `Source/Vestige/Tests/`, `Docs/PlayTest.md` |
| [`vestige-artist`](agents/vestige-artist.md) | Makes Blender models from scripts, exports and imports them, keeps the licensing records | `Art/`, `Content/Vestige/`, `CREDITS.md`, `LICENSING.md` |

The main Claude session runs the flow, because subagents can't start other subagents: it hands each agent its brief, passes the results on, takes game feel and design questions to the user, and commits. Every agent questions the user's ideas rather than just agreeing. Working for a contributor, nothing in the flow stands in for the maintainer's review of the pull request.

## The flow

![The agent flow, from a task being picked up to done](agent-flow.png)

The diagram's source is [`agent-flow.puml`](agent-flow.puml) (PlantUML). After changing it, render the image again: in VS Code, open the file and press Alt+D to preview it, or right-click > Export Current Diagram; from a terminal in `.claude/`, `java -jar <path to plantuml.jar> -tpng agent-flow.puml` (the VS Code PlantUML extension ships a `plantuml.jar`).

A "No design choice" change is one with no new owner, interface, data, save format or engine feature.

The agents need Claude Code; they are picked up from `.claude/agents/` when Claude Code runs in this repository. With another AI tool, follow the same roles from "How work is done" in `AGENTS.md`. The reasons for the workflow are in the decision log entry of 2026-11-11 in `Docs/DecisionLog.md`. Personal Claude Code settings go in `.claude/settings.local.json`, which Git ignores.
