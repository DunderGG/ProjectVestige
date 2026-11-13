@AGENTS.md

## Claude Code notes

- The project rules live in `AGENTS.md` (imported above), `Docs/CodingGuidelines.md` and `Docs/Architecture.md`. Read both documents before writing or reviewing C++ in this repo, plus the guide in `Docs/Systems/` for each system the change touches.
- When working for David, the maintainer, these rules override Claude Code's default git attribution: commits carry no `Co-Authored-By:` or `Claude-Session:` lines, are authored by David Bennehag, and are never pushed or turned into pull requests. When working for another contributor, follow `CONTRIBUTING.md` instead.
- Before committing, run through the review checklist at the end of `Docs/CodingGuidelines.md`.
- Changes that need a manual check in the editor go into `Docs/PlayTest.md`, section "Pending checks", in the same commit, instead of only in your report.

## Specialist subagents

The roles in "How work is done" in `AGENTS.md` are subagents in `.claude/agents/`. Use them for every change to code or content; this file is the project's instruction to use subagents, so don't skip them to save tokens.

| Role | Subagent |
|---|---|
| Game designer | `vestige-game-designer` (advises on game design; the user decides) |
| Architect | `vestige-architect` |
| Reviewer | `vestige-reviewer` (designs and code; review, then re-review) |
| Art reviewer | `vestige-art-reviewer` (art from the artist; review, then re-review) |
| Implementer | `vestige-implementer` |
| Test author | `vestige-test-author` |
| Artist | `vestige-artist` |

- **The main session runs the flow.** Subagents can't start other subagents, so you start each step, pass results between them, decide when a review round is done, relay only game feel and game design questions to the user, and commit.
- **Every subagent starts without this conversation.** Give each a brief that stands on its own: the task and roadmap item, the user's own words, whether the user is the maintainer or a contributor, the approved design (or where it is written), every file changed so far (including new untracked files, which `git diff` doesn't show), the planned commit subject, and on a review round the earlier findings with the answers to them. Pass on what the next agent needs (findings, decisions, file lists), not whole reports or logs.
- **Continue an agent instead of starting a new one** for its next round: send the re-review to the reviewer that did the last review, and the fixes to the architect or implementer that did the work (SendMessage to the agent's id), so it doesn't read the docs again. Start a fresh one only for a new step, or when the old one's context is very long or out of date.
- **Read each result before acting on it.** A subagent's report is a claim: check the diff, the build output or the test log before telling the user something is done.
- **Review loops end.** If the reviewer and the author still disagree after three rounds, decide yourself from the evidence (the engine source, Lyra, a test or a measurement) and record why in the decision log. Take it to the user only when the choice changes game feel or design, or to the maintainer when it is on the "still needs the maintainer's yes" list, put as what each option costs and risks for the game.
- **If the subagents are missing on this machine,** run the same steps with general-purpose subagents, giving each the matching file's instructions if you can read it, or the role from "How work is done", and tell the user. The reasons for the workflow are recorded in the decision log entry of 2026-11-11 in `Docs/DecisionLog.md`.
