# Workflow audit and recommended fixes

**Date:** 2026-11-14  
**Scope:** `AGENTS.md`, `CLAUDE.md`, `.claude/README.md`, `.claude/settings.json`, `.claude/agent-flow.puml`, the seven `.claude/agents/*.md` files, and the workflow entries in `Docs/DecisionLog.md`.  
**Status:** Fixes 1 to 6 and 8 are applied (see "Implementation status"). Fixes 7 and 9 wait for usage data and the maintainer's go-ahead. Every applied change had the maintainer's yes (`AGENTS.md`, "What still needs the maintainer's yes") and has a `Docs/DecisionLog.md` entry dated 2026-11-14.

This document replaces an earlier audit and a second review of it. The first pass over-read some documents and missed others, so every finding was re-checked against the repository files and Claude Code's published documentation ([permissions](https://code.claude.com/docs/en/permissions), [subagents](https://code.claude.com/docs/en/sub-agents)). Only the fixes that survived that check are below. Findings that did not survive are in the appendix, with the reason, so nobody re-raises them.

## Implementation status

| Fix | Status | Commit subject |
|---|---|---|
| 1. Fresh session per roadmap item | Applied | Work each roadmap item in a fresh session |
| 2. Lanes and tripwires | Applied | Route changes by lane and tripwire |
| 3. No avoidable permission prompts | Applied: scratch folder `Saved/AgentWork/`, one allow rule for the script, one `Edit(/Saved/AgentWork/**)` rule. Asset-changing commands stay gated | Keep agent scratch files and reports inside the repository |
| 4. Verify script | Applied: `Tools/Verify-Vestige.ps1`. First run: 394 tests passed, 47 s; a second run took 24 s. An independent review found that a crashed test run could report success; fixed before the commit | Add a verify script that builds and runs the tests |
| 5. Reports only when needed | Applied | Keep agent scratch files and reports inside the repository |
| 6. Out-of-date statements | Applied: nested subagents, hard-coded sizes, and the diagram, which now shows a lane-picking step that branches into the writing route and lanes A, B and C (re-rendered) | Correct outdated statements in the workflow files; Show lanes and the editor lock in the agent flow diagram; Draw the lanes in the agent flow diagram |
| 7. `effort`, `maxTurns`, Sonnet trial | Not started: needs usage data from a few tasks first | |
| 8. Reversibility tag | Applied | Work each roadmap item in a fresh session |
| 9. Usage log | Not started: needs the maintainer's go-ahead, because it reads personal transcript files | |

**To confirm on first real use:**

- The allow rule for the script (`PowerShell(& "…\Verify-Vestige.ps1" *)`) matches the way Claude Code is asked to call it. If it still prompts, approve it once and keep the form it offers.
- Whether background subagents still prompt for anything the rules above should cover.
- `claude --version`, to confirm the nested-subagent statement in `CLAUDE.md` for the installed version.
- The script's "editor is running" stop and its crash path were reviewed but not run.

## Summary

The workflow's safeguards are good and should stay: one owner per responsibility, an independent reviewer, tests for every changed rule, a failing test first for bug fixes, and an explicit list of decisions reserved for the maintainer. The weaknesses are in how work is **routed**, how often the main session is **re-read**, and how many **permission prompts** reach the maintainer. In priority order:

1. The main session's turn count was the largest measured cost, and nothing in the workflow limits it before it has built up.
2. "When in doubt, run the full flow" sends uncertain work to the most expensive path, and "doubt" has no boundary.
3. The agents' own procedure (scratch files outside the repository, long raw commands) causes permission prompts that all reach the maintainer.
4. Three smaller process gaps: who reruns the suite after a fix round, report files written when nobody needs them, and statements in the docs that are now wrong.

## What to keep

- Independent code and design review, and the reviewer treating the maintainer's ideas as proposals to check.
- Automated tests for every changed rule, and a test that fails before a bug is fixed.
- A build and a full suite run after the last source change.
- Human decisions reserved for game feel, game design, licensing, money and anything that can't be undone.
- The maintainer's recorded decision to let work continue on a recommended option while a design question is open, because the code built on it is kept cheap to change (see appendix, item A).
- Narrow command permissions. Do not allow arbitrary shells, Python or editor commands just to stop the prompts.

## Evidence and limits

**Measured** (UTF-8 bytes / 4, the rough method `Docs/DecisionLog.md` uses):

| What | Approx. tokens |
|---|---|
| `AGENTS.md` (in every agent's context through `CLAUDE.md`) | 5,000 |
| `Docs/CodingGuidelines.md` (implementer and reviewer read all of it) | 12,900 |
| `Docs/Architecture.md` | 18,800 |
| `Docs/Roadmap.md` | 23,000 |
| `Docs/DecisionLog.md` | 31,900 |
| `Docs/PlayTest.md` (the test author reads all of it) | 7,200 |
| Each of the seven `.claude/agents/*.md` files | 2,000 to 2,700 |
| Automated tests | 394 `TEST_METHOD`s in 40 files |

**From the decision log, not re-measured:** on the spring-crops task the main session read about 92 million tokens over 348 turns while its conversation grew from 130,000 to 430,000 tokens, and all seven agents together used about 1 million. A first review cost about 128,000 tokens and its second round about 18,000. This is **one task**, so treat the ratios as indicative, not as a rule.

**Not measured, so treat as open:**

- Real token usage and cost per role across several tasks. Claude Code keeps session transcripts in the user profile, so this is possible, but it reads personal data and needs the maintainer's go-ahead.
- Which permission mode the maintainer uses and which model his main session runs on. `.claude/settings.local.json` is git-ignored and not in the checkout.
- How an allow rule for a script call is matched (`PowerShell(& ./Tools/x.ps1 *)` or without the `&`). The documentation shows the rule shape but not the call operator, so the first run settles it.
- How long the test suite takes to run.
- Which version of Claude Code is installed (some features below need recent versions).

## Recommended fixes

### Fix 1. Start every roadmap item in a fresh session (High)

**Problem.** Each turn re-reads the whole main conversation. The 348-turn task shows that this, not the agents, was most of the cost, and the current rule (hand over at about 200,000 tokens) only reacts after the cost has built up. Claude Code's prompt cache makes repeat reads cheaper than the raw token count suggests, but the real price split is unknown.

**Fix.**

- One roadmap item per session. Everything durable is already in the docs and in git, so little is lost by starting fresh.
- Keep the 200,000-token handoff as a backstop for tasks that really are that long.
- Cut turns: the verify script (Fix 4) replaces a chain of greps and log reads with one call. Change "read each result before acting on it" in `CLAUDE.md` to "verify with the script's summary and the diff stat, not by re-reading reports".

**Cost and risk.** None to run. Costs a one-line edit to `CLAUDE.md`.

### Fix 2. Replace "when in doubt, run the full flow" with lanes and tripwires (High)

**Problem.** `AGENTS.md` ("Smaller changes") defines a small task as "a few lines in one or two files" and ends with "When in doubt, run the full flow". Most of what the lanes below say is already in `AGENTS.md`: a change with no design choice already skips the architect and the design review. What is missing is a concrete boundary, so uncertain work defaults to the full chain of agents. The decision log (2026-11-13) shows that the number of agents per task is the main cost lever.

**Fix.** Three lanes, plus a list of tripwires:

- **Lane A, trivial:** typos, lore, a value the maintainer already chose, a one-function fix with no rule change. One agent and one code review (today's "small task").
- **Lane B, standard change inside an established system:** no game designer unless `Docs/GameDesign.md` and the maintainer's words leave player-facing behaviour open; **no architect and no design review**. Implementer, then test author if a rule changed, then one code review.
- **Lane C, new design:** the full flow.

**Tripwires.** If any one applies, the change is Lane C:

1. A new class, component or subsystem, or a change to a row of the ownership table (`Docs/Architecture.md` section 13).
2. A new or changed data asset type or field, a content ID scheme, or anything saved (save key, version, `IVestigeSaveable`).
3. A new engine framework or `Build.cs` dependency, or a new gameplay ability, effect or attribute.
4. Anything that runs per frame, on a timer or on the new day, or loops over many objects.
5. A change to a public interface other classes call, or to the layer or include direction.
6. The system has no guide in `Docs/Systems/`.

**Tie-break:** unsure whether a *tripwire* applies means Lane C. Unsure about anything else means Lane B.

**Safety net.** The main session puts a two-line routing note in the brief to the reviewer (owner class, tripwires checked). The code review verifies it, and a crossed tripwire is a `CHANGES NEEDED` finding that sends the task back to Lane C. This keeps independent checking, which matters because the maintainer cannot yet catch an architectural mistake himself.

**Expected effect.** Lane B skips the two most expensive steps. No number can be given without usage data. The list above is a draft: adjust the tripwires after the first few tasks.

### Fix 3. Stop the agents' own procedure from causing permission prompts (High)

**Problem.** The documentation says:

- Subagents that Claude starts run in the background by default, and **every permission prompt from one surfaces in the main session**, so each un-allowed command interrupts the maintainer.
- Reads outside the working directory and edits outside the working directory and additional directories need approval, and output redirects are checked the same way.
- `.claude/settings.json` allows `Build.bat` and a few read commands, but not the documented test command (`UnrealEditor-Cmd.exe … Automation RunTests`), the off-screen game launch, Blender or the headless asset scripts.

All seven agent files send reports, build logs, scripts and `-userdir` folders to "a temporary folder outside the repository". The standard procedure therefore triggers prompts that need not exist.

**Fix.**

1. Use **`Saved/AgentWork/<task>/`** for reports, logs, editor scripts and `-userdir`. `Saved/` is already git-ignored (`.gitignore` line 3) and is inside the working directory, so no permission is widened. Delete the task folder after the commit so it does not grow.
2. Move the exact build and test commands into a committed script (Fix 4) and allow **one narrow rule** for it instead of several long raw commands. The script takes no parameter that runs an arbitrary command.
3. **Keep asset-changing commands gated.** Headless editor scripts and Blender can modify project files; they should keep prompting until the maintainer has approved the exact scope.
4. Confirm the allow-rule form on the first run.

This applies to Claude Code's permission system only. Copilot in VS Code has its own approval prompts that these rules do not affect.

**Risk.** It changes permissions, so the maintainer approves the exact rule, and the script goes through the implementer and a code review like any code.

### Fix 4. One verify script that builds and runs the suite (Medium)

**Problem.** `AGENTS.md` step 6 says to rerun the build and suite after a fix round that changed code, but not who. In practice either the test author is woken only to rerun, or the main session runs the suite and searches raw logs itself, which costs turns. Three agent files also carry near-identical "never read logs whole, search for `error`" paragraphs.

**Fix.** `Tools/Verify-Vestige.ps1` that:

- checks the editor is closed and stops with a clear message if it is not;
- builds `VestigeEditor` (Development, Win64) with the command in `Docs/Setup.md` section 6;
- runs the whole `Vestige` suite with the command in `Docs/CodingGuidelines.md` section 13, with `-abslog=`;
- writes full logs under `Saved/AgentWork/<task>/` and prints a short summary: build result, tests run, passed and failed, failing test names, log path.

**Rule:** whoever last changes source or tests runs it, and the main session reads its one summary line. The three log-handling paragraphs in the agent files shrink to one line.

### Fix 5. Write a report file only when the next step needs one (Medium)

**Problem.** `CLAUDE.md` puts reports in "a temporary folder outside the repository" with no fixed location, and every agent file requires a full report file "even where your tools discourage writing report files", even for a one-line change. Output tokens are the most expensive tokens, and a file nobody reads is waste.

**Fix.** Reports go in `Saved/AgentWork/<task>/` (Fix 3). A report file is written **only** when the next step needs more than the 300-word reply carries, when a review has more than three findings, or for a session handoff. A Lane A change needs only the reply.

### Fix 6. Correct statements that are out of date (Low, but cheap)

- **Nested subagents.** `CLAUDE.md` and `.claude/README.md` say subagents cannot start other subagents. The current documentation says they can, up to three layers below the main conversation by default (`CLAUDE_CODE_MAX_SUBAGENT_SPAWN_DEPTH`). Run `claude --version` to see what the installed version does, then correct the sentence. This is not a case for rebuilding the flow around an orchestrator subagent: that is untested here, and subagents cannot ask the user questions directly, so the main session would still have to relay them.
- **Hard-coded file sizes.** The agent files say the decision log is about 29,000 tokens, the roadmap about 20,000 and `PlayTest.md` about 6,000. They measure about 31,900, 23,000 and 7,200. The advice (search large files, don't read them whole) does not need the numbers, so delete them rather than keep them in step.
- **Diagram.** `agent-flow.puml` does not show that editor and build steps never run at the same time, even across the code and art branches. Add one note to the fork the next time the diagram is edited and re-rendered.

### Fix 7. Set `effort` and `maxTurns` per agent, and trial a cheaper model for the implementer (Medium)

**Problem.** The model choices are not backed by a comparison, but a broad benchmark is too heavy for a solo developer. The documentation gives cheaper controls:

- `effort` can be set per agent in its frontmatter. None of the seven agents sets it, so all inherit the session's level.
- `maxTurns` caps a runaway agent (for example one stuck on a hung build); the output is marked partial and the agent can be resumed.
- Agents with `model: inherit` (the implementer and the artist) run on **whatever model the main session uses**. The implementer writes the most output, so this matters.

**Fix.**

- Keep the architect, game designer and both reviewers on Opus. Their value is judgment, and the reviewer is the technical safety net.
- Set `effort` explicitly on every agent, and a generous `maxTurns` as a safety stop. Start from current behaviour and adjust after a few tasks; no specific values are proposed without usage data.
- **Trial Sonnet for the implementer** on the next two or three tasks. The design is already approved and an independent Opus review, the build and the tests check the result. Revert if review rounds or High findings go up.

### Fix 8. Tag each technical decision as easy or hard to undo (Low)

`AGENTS.md` step 6 already requires the reply to list the technical decisions taken, a line each, "so they can question any of them". Add a short tag to each line saying whether it is **easy or hard to undo**, so the maintainer can see which ones deserve his attention. This is a wording change to an existing rule.

### Fix 9. Log usage automatically (Low, optional)

Counting tokens by hand conflicts with the maintainer's goal of minimal input, and judging cost per accepted task (tokens, interruptions and rework rounds together) is better than judging each agent call alone. Claude Code has a `SubagentStop` hook, and each subagent's transcript is stored under the user profile. A small hook script could append one line per agent (name, model, tokens) to `Saved/AgentWork/usage.csv`. The fields the hook receives were not verified, and it reads personal transcript files, so this needs the maintainer's go-ahead. Try it on a throwaway task first.

## Plan

| Phase | What | Effort | Risk |
|---|---|---|---|
| **0. Words only** | Fixes 1, 2, 6 and 8: fresh session per roadmap item, lanes and tripwires, corrected sentences, the reversibility tag | About an hour | Low. Needs the maintainer's yes and a decision-log entry |
| **1. Tooling** | Fixes 3, 4 and 5: `Saved/AgentWork/`, `Tools/Verify-Vestige.ps1`, one narrow allow rule, reports only when needed | About a day | Medium. It changes permissions, so the maintainer approves the exact rule; the script goes through the implementer and a code review |
| **2. Measure and tune** | Fix 7 and optionally Fix 9 | Over the next tasks | Low. Each change is easy to revert |
| **3. Only if cost stays too high** | Consolidate the repeated paragraphs across the agent files; try an orchestrator subagent | Later | Higher. Worth doing only if phases 0 to 2 are not enough |

**Recommendation: do phases 0 and 1 now.** They remove most interruptions and the largest avoidable cost, and none of them weakens a safeguard. Phase 2 is cheap once phase 1 exists.

## Appendix: earlier findings not adopted

**A. "Continuing on the recommendation can bypass the user's decision." Rejected.** The audit missed the clause in `AGENTS.md` that limits it: "the code built on the recommended option is kept cheap to change", plus the rule that an unanswered question goes into `Docs/GameDesign.md` section 15. It is also the maintainer's own recorded decision (`Docs/DecisionLog.md`, 2026-11-13: he asked to be needed as little as possible, and "every question to the maintainer, which stalled small tasks" is a rejected option). Pausing dependent work would increase the human input he asked to reduce. Irreversible cases (save formats, deleting without a copy, licensing) are already on the maintainer's-yes list.

**B. "The flow diagram lets the implementer and test author run in parallel." Rejected; the finding was factually wrong.** The fork in `agent-flow.puml` is between the code branch and the art branch. Inside the code branch the order is implementer, test author, code review, and the legend states the bug-fix exception. The only true remainder is the editor/build note in Fix 6.

**C. "The test author's edge-case list encourages over-testing." Rejected.** "Test the edges" is read as the edges that apply, the suite is large (394 tests) but there is no evidence it is bloated or slow, and the project rule (a test for every changed rule, a test that would have caught each bug) is the real standard. If suite run time becomes a problem, measure it first.

**D. "Trim what agents must read in `CodingGuidelines.md`." Not worth it.** The first suspicion was that reading it whole on every fresh start was a major per-agent cost. Measured, it is about 12,900 tokens against about 128,000 for a first review. The better lever is how many fresh agents a task starts, which Fix 2 reduces.

**E. "Restructure the duplicated agent instructions now." Deferred.** The repeated paragraphs ("who you work for", "question what looks wrong", "what you return") cost a few hundred tokens per agent. The real cost is drift between copies, which Fix 6 reduces by deleting hard-coded numbers. Consolidate only when those files are edited anyway (phase 3).

**F. "The build and test ownership instruction is ambiguous about the main session." Partly rejected.** `AGENTS.md` step 6 is clear that the main session reads the log and reruns only if a fix round changed code. The real gap, who reruns after a fix round, is Fix 4.

**G. "Benchmark each model role." Replaced** by Fix 7, which uses per-agent `effort`, `maxTurns` and a single-role trial instead of a benchmark.
