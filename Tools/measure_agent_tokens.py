# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Shows where the agent workflow's tokens go, from the Claude Code transcripts of this project.

Run (from the project folder; Python 3.9 or later, standard library only; the engine's bundled
Python works too):
    python Tools/measure_agent_tokens.py
    python Tools/measure_agent_tokens.py --session latest

It reads the transcripts Claude Code keeps for this repository (one .jsonl file per main session,
and one per subagent in <session-id>/subagents/) and prints token counts and an estimated cost. It
prints numbers only, never transcript text; the one exception is a session's label (see below).

Without --session it lists the last sessions, one line each, with the main session and all its
subagents added up. With --session it shows one session as a table with a row per role of the flow
in .claude/agent-flow.puml: the main session, then each subagent by its agent type. A reviewer's
work is split by the time of each API call: calls before the session's first implementer started
are the design review, later ones the code review (in a session with an architect but no
implementer, all of it is design review; with neither, all of it is code review), so one reviewer
agent that did both reviews counts in both rows. Runs are one per subagent plus one for each time
the main session continued it with SendMessage, a reviewer's runs placed by their own time.

A turn is one API call (one message id; Claude Code writes one call as several lines). Fresh input,
cache write and cache read add up to the context sent on that call. The cost is estimated at the API
list prices in PRICES below. On a subscription it is a proxy for how fast the usage limits are used,
not a bill. A model without a price prints a warning; its tokens are counted but cost nothing in the
totals.

The totals are a lower bound: Claude Code does not write failed or interrupted API calls to the
transcript (in one session, 108 api-request records against 102 replies, $7.67 here against the
$8.35 Claude Code itself reported).

The label of a session is the <slug> of the first Saved/AgentWork/<slug>/ file the main session
writes or edits (else the first one any tool call names); else the user's first message (a <message from="human"> in the prompt that woke the session)
when it has 4 or more words after a leading [...] is removed; else the coordinator's <note>; else
that message anyway. It is the first 60 characters.

Options:
    --last N            how many of the newest sessions to list, by start time (default 5)
    --session ID        a session id, a unique start of one, or "latest": the newest session (by
                        start time) that has subagents; the session running the command counts too
    --project-dir DIR   the folder with the transcripts; by default the Claude Code folder for this
                        repository, ~/.claude/projects/<the repository path with every character but
                        letters and digits turned into ->
"""

import argparse
import html
import json
import re
import sys
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# API list prices in USD per million tokens, by model id without its "claude-" prefix:
# (input, output, cache read).
PRICES = {
    "opus-5-5": (4.0, 20.0, 0.20),
    "opus-5": (5.0, 25.0, 0.50),
    "opus-4-8": (5.0, 25.0, 0.50),
    "opus-4-7": (5.0, 25.0, 0.50),
    "opus-4-6": (5.0, 25.0, 0.50),
    "sonnet-5-5": (2.0, 10.0, 0.10),
    "sonnet-5": (2.0, 10.0, 0.20),
    "sonnet-4-6": (3.0, 15.0, 0.30),
    "haiku-5-5": (0.10, 0.50, 0.01),
}
# A cache write costs this many times the input price, by how long the cache lives.
CACHE_WRITE_5M_FACTOR = 1.25
CACHE_WRITE_1H_FACTOR = 2.0
TOKENS_PER_MILLION = 1_000_000.0
LABEL_LENGTH = 60
# A user message with fewer words than this is too short to name a session ("yes").
LABEL_MIN_WORDS = 4
# How many of a session's first prompts are searched for the user's own words.
LABEL_PROMPT_LIMIT = 6
SESSION_ID_SHOWN = 8
SYNTHETIC_MODEL = "<synthetic>"

MAIN_ROLE = "Main session"
DESIGN_REVIEW_ROLE = "Design review"
CODE_REVIEW_ROLE = "Code review"
OTHER_ROLE = "Other"
REVIEWER_TYPE = "vestige-reviewer"
IMPLEMENTER_TYPE = "vestige-implementer"
ARCHITECT_TYPE = "vestige-architect"
ROLE_ORDER = [MAIN_ROLE, "Game designer", "Architect", DESIGN_REVIEW_ROLE, "Implementer", "Test author",
              CODE_REVIEW_ROLE, "Artist", "Art review", "Writer", OTHER_ROLE]
ROLE_BY_AGENT_TYPE = {
    "vestige-game-designer": "Game designer",
    ARCHITECT_TYPE: "Architect",
    IMPLEMENTER_TYPE: "Implementer",
    "vestige-test-author": "Test author",
    "vestige-artist": "Artist",
    "vestige-art-reviewer": "Art review",
    "vestige-writer": "Writer",
}

TIMESTAMP_PATTERN = re.compile(r'"timestamp":"([^"]+)"')
HUMAN_MESSAGE_PATTERN = re.compile(r'<message\b[^>]*\bfrom="human[^>]*>(.*?)</message>', re.S)
NOTE_PATTERN = re.compile(r"<note>(.*?)</note>", re.S)
BRIEF_PATH_PATTERN = re.compile(r"Saved[\/]+AgentWork[\/]+([A-Za-z0-9_-]+)[\/]", re.I)
WORK_PATH_PATTERN = re.compile(r"Saved[\\/]+AgentWork[\\/]+([^\\/]+)[\\/]", re.I)
LEADING_BRACKET_PATTERN = re.compile(r"^\s*\[[^\]]*\]\s*")
TAG_PATTERN = re.compile(r"<[^>]+>")
WHITESPACE_PATTERN = re.compile(r"\s+")

unknown_models = set()
parse_errors = 0
multi_iteration_calls = 0


@dataclass
class Totals:
    turns: int = 0
    fresh: int = 0
    cache_write: int = 0
    cache_read: int = 0
    output: int = 0
    cost: float = 0.0

    def add(self, other: "Totals") -> None:
        self.turns += other.turns
        self.fresh += other.fresh
        self.cache_write += other.cache_write
        self.cache_read += other.cache_read
        self.output += other.output
        self.cost += other.cost

    def tokens(self) -> int:
        return self.fresh + self.cache_write + self.cache_read + self.output


@dataclass
class Labels:
    """The label candidates from a session's first prompts."""

    human: Optional[str] = None
    human_stripped: Optional[str] = None
    note: Optional[str] = None
    plain: Optional[str] = None
    prompts_read: int = 0

    def best(self) -> Optional[str]:
        if self.human_stripped and len(self.human_stripped.split()) >= LABEL_MIN_WORDS:
            return self.human_stripped
        return self.note or self.human_stripped or self.human or self.plain


@dataclass
class Transcript:
    start: Optional[datetime] = None
    totals: Totals = field(default_factory=Totals)
    first_context: Optional[int] = None
    last_context: Optional[int] = None
    label: Optional[str] = None
    brief_slug: Optional[str] = None
    fallback_slug: Optional[str] = None
    # Each unique API call's time and tokens, in file order.
    calls: List[Tuple[Optional[datetime], Totals]] = field(default_factory=list)
    # SendMessage calls by the agent id they went to: the time of each.
    send_times: Dict[str, List[Optional[datetime]]] = field(default_factory=dict)


@dataclass
class Agent:
    agent_type: Optional[str]
    start: Optional[datetime]
    transcript: Transcript


@dataclass
class Session:
    session_id: str
    start: Optional[datetime]
    label: str
    main: Transcript
    agents: List[Agent]
    total: Totals
    role_totals: Dict[str, Totals]
    role_runs: Dict[str, int]


def format_count(value: int) -> str:
    return f"{value:,}"


def format_cost(value: float) -> str:
    return f"${value:,.2f}"


def format_local_time(time: Optional[datetime]) -> str:
    if time is None:
        return "?"
    return time.astimezone().strftime("%Y-%m-%d %H:%M")


def format_share(cost: float, total_cost: float) -> str:
    if total_cost <= 0:
        return "-"
    return f"{cost * 100.0 / total_cost:.1f}%"


def parse_time(text: Optional[str]) -> Optional[datetime]:
    if not text:
        return None
    text = text.replace("Z", "+00:00")
    try:
        return datetime.fromisoformat(text)
    except ValueError:
        pass
    # Python before 3.11 reads only 3 or 6 digits of a second's fraction.
    try:
        return datetime.fromisoformat(re.sub(r"\.\d+", "", text))
    except ValueError:
        return None


def line_time(line: str) -> Optional[datetime]:
    match = TIMESTAMP_PATTERN.search(line)
    return parse_time(match.group(1)) if match else None


def read_lines(path: Path):
    with open(path, encoding="utf-8", errors="replace") as transcript:
        yield from transcript


def file_start(path: Path) -> Optional[datetime]:
    for line in read_lines(path):
        time = line_time(line)
        if time is not None:
            return time
    return None


def model_price(model: str):
    key = re.sub(r"-\d{8}$", "", re.sub(r"\[1m\]$", "", re.sub(r"^claude-", "", model)))
    if key in PRICES:
        return PRICES[key]
    unknown_models.add(model)
    return None


def prompt_text(record: dict) -> Optional[str]:
    """The text of a prompt line: a user line (not a tool result) or a prompt queued for the session."""
    if record.get("type") == "queue-operation":
        content = record.get("content")
        return content if record.get("operation") == "enqueue" and isinstance(content, str) else None
    content = (record.get("message") or {}).get("content")
    if isinstance(content, str):
        return content
    text = None
    for block in content or []:
        if not isinstance(block, dict):
            continue
        if block.get("type") == "tool_result":
            return None
        if block.get("type") == "text" and not text:
            text = block.get("text")
    return text


def format_label(text: str) -> Optional[str]:
    text = WHITESPACE_PATTERN.sub(" ", html.unescape(TAG_PATTERN.sub(" ", text))).strip()
    return text[:LABEL_LENGTH] if text else None


def add_label_candidate(labels: Labels, record: dict) -> None:
    """Collects label candidates from the first prompts: the user's own words (a <message from="human">
    in the prompt that woke the session), the coordinator's <note> (a meta line that starts the
    session) and the first plain prompt. Other meta lines (reminders, skill text) are not prompts."""
    text = prompt_text(record)
    if not text:
        return
    labels.prompts_read += 1
    human = HUMAN_MESSAGE_PATTERN.search(text)
    if human:
        labels.human = format_label(human.group(1))
        labels.human_stripped = format_label(LEADING_BRACKET_PATTERN.sub("", html.unescape(human.group(1))))
        return
    note = NOTE_PATTERN.search(text)
    if note:
        labels.note = labels.note or format_label(note.group(1))
    elif not record.get("isMeta") and record.get("type") == "user":
        labels.plain = labels.plain or format_label(text)


def add_usage(totals: Totals, message: dict) -> int:
    """Adds one API call's usage to the totals and returns the context it sent."""
    global multi_iteration_calls
    usage = message["usage"]
    fresh = int(usage.get("input_tokens") or 0)
    cache_write = int(usage.get("cache_creation_input_tokens") or 0)
    cache_read = int(usage.get("cache_read_input_tokens") or 0)
    output = int(usage.get("output_tokens") or 0)
    cache_write_1h = int((usage.get("cache_creation") or {}).get("ephemeral_1h_input_tokens") or 0)
    cache_write_5m = cache_write - cache_write_1h
    iterations = usage.get("iterations")
    if isinstance(iterations, list) and len(iterations) > 1:
        multi_iteration_calls += 1

    totals.turns += 1
    totals.fresh += fresh
    totals.cache_write += cache_write
    totals.cache_read += cache_read
    totals.output += output

    price = model_price(str(message.get("model")))
    if price:
        input_price, output_price, cache_read_price = price
        cost = (fresh * input_price + cache_write_5m * input_price * CACHE_WRITE_5M_FACTOR
                + cache_write_1h * input_price * CACHE_WRITE_1H_FACTOR + cache_read * cache_read_price
                + output * output_price)
        totals.cost += cost / TOKENS_PER_MILLION
    return fresh + cache_write + cache_read


def scan_tool_uses(message: dict, time: Optional[datetime], result: Transcript, seen_tool_uses: set) -> None:
    """Notes the SendMessage calls that continued a subagent (by the agent id they went to) and the
    first scratch-folder file the session wrote or edited (by its folder name)."""
    for block in message.get("content") or []:
        if not isinstance(block, dict) or block.get("type") != "tool_use" or block.get("id") in seen_tool_uses:
            continue
        seen_tool_uses.add(block.get("id"))
        tool_input = block.get("input") or {}
        if block.get("name") == "SendMessage":
            target = tool_input.get("to")
            if isinstance(target, str) and target:
                result.send_times.setdefault(target, []).append(time)
        elif block.get("name") in ("Write", "Edit") and result.brief_slug is None:
            path = tool_input.get("file_path")
            match = WORK_PATH_PATTERN.search(path) if isinstance(path, str) else None
            if match:
                result.brief_slug = match.group(1)
        if result.fallback_slug is None:
            # A brief written through Bash is named in the prompts and commands that point to it; a placeholder like <slug> is not a name.
            for value in tool_input.values():
                match = BRIEF_PATH_PATTERN.search(value) if isinstance(value, str) else None
                if match:
                    result.fallback_slug = match.group(1)
                    break


def measure_transcript(path: Path, is_main: bool = False) -> Transcript:
    global parse_errors
    result = Transcript()
    labels = Labels()
    seen_messages = set()
    seen_tool_uses = set()
    for line in read_lines(path):
        if result.start is None:
            result.start = line_time(line)
        # Unescaped quotes only occur in the JSON structure itself, never inside a string value.
        is_assistant = '"type":"assistant"' in line
        wants_label = (is_main and not labels.human and labels.prompts_read < LABEL_PROMPT_LIMIT
                       and ('"type":"user"' in line or '"type":"queue-operation"' in line))
        if not (is_assistant or wants_label):
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            parse_errors += 1
            continue

        record_type = record.get("type")
        if record_type in ("user", "queue-operation"):
            if wants_label:
                add_label_candidate(labels, record)
            continue
        if record_type != "assistant":
            continue
        message = record.get("message") or {}
        time = parse_time(record.get("timestamp"))
        if is_main:
            scan_tool_uses(message, time, result, seen_tool_uses)
        if not message.get("usage") or message.get("model") == SYNTHETIC_MODEL:
            continue
        message_id = message.get("id")
        if message_id:
            if message_id in seen_messages:
                continue
            seen_messages.add(message_id)
        call = Totals()
        context = add_usage(call, message)
        result.totals.add(call)
        result.calls.append((time, call))
        if result.first_context is None:
            result.first_context = context
        result.last_context = context
    result.label = result.brief_slug or result.fallback_slug or labels.best()
    return result


def agent_type(meta_path: Path) -> Optional[str]:
    try:
        with open(meta_path, encoding="utf-8") as meta:
            return json.load(meta).get("agentType")
    except (OSError, ValueError):
        return None


def subagent_files(main_file: Path) -> List[Path]:
    subagent_dir = main_file.parent / main_file.stem / "subagents"
    if not subagent_dir.is_dir():
        return []
    return sorted(subagent_dir.glob("agent-*.jsonl"))


def measure_session(main_file: Path) -> Session:
    main = measure_transcript(main_file, is_main=True)
    agents = []
    for agent_file in subagent_files(main_file):
        measured = measure_transcript(agent_file)
        agents.append(Agent(agent_type(agent_file.with_name(agent_file.stem + ".meta.json")), measured.start, measured))

    implementer_starts = [agent.start for agent in agents if agent.agent_type == IMPLEMENTER_TYPE and agent.start]
    first_implementer_start = min(implementer_starts) if implementer_starts else None
    has_architect = any(agent.agent_type == ARCHITECT_TYPE for agent in agents)

    def reviewer_role(time: Optional[datetime]) -> str:
        if first_implementer_start is None:
            return DESIGN_REVIEW_ROLE if has_architect else CODE_REVIEW_ROLE
        return DESIGN_REVIEW_ROLE if time is not None and time < first_implementer_start else CODE_REVIEW_ROLE

    role_totals = {MAIN_ROLE: main.totals}
    role_runs = {MAIN_ROLE: 1}
    total = Totals()
    total.add(main.totals)
    for agent in agents:
        total.add(agent.transcript.totals)
    for agent_file, agent in zip(subagent_files(main_file), agents):
        agent_id = agent_file.stem[len("agent-"):]
        run_times = [agent.start] + main.send_times.get(agent_id, [])
        if agent.agent_type == REVIEWER_TYPE:
            for time, call in agent.transcript.calls:
                role_totals.setdefault(reviewer_role(time), Totals()).add(call)
            for time in run_times:
                role = reviewer_role(time)
                role_runs[role] = role_runs.get(role, 0) + 1
        else:
            role = ROLE_BY_AGENT_TYPE.get(agent.agent_type, OTHER_ROLE)
            role_totals.setdefault(role, Totals()).add(agent.transcript.totals)
            role_runs[role] = role_runs.get(role, 0) + len(run_times)
    return Session(main_file.stem, main.start, main.label or "(no prompt)", main, agents, total, role_totals,
                   role_runs)


def print_warnings() -> None:
    for model in sorted(unknown_models):
        print(f"Warning: no price for model '{model}'; its tokens are counted, its cost is left out.")
    if parse_errors:
        print(f"Warning: {parse_errors} transcript line(s) could not be read and are left out.")
    if multi_iteration_calls:
        print(f"Warning: {multi_iteration_calls} API call(s) have more than one usage iteration (advisor calls?); "
              "they may be counted differently than Claude Code counts them.")


def print_session_list(main_files: List[Path]) -> None:
    row = "{:<16}  {:<8}  {:>6}  {:>13}  {:>10}  {}"
    lines = [row.format("Start", "Session", "Turns", "Tokens", "Est. cost", "Label")]
    for main_file in main_files:
        session = measure_session(main_file)
        lines.append(row.format(format_local_time(session.start), session.session_id[:SESSION_ID_SHOWN],
                                session.total.turns, format_count(session.total.tokens()),
                                format_cost(session.total.cost), session.label))
    print_warnings()
    print("\n".join(lines))
    print("Turns, tokens and cost add up the main session and all its subagents. Details: --session <id or latest>.")


def table_row(role: str, runs: int, totals: Totals, total_cost: float) -> str:
    row = "{:<13}  {:>4}  {:>6}  {:>11}  {:>12}  {:>13}  {:>11}  {:>10}  {:>6}"
    return row.format(role, runs, totals.turns, format_count(totals.fresh), format_count(totals.cache_write),
                      format_count(totals.cache_read), format_count(totals.output), format_cost(totals.cost),
                      format_share(totals.cost, total_cost))


def print_session_table(main_file: Path) -> None:
    session = measure_session(main_file)
    header = "{:<13}  {:>4}  {:>6}  {:>11}  {:>12}  {:>13}  {:>11}  {:>10}  {:>6}"
    lines = [f"Session {session.session_id}, started {format_local_time(session.start)}: {session.label}",
             header.format("Role", "Runs", "Turns", "Fresh in", "Cache write", "Cache read", "Output", "Est. cost",
                           "Share")]
    for role in ROLE_ORDER:
        lines.append(table_row(role, session.role_runs.get(role, 0), session.role_totals.get(role, Totals()),
                               session.total.cost))
    lines.append(table_row("Total", sum(session.role_runs.values()), session.total, session.total.cost))

    main = session.main
    if main.first_context is not None:
        lines.append(f"Main session context: {format_count(main.first_context)} tokens at its first API call, "
                     f"{format_count(main.last_context)} at its last.")
    print_warnings()
    print("\n".join(lines))


def default_project_dir() -> Path:
    repository_root = Path(__file__).resolve().parent.parent
    # Claude Code names a project's folder after its path, with every other character turned into -.
    return Path.home() / ".claude" / "projects" / re.sub(r"[^A-Za-z0-9]", "-", str(repository_root))


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Shows where the agent workflow's tokens go, from the Claude Code "
                                                 "transcripts of this project. See the module docstring for details.")
    parser.add_argument("--last", type=int, default=5, help="how many of the newest sessions to list (default 5)")
    parser.add_argument("--session", help='a session id, a unique start of one, or "latest"')
    parser.add_argument("--project-dir", type=Path, help="the folder with the transcripts")
    arguments = parser.parse_args()
    if arguments.last < 1:
        parser.error("--last must be 1 or more")
    return arguments


def stop(reason: str) -> None:
    print(f"measure_agent_tokens: {reason}")
    sys.exit(1)


def main() -> None:
    # Labels can hold any character; don't let a narrow console encoding stop the run.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="replace")
    arguments = parse_arguments()
    project_dir = arguments.project_dir or default_project_dir()
    if not project_dir.is_dir():
        stop(f"no transcripts found: {project_dir}")

    sessions = []
    for path in project_dir.glob("*.jsonl"):
        start = file_start(path)
        if start is not None:
            sessions.append((start, path))
    sessions.sort(key=lambda session: session[0])
    if not sessions:
        stop(f"no sessions found in {project_dir}")

    if not arguments.session:
        print_session_list([path for _, path in sessions[-arguments.last:]])
        return

    if arguments.session == "latest":
        chosen = [path for _, path in sessions if subagent_files(path)][-1:]
        if not chosen:
            stop("no session with subagents found")
    else:
        prefix = arguments.session.lower()
        chosen = [path for _, path in sessions if path.stem.lower().startswith(prefix)]
        if not chosen:
            stop(f"no session id starts with '{arguments.session}'")
        if len(chosen) > 1:
            stop(f"{len(chosen)} sessions start with '{arguments.session}'; give more of the id")
    print_session_table(chosen[0])


if __name__ == "__main__":
    main()
