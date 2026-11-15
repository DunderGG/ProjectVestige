@AGENTS.md

## Claude Code notes

- The project rules live in `AGENTS.md` (imported above), `Docs/CodingGuidelines.md` and `Docs/Architecture.md`. Read both documents before writing or reviewing C++ in this repo, plus the guide in `Docs/Systems/` for each system the change touches.
- When working for David, the maintainer, these rules override Claude Code's default git attribution: commits carry no `Co-Authored-By:` or `Claude-Session:` lines, are authored by David Bennehag, and are never pushed or turned into pull requests. When working for another contributor, follow `CONTRIBUTING.md` instead.
- Before committing, run through the review checklist at the end of `Docs/CodingGuidelines.md`.
- Changes that need a manual check in the editor go into `Docs/PlayTest.md`, section "Pending checks", in the same commit, instead of only in your report.

## Specialist subagents

The roles in "How work is done" in `AGENTS.md` are subagents in `.claude/agents/`. Use them as the lane in `AGENTS.md` says; this file is the project's instruction to use subagents, so don't skip one that the lane calls for to save tokens.

| Role | Subagent |
|---|---|
| Game designer | `vestige-game-designer` (advises on game design; the user decides) |
| Architect | `vestige-architect` |
| Reviewer | `vestige-reviewer` (designs and code; review, then re-review) |
| Art reviewer | `vestige-art-reviewer` (art from the artist; review, then re-review) |
| Implementer | `vestige-implementer` |
| Test author | `vestige-test-author` |
| Artist | `vestige-artist` |

- **The main session runs the flow.** Current Claude Code versions let a subagent start subagents of its own (three layers deep by default), but this flow doesn't use that: a subagent can't ask the user questions, so the main session would still have to relay them, and a layer more is one more context to pay for. So you start each step, pass results between them, decide when a review round is done, relay only game feel and game design questions to the user, and commit.
- **Every subagent starts without this conversation.** Give each a brief that stands on its own: the task and roadmap item, the lane and the tripwires checked, the user's own words, whether the user is the maintainer or a contributor, the approved design (or where it is written), every file changed so far (including new untracked files, which `git diff` doesn't show), the planned commit subject, and on a review round the earlier findings with the answers to them. Pass on what the next agent needs (findings, decisions, file lists), not whole reports or logs. Give every agent of the task the same report folder (a temporary folder outside the repository): agents write their full reports there and reply in about 300 words, and the next agent gets the report file's path instead of its text. Read a report file yourself only when you need a detail from it.
- **Continue an agent instead of starting a new one** for its next round: send the re-review to the reviewer that did the last review, and the fixes to the architect or implementer that did the work (SendMessage to the agent's id), so it doesn't read the docs again. Start a fresh one only for a new step, or when the old one's context is very long or out of date.
- **Read each result before acting on it.** A subagent's report is a claim: check the diff and the summary of `Tools/Verify-Vestige.ps1` (or its logs) before telling the user something is done, and don't re-read whole reports to do it.
- **Few review rounds.** Send the author all of a review's findings at once (the report file) and have it fix them all in one round; reviewers raise every finding in their first review and approve with nits once only Low findings are left.
- **One roadmap item per session.** Start each roadmap item in a fresh session: everything durable is already in the docs and in git, and a long conversation is paid for again on every turn. When an item is committed, say so and leave the next item to a new session instead of carrying on in this one. The 200,000-token handoff below stays as a backstop for an item that long.
- **Keep the conversation short.** Every turn re-reads the whole conversation, so its length times the number of turns is most of a task's cost (the crops task in November 2026: about 92 million tokens read by the main session against about 1 million by all its agents). Don't paste reports, diffs or logs into the conversation, and don't wake for status checks. When the conversation passes about 200,000 tokens, finish the current step, write a handoff note to the report folder (the task, the decisions so far, the files changed, the report files, what is left) and continue the task in a fresh session from that note: with the desktop app's hand-off tool where there is one, otherwise by telling the user.
- **Review loops end.** If the reviewer and the author still disagree after three rounds, decide yourself from the evidence (the engine source, Lyra, a test or a measurement) and record why in the decision log. Take it to the user only when the choice changes game feel or design, or to the maintainer when it is on the "still needs the maintainer's yes" list, put as what each option costs and risks for the game.
- **If the subagents are missing on this machine,** run the same steps with general-purpose subagents, giving each the matching file's instructions if you can read it, or the role from "How work is done", and tell the user. The reasons for the workflow are recorded in the decision log entry of 2026-11-11 in `Docs/DecisionLog.md`.
