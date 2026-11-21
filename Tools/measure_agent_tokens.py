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

With --html it also writes a dashboard: one self-contained HTML file (inline CSS, SVG and JavaScript,
no network, no third-party code) with the numbers of the shown sessions as JSON in the page: the cost
and tokens per role, main session against agents, the token kinds, a comparison of the tasks, and a
timeline of one task's agent runs, each with hover numbers and a table. The sessions shown are the
ones --last or --session select; the text output above is written as before. A timeline bar spans a
run's first to last API call, not the time the agent waited; idle gaps over 10 minutes are drawn short.

Options:
    --last N            how many of the newest sessions to list, by start time (default 5)
    --session ID        a session id, a unique start of one, or "latest": the newest session (by
                        start time) that has subagents; the session running the command counts too
    --html [PATH]       also write the HTML dashboard of the selected sessions; without a PATH,
                        Saved/TokenDashboard/token-dashboard.html under the repository root
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
from typing import Callable, Dict, List, Optional, Tuple

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
    # The cost split by kind of token; adds up to cost.
    cost_fresh: float = 0.0
    cost_cache_write: float = 0.0
    cost_cache_read: float = 0.0
    cost_output: float = 0.0

    def add(self, other: "Totals") -> None:
        self.turns += other.turns
        self.fresh += other.fresh
        self.cache_write += other.cache_write
        self.cache_read += other.cache_read
        self.output += other.output
        self.cost += other.cost
        self.cost_fresh += other.cost_fresh
        self.cost_cache_write += other.cost_cache_write
        self.cost_cache_read += other.cost_cache_read
        self.cost_output += other.cost_output

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
class Run:
    """One run of a subagent: its first start, or one continuation by the main session (SendMessage)."""

    agent_index: int
    agent_id: str
    role: str
    continuation: bool
    start: Optional[datetime]
    end: Optional[datetime]
    totals: Totals


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
    runs: List[Run] = field(default_factory=list)


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
        cost_fresh = fresh * input_price
        cost_cache_write = (cache_write_5m * input_price * CACHE_WRITE_5M_FACTOR
                            + cache_write_1h * input_price * CACHE_WRITE_1H_FACTOR)
        cost_cache_read = cache_read * cache_read_price
        cost_output = output * output_price
        totals.cost_fresh += cost_fresh / TOKENS_PER_MILLION
        totals.cost_cache_write += cost_cache_write / TOKENS_PER_MILLION
        totals.cost_cache_read += cost_cache_read / TOKENS_PER_MILLION
        totals.cost_output += cost_output / TOKENS_PER_MILLION
        totals.cost += (cost_fresh + cost_cache_write + cost_cache_read + cost_output) / TOKENS_PER_MILLION
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


def split_runs(agent: Agent, agent_index: int, agent_id: str, send_times: List[Optional[datetime]],
               reviewer_role: Callable[[Optional[datetime]], str]) -> List[Run]:
    """Cuts an agent's API calls into runs at the times the main session continued it with SendMessage.
    A run's role is the agent's role; a reviewer's by the time of the run's first call."""
    boundaries = sorted(time for time in send_times if time is not None)
    groups: Dict[int, List[Tuple[Optional[datetime], Totals]]] = {}
    for time, call in agent.transcript.calls:
        index = sum(1 for boundary in boundaries if time is not None and boundary <= time)
        groups.setdefault(index, []).append((time, call))

    def role_of(time: Optional[datetime]) -> str:
        if agent.agent_type == REVIEWER_TYPE:
            return reviewer_role(time)
        return ROLE_BY_AGENT_TYPE.get(agent.agent_type, OTHER_ROLE)

    runs = []
    # A continuation that made no API call is kept as an empty run, so the runs match the run counts.
    for index in range(1, len(boundaries) + 1):
        if index not in groups:
            runs.append(Run(agent_index, agent_id, role_of(boundaries[index - 1]), True, boundaries[index - 1],
                            boundaries[index - 1], Totals()))
    for index in sorted(groups):
        calls = groups[index]
        times = [time for time, _ in calls if time is not None]
        totals = Totals()
        for _, call in calls:
            totals.add(call)
        start = min(times) if times else None
        runs.append(Run(agent_index, agent_id, role_of(start), index > 0, start, max(times) if times else None, totals))
    return runs


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
    runs = []
    for agent_index, (agent_file, agent) in enumerate(zip(subagent_files(main_file), agents)):
        agent_id = agent_file.stem[len("agent-"):]
        run_times = [agent.start] + main.send_times.get(agent_id, [])
        runs.extend(split_runs(agent, agent_index, agent_id, main.send_times.get(agent_id, []), reviewer_role))
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
                   role_runs, runs)


def warning_lines() -> List[str]:
    lines = [f"no price for model '{model}'; its tokens are counted, its cost is left out."
             for model in sorted(unknown_models)]
    if parse_errors:
        lines.append(f"{parse_errors} transcript line(s) could not be read and are left out.")
    if multi_iteration_calls:
        lines.append(f"{multi_iteration_calls} API call(s) have more than one usage iteration (advisor calls?); "
                     "they may be counted differently than Claude Code counts them.")
    return lines


def print_warnings() -> None:
    for line in warning_lines():
        print(f"Warning: {line}")


def print_session_list(sessions: List[Session]) -> None:
    row = "{:<16}  {:<8}  {:>6}  {:>13}  {:>10}  {}"
    lines = [row.format("Start", "Session", "Turns", "Tokens", "Est. cost", "Label")]
    for session in sessions:
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


def print_session_table(session: Session) -> None:
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


def totals_data(totals: Totals) -> dict:
    return {"turns": totals.turns, "tokens": totals.tokens(), "fresh": totals.fresh, "cache_write": totals.cache_write,
            "cache_read": totals.cache_read, "output": totals.output, "cost": totals.cost,
            "cost_fresh": totals.cost_fresh, "cost_cache_write": totals.cost_cache_write,
            "cost_cache_read": totals.cost_cache_read, "cost_output": totals.cost_output}


def epoch(time: Optional[datetime]) -> Optional[float]:
    return time.timestamp() if time else None


def session_data(session: Session) -> dict:
    """One session as numbers and labels for the dashboard: never transcript text."""
    main_times = [time for time, _ in session.main.calls if time is not None]
    roles = {}
    for role, totals in session.role_totals.items():
        roles[role] = totals_data(totals)
        roles[role]["runs"] = session.role_runs.get(role, 0)
    return {
        "id": session.session_id,
        "label": session.label,
        "start": epoch(session.start),
        "total": totals_data(session.total),
        "roles": roles,
        "role_runs": session.role_runs,
        "agent_count": len(session.agents),
        "main_calls": [round(epoch(time), 1) for time in sorted(main_times)],
        "runs": [{"agent": run.agent_index, "role": run.role, "continuation": run.continuation,
                  "start": epoch(run.start), "end": epoch(run.end), "totals": totals_data(run.totals)}
                 for run in session.runs],
    }


def write_dashboard(sessions: List[Session], path: Path) -> None:
    """Writes the dashboard: one self-contained HTML file with the data as JSON in the page."""
    data = {"generated": datetime.now().timestamp(), "roles_order": ROLE_ORDER, "warnings": warning_lines(),
            "sessions": [session_data(session) for session in sessions]}
    # <, > and & are escaped, so no value can end the script element or start a comment in it.
    payload = (json.dumps(data, separators=(",", ":")).replace("<", "\\u003c").replace(">", "\\u003e")
               .replace("&", "\\u0026"))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(DASHBOARD_TEMPLATE.replace("__DATA__", payload), encoding="utf-8")


def default_project_dir() -> Path:
    repository_root = Path(__file__).resolve().parent.parent
    # Claude Code names a project's folder after its path, with every other character turned into -.
    return Path.home() / ".claude" / "projects" / re.sub(r"[^A-Za-z0-9]", "-", str(repository_root))


def default_dashboard_path() -> Path:
    return Path(__file__).resolve().parent.parent / "Saved" / "TokenDashboard" / "token-dashboard.html"


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Shows where the agent workflow's tokens go, from the Claude Code "
                                                 "transcripts of this project. See the module docstring for details.")
    parser.add_argument("--last", type=int, default=5, help="how many of the newest sessions to list (default 5)")
    parser.add_argument("--session", help='a session id, a unique start of one, or "latest"')
    parser.add_argument("--html", nargs="?", type=Path, const=default_dashboard_path(), metavar="PATH",
                        help="also write the HTML dashboard (default Saved/TokenDashboard/token-dashboard.html)")
    parser.add_argument("--project-dir", type=Path, help="the folder with the transcripts")
    arguments = parser.parse_args()
    if arguments.last < 1:
        parser.error("--last must be 1 or more")
    return arguments


def stop(reason: str) -> None:
    print(f"measure_agent_tokens: {reason}")
    sys.exit(1)


DASHBOARD_TEMPLATE = r'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Agent token dashboard</title>
<style>
:root {
  --bg: #fcfcfb; --panel: #ffffff; --text: #0b0b0b; --muted: #52514e; --border: #e3e2dc; --grid: #ecebe6;
  --c1: #2a78d6; --c2: #eb6834; --c3: #1baf7a; --c4: #eda100; --hover: #f3f2ee;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #1a1a19; --panel: #222221; --text: #f0efec; --muted: #c3c2b7; --border: #383835; --grid: #2d2d2b;
    --c1: #3987e5; --c2: #d95926; --c3: #199e70; --c4: #c98500; --hover: #2a2a28;
  }
}
:root[data-theme="dark"] {
  --bg: #1a1a19; --panel: #222221; --text: #f0efec; --muted: #c3c2b7; --border: #383835; --grid: #2d2d2b;
  --c1: #3987e5; --c2: #d95926; --c3: #199e70; --c4: #c98500; --hover: #2a2a28;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--text); font: 15px/1.45 system-ui, "Segoe UI", sans-serif; }
main { max-width: 1100px; min-width: 0; margin: 0 auto; padding: 16px; }
header.top { display: flex; flex-wrap: wrap; gap: 12px; align-items: center; justify-content: space-between; }
h1 { font-size: 1.4rem; margin: 0; }
h2 { font-size: 1.05rem; margin: 0 0 4px; }
p.sub, .note { color: var(--muted); margin: 2px 0 10px; font-size: 0.9rem; }
.toolbar { display: flex; gap: 8px; flex-wrap: wrap; align-items: center; }
button, select { font: inherit; color: var(--text); background: var(--panel); border: 1px solid var(--border);
  border-radius: 6px; padding: 4px 10px; cursor: pointer; max-width: 100%; min-width: 0; }
.toolbar > * { min-width: 0; }
button[aria-pressed="true"] { border-color: var(--text); font-weight: 600; }
button:focus-visible, select:focus-visible, summary:focus-visible { outline: 2px solid var(--c1); outline-offset: 2px; }
.seg-btn { display: inline-flex; gap: 0; }
.seg-btn button { border-radius: 0; margin-left: -1px; }
.seg-btn button:first-child { border-radius: 6px 0 0 6px; margin-left: 0; }
.seg-btn button:last-child { border-radius: 0 6px 6px 0; }
.tiles { display: grid; grid-template-columns: repeat(auto-fit, minmax(min(150px, 100%), 1fr)); gap: 10px; margin: 14px 0; }
.tile, section.card { background: var(--panel); border: 1px solid var(--border); border-radius: 10px; padding: 12px 14px; }
.tile .k { color: var(--muted); font-size: 0.82rem; }
.tile .v { font-size: 1.5rem; font-weight: 600; font-variant-numeric: tabular-nums; }
section.card { margin: 14px 0; }
.legend { display: flex; flex-wrap: wrap; gap: 4px 14px; margin: 4px 0 8px; font-size: 0.85rem; color: var(--muted); }
.legend i { display: inline-block; width: 10px; height: 10px; border-radius: 2px; margin-right: 5px; vertical-align: -1px; }
.row { display: grid; grid-template-columns: minmax(80px, 150px) minmax(0, 1fr) minmax(70px, 120px); gap: 8px; align-items: center;
  padding: 3px 0; border-bottom: 1px solid var(--grid); }
.row .lab { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; font-size: 0.88rem; }
.row .val { font-size: 0.82rem; color: var(--muted); text-align: right; font-variant-numeric: tabular-nums; }
.track { display: flex; gap: 2px; height: 14px; width: 100%; }
.track .s { height: 100%; min-width: 0; border-radius: 2px; }
.track .s:hover, .tl-run:hover { filter: brightness(1.15); }
.c1 { background: var(--c1); } .c2 { background: var(--c2); } .c3 { background: var(--c3); } .c4 { background: var(--c4); }
details { margin-top: 8px; }
summary { cursor: pointer; color: var(--muted); font-size: 0.85rem; }
.tbl-wrap { overflow-x: auto; margin-top: 6px; border: 1px solid var(--border); border-radius: 6px; }
table { border-collapse: collapse; width: 100%; font-size: 0.82rem; font-variant-numeric: tabular-nums; }
th, td { padding: 4px 10px; text-align: right; white-space: nowrap; border-bottom: 1px solid var(--grid); }
th:first-child, td:first-child { text-align: left; }
th { color: var(--muted); font-weight: 600; }
#tip { position: fixed; z-index: 10; pointer-events: none; display: none; background: var(--text); color: var(--bg);
  padding: 6px 9px; border-radius: 6px; font-size: 0.8rem; white-space: pre-line; max-width: 280px; }
svg text { fill: var(--muted); font-size: 11px; font-family: inherit; }
svg .lab-main { fill: var(--text); }
.tl-wrap { overflow: hidden; width: 100%; }
.warn { color: var(--text); border-left: 3px solid var(--c4); padding: 2px 10px; margin: 6px 0; font-size: 0.88rem; }
ul.notes { margin: 6px 0; padding-left: 20px; color: var(--muted); font-size: 0.88rem; }
.stat { display: inline-block; margin-right: 18px; font-size: 0.9rem; }
.stat b { font-variant-numeric: tabular-nums; }
@media (max-width: 560px) { .row { grid-template-columns: 78px minmax(0, 1fr) 64px; } main { padding: 16px; } }
</style>
</head>
<body>
<main>
  <header class="top">
    <div>
      <h1>Agent token dashboard</h1>
      <p class="sub" id="subtitle"></p>
    </div>
    <div class="toolbar">
      <span class="seg-btn" role="group" aria-label="Metric">
        <button id="m-cost" aria-pressed="true">Cost</button><button id="m-tokens" aria-pressed="false">Tokens</button>
      </span>
      <button id="theme" aria-label="Toggle light and dark mode">Light / dark</button>
    </div>
  </header>
  <div id="warnings"></div>
  <div class="tiles" id="tiles"></div>

  <section class="card"><h2>Where the workflow's usage goes, by role</h2>
    <p class="note">All shown sessions added up. One bar per role of the agent flow.</p>
    <div id="roles"></div><div id="roles-table"></div></section>

  <section class="card"><h2>Main session against agents, per task</h2>
    <p class="note">The main session carries the whole conversation on every turn, so it is usually a large share.</p>
    <div class="legend" id="split-legend"></div>
    <div id="split"></div><div id="split-table"></div></section>

  <section class="card"><h2>Token kinds</h2>
    <p class="note">The same usage as share of tokens and as share of cost. Cache reads are most of the tokens but cost little; fresh input, cache writes and output are the rest of the tokens and most of the cost.</p>
    <div class="legend" id="kinds-legend"></div>
    <div id="kinds"></div><div id="kinds-table"></div></section>

  <section class="card"><h2>Tasks compared</h2>
    <p class="note">One bar per session, oldest first. Split by:</p>
    <div class="toolbar"><span class="seg-btn" role="group" aria-label="Split by">
      <button id="by-group" aria-pressed="true">Who spent it</button><button id="by-kind" aria-pressed="false">Kind of token</button>
    </span></div>
    <div class="legend" id="compare-legend"></div>
    <div id="compare"></div><div id="compare-table"></div></section>

  <section class="card"><h2>Timeline of one task</h2>
    <p class="note">The top row is the main session's API activity (calls less than 2 minutes apart make one block); below it, one row per agent. A bar spans a run's first to last API call (not the time the agent was waiting). Time is linear within each stretch, but idle stretches longer than 10 minutes are compressed into a short hatched break marked with a zig-zag and its real length, so long gaps do not shrink the runs. Circles mark hand-offs: hollow is the agent's first start, filled is the main session continuing it with SendMessage (a continuation that made no API call is a tiny bar with no turns).</p>
    <div class="toolbar"><label for="pick">Session</label><select id="pick"></select></div>
    <div id="rounds" style="margin:8px 0"></div>
    <div class="legend" id="tl-legend"></div>
    <div class="tl-wrap" id="timeline"></div><div id="timeline-table"></div></section>

  <section class="card"><h2>About these numbers</h2>
    <ul class="notes">
      <li>Prices are API list prices, so the cost is a proxy for how fast usage limits are used on a subscription, not a bill.</li>
      <li>Totals are a lower bound: failed or interrupted API calls are not written to transcripts (about 0 to 8 percent low).</li>
      <li>The split between design review and code review is by time: reviewer calls before the first implementer started are design review.</li>
      <li>Timeline bars span the first to the last API call of a run, not the time the agent was waiting for a result.</li>
      <li>A reviewer run is placed in design or code review by the time of its first call; the role totals split by each call.</li>
      <li>Only numbers and labels are shown, never transcript text. A task's label is its scratch folder name or, without one, the start of its first message.</li>
    </ul></section>
</main>
<div id="tip" role="tooltip"></div>
<script id="data" type="application/json">__DATA__</script>
<script>
"use strict";
const DATA = JSON.parse(document.getElementById("data").textContent);
const GROUP_OF = {"Main session": "Main", "Game designer": "Authors", "Architect": "Authors", "Implementer": "Authors",
  "Test author": "Authors", "Artist": "Authors", "Writer": "Authors", "Design review": "Reviewers",
  "Code review": "Reviewers", "Art review": "Reviewers", "Other": "Other"};
const GROUPS = ["Main", "Authors", "Reviewers", "Other"];
const KINDS = [["fresh", "Fresh input", "cost_fresh"], ["cache_write", "Cache write", "cost_cache_write"],
  ["cache_read", "Cache read", "cost_cache_read"], ["output", "Output", "cost_output"]];
const state = { metric: "cost", by: "group", session: busiestSession() };

function busiestSession() {
  let best = 0;
  DATA.sessions.forEach((s, i) => { if (s.total.cost > DATA.sessions[best].total.cost) best = i; });
  return best;
}
const esc = s => String(s).replace(/[&<>"']/g, c => ({"&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"}[c]));
const $ = id => document.getElementById(id);
const fmtInt = n => Math.round(n).toLocaleString("en-US");
const fmtCost = n => "$" + n.toLocaleString("en-US", {minimumFractionDigits: 2, maximumFractionDigits: 2});
const fmtShare = (n, total) => total > 0 ? (n * 100 / total).toFixed(1) + "%" : "-";
function fmtTok(n) {
  if (n >= 1e9) return (n / 1e9).toFixed(2) + " B";
  if (n >= 1e6) return (n / 1e6).toFixed(1) + " M";
  if (n >= 1e3) return (n / 1e3).toFixed(1) + " k";
  return String(Math.round(n));
}
const fmtMetric = n => state.metric === "cost" ? fmtCost(n) : fmtTok(n);
const pad2 = n => String(n).padStart(2, "0");
const fmtTime = sec => {
  if (sec == null) return "?";
  const d = new Date(sec * 1000);
  return `${d.getFullYear()}-${pad2(d.getMonth() + 1)}-${pad2(d.getDate())} ${pad2(d.getHours())}:${pad2(d.getMinutes())}`;
};
const fmtClock = sec => new Date(sec * 1000).toLocaleTimeString("en-GB", {hour: "2-digit", minute: "2-digit", hour12: false});
function fmtDuration(sec) {
  if (sec < 90) return Math.round(sec) + " s";
  if (sec < 5400) return Math.round(sec / 60) + " min";
  return (sec / 3600).toFixed(1) + " h";
}
const metricOf = t => state.metric === "cost" ? t.cost : t.tokens;
const sum = (list, f) => list.reduce((a, x) => a + f(x), 0);
function addTotals(a, b) { const r = {}; for (const k in b) r[k] = (a[k] || 0) + b[k]; return r; }
function emptyTotals() { return {turns: 0, tokens: 0, cost: 0, fresh: 0, cache_write: 0, cache_read: 0, output: 0,
  cost_fresh: 0, cost_cache_write: 0, cost_cache_read: 0, cost_output: 0}; }
function allRoles() {
  const roles = {};
  for (const role of DATA.roles_order) roles[role] = Object.assign(emptyTotals(), {runs: 0});
  for (const s of DATA.sessions) for (const role in s.roles) roles[role] = addTotals(roles[role] || emptyTotals(), s.roles[role]);
  return roles;
}
function groupTotals(session) {
  const groups = {};
  for (const g of GROUPS) groups[g] = emptyTotals();
  for (const role in session.roles) groups[GROUP_OF[role] || "Other"] = addTotals(groups[GROUP_OF[role] || "Other"], session.roles[role]);
  return groups;
}
function shortLabel(s) { return s.label; }
function sessionName(s) { return fmtTime(s.start).slice(5, 10) + " " + shortLabel(s); }

function legend(el, names) {
  $(el).innerHTML = names.map((n, i) => `<span><i class="c${i + 1}"></i>${esc(n)}</span>`).join("");
}
function table(headers, rows) {
  const head = headers.map(h => `<th>${esc(h)}</th>`).join("");
  const body = rows.map(r => "<tr>" + r.map(c => `<td>${esc(c)}</td>`).join("") + "</tr>").join("");
  return `<details><summary>Table</summary><div class="tbl-wrap"><table><thead><tr>${head}</tr></thead><tbody>${body}</tbody></table></div></details>`;
}
// rows: {label, tip?, segs: [{value, slot, tip}], valueText}
function bars(el, rows, scale) {
  const max = scale || Math.max(1e-9, ...rows.map(r => sum(r.segs, s => s.value)));
  $(el).innerHTML = rows.map(r => {
    const segs = r.segs.filter(s => s.value > 0).map(s =>
      `<div class="s c${s.slot}" style="width:${(s.value * 100 / max).toFixed(3)}%" data-tip="${esc(s.tip)}"></div>`).join("");
    return `<div class="row"><div class="lab" title="${esc(r.label)}">${esc(r.label)}</div><div class="track">${segs}</div><div class="val">${esc(r.valueText)}</div></div>`;
  }).join("");
}

function renderTiles() {
  const ss = DATA.sessions;
  const total = ss.reduce((a, s) => addTotals(a, s.total), emptyTotals());
  const main = sum(ss, s => s.roles["Main session"] ? s.roles["Main session"].cost : 0);
  const tiles = [["Estimated cost", fmtCost(total.cost)], ["Tokens", fmtTok(total.tokens)], ["API turns", fmtInt(total.turns)],
    ["Sessions", fmtInt(ss.length)], ["Main session's share of cost", fmtShare(main, total.cost)]];
  $("tiles").innerHTML = tiles.map(t => `<div class="tile"><div class="k">${esc(t[0])}</div><div class="v">${esc(t[1])}</div></div>`).join("");
  $("subtitle").textContent = `${ss.length} session(s), generated ${fmtTime(DATA.generated)}. Costs are estimated at API list prices.`;
  const warnings = DATA.warnings.map(w => `<div class="warn">${esc(w.charAt(0).toUpperCase() + w.slice(1))}</div>`).join("");
  $("warnings").innerHTML = warnings;
}

function renderRoles() {
  const roles = allRoles();
  const total = sum(Object.values(roles), r => metricOf(r));
  const list = DATA.roles_order.filter(r => roles[r].turns > 0 || roles[r].runs > 0);
  bars("roles", list.map(role => {
    const t = roles[role];
    return {label: role, valueText: fmtMetric(metricOf(t)) + "  " + fmtShare(metricOf(t), total),
      segs: [{value: metricOf(t), slot: 1, tip: `${role}\n${state.metric === "cost" ? "Cost" : "Tokens"}: ${fmtMetric(metricOf(t))} (${fmtShare(metricOf(t), total)})\nCost ${fmtCost(t.cost)}, ${fmtInt(t.tokens)} tokens\n${fmtInt(t.turns)} turns in ${fmtInt(t.runs)} run(s)`}]};
  }));
  const totalCost = sum(Object.values(roles), r => r.cost);
  $("roles-table").innerHTML = table(["Role", "Runs", "Turns", "Fresh in", "Cache write", "Cache read", "Output", "Est. cost", "Share"],
    list.map(role => { const t = roles[role]; return [role, fmtInt(t.runs), fmtInt(t.turns), fmtInt(t.fresh), fmtInt(t.cache_write),
      fmtInt(t.cache_read), fmtInt(t.output), fmtCost(t.cost), fmtShare(t.cost, totalCost)]; }));
}

function renderSplit() {
  legend("split-legend", ["Main session", "Agents"]);
  const ss = DATA.sessions;
  const rows = ss.map(s => {
    const main = s.roles["Main session"] || emptyTotals();
    const agentsValue = metricOf(s.total) - metricOf(main);
    return {label: sessionName(s), valueText: fmtMetric(metricOf(s.total)), segs: [
      {value: metricOf(main), slot: 1, tip: `${s.label}\nMain session: ${fmtMetric(metricOf(main))} (${fmtShare(metricOf(main), metricOf(s.total))})`},
      {value: agentsValue, slot: 2, tip: `${s.label}\nAgents: ${fmtMetric(agentsValue)} (${fmtShare(agentsValue, metricOf(s.total))})`}]};
  });
  bars("split", rows);
  $("split-table").innerHTML = table(["Session", "Main cost", "Agents cost", "Main share", "Main tokens", "Agent tokens"],
    ss.map(s => { const m = s.roles["Main session"] || emptyTotals(); return [sessionName(s), fmtCost(m.cost), fmtCost(s.total.cost - m.cost),
      fmtShare(m.cost, s.total.cost), fmtInt(m.tokens), fmtInt(s.total.tokens - m.tokens)]; }));
}

function renderKinds() {
  legend("kinds-legend", KINDS.map(k => k[1]));
  const total = DATA.sessions.reduce((a, s) => addTotals(a, s.total), emptyTotals());
  const row = (label, field, fmt, totalValue) => ({label, valueText: fmt(totalValue), segs: KINDS.map((k, i) => {
    const value = total[field(k)];
    return {value, slot: i + 1, tip: `${k[1]}\n${label}: ${fmt(value)} (${fmtShare(value, totalValue)})`};
  })});
  // Each bar is scaled to its own total, so both show shares and are as long as the track.
  bars("kinds", [row("Tokens", k => k[0], fmtTok, total.tokens), row("Cost", k => k[2], fmtCost, total.cost)].map(r => {
    const t = sum(r.segs, s => s.value);
    r.segs.forEach(s => { s.value = t > 0 ? s.value / t : 0; });
    return r;
  }), 1);
  $("kinds-table").innerHTML = table(["Kind", "Tokens", "Token share", "Cost", "Cost share"],
    KINDS.map(k => [k[1], fmtInt(total[k[0]]), fmtShare(total[k[0]], total.tokens), fmtCost(total[k[2]]), fmtShare(total[k[2]], total.cost)]));
}

function renderCompare() {
  const names = state.by === "group" ? GROUPS : KINDS.map(k => k[1]);
  legend("compare-legend", names);
  const ss = DATA.sessions;
  const segsOf = s => {
    const g = groupTotals(s);
    return names.map((n, i) => state.by === "group" ? [n, metricOf(g[n])] :
      [n, state.metric === "cost" ? s.total[KINDS[i][2]] : s.total[KINDS[i][0]]]);
  };
  bars("compare", ss.map(s => ({label: sessionName(s), valueText: fmtMetric(metricOf(s.total)),
    segs: segsOf(s).map(([n, v], i) => ({value: v, slot: i + 1, tip: `${s.label}\n${n}: ${fmtMetric(v)} (${fmtShare(v, metricOf(s.total))})`}))})));
  $("compare-table").innerHTML = table(["Session", ...names, "Total"],
    ss.map(s => [sessionName(s), ...segsOf(s).map(x => fmtMetric(x[1])), fmtMetric(metricOf(s.total))]));
}

function renderPicker() {
  const pick = $("pick");
  pick.innerHTML = DATA.sessions.map((s, i) => `<option value="${i}">${esc(sessionName(s))} (${fmtCost(s.total.cost)})</option>`).join("");
  pick.value = String(state.session);
}

// Idle time longer than this is drawn as a short break; main-session API calls closer than this make one block.
const IDLE_GAP_SECONDS = 600, MAIN_CLUSTER_SECONDS = 120, GAP_WIDTH = 30, MIN_RUN_WIDTH = 6;

function mainClusters(times) {
  const clusters = [];
  for (const t of times) {
    const last = clusters[clusters.length - 1];
    if (last && t - last.end <= MAIN_CLUSTER_SECONDS) { last.end = t; last.calls++; }
    else clusters.push({start: t, end: t, calls: 1});
  }
  return clusters;
}

// The stretches where something is active, as [start, end] pairs, with gaps up to IDLE_GAP_SECONDS closed.
function activeSegments(intervals) {
  const sorted = intervals.map(i => [i.start, i.end]).sort((a, b) => a[0] - b[0]);
  const segments = [];
  for (const [start, end] of sorted) {
    const last = segments[segments.length - 1];
    if (last && start - last[1] <= IDLE_GAP_SECONDS) last[1] = Math.max(last[1], end);
    else segments.push([start, end]);
  }
  return segments;
}

function renderTimeline() {
  const s = DATA.sessions[state.session];
  const el = $("timeline");
  if (!s) { el.innerHTML = ""; return; }
  const design = s.role_runs["Design review"] || 0, code = s.role_runs["Code review"] || 0, art = s.role_runs["Art review"] || 0;
  $("rounds").innerHTML = `<span class="stat">Design review runs: <b>${design}</b></span><span class="stat">Code review runs: <b>${code}</b></span>` +
    `<span class="stat">Art review runs: <b>${art}</b></span><span class="stat">Subagents: <b>${s.agent_count}</b></span>`;
  legend("tl-legend", GROUPS);
  // rows: the main session's activity blocks, then one row per agent in start order
  const rows = [];
  const clusters = mainClusters(s.main_calls || []);
  if (clusters.length) rows.push({label: "Main session", runs: clusters.map(c => ({start: c.start, end: c.end, role: "Main session",
    continuation: false, main: true, calls: c.calls}))});
  const byAgent = {};
  for (const r of s.runs) (byAgent[r.agent] = byAgent[r.agent] || []).push(r);
  const labelCount = {};
  const firstStart = a => Math.min(...byAgent[a].map(r => r.start == null ? Infinity : r.start));
  Object.keys(byAgent).map(Number).sort((a, b) => firstStart(a) - firstStart(b)).forEach(a => {
    const runs = byAgent[a].filter(r => r.start != null);
    if (!runs.length) return;
    const first = runs[0].role;
    labelCount[first] = (labelCount[first] || 0) + 1;
    rows.push({label: first + (labelCount[first] > 1 ? " " + labelCount[first] : ""), runs});
  });
  const all = rows.flatMap(r => r.runs);
  if (!all.length) { el.innerHTML = '<p class="note">No timed API calls in this session.</p>'; $("timeline-table").innerHTML = ""; return; }

  const segments = activeSegments(all);
  const W = Math.max(300, el.clientWidth), labelW = W < 480 ? 104 : 120, pad = 14, rowH = 24, top = 8, axisH = 20;
  const plotLeft = labelW + 6, plotWidth = W - plotLeft - pad;
  const gapW = segments.length > 1 ? Math.min(GAP_WIDTH, plotWidth * 0.3 / (segments.length - 1)) : 0;
  const spanWidth = plotWidth - gapW * (segments.length - 1);
  const activeTotal = sum(segments, g => Math.max(g[1] - g[0], 1));
  // Each segment gets a share of the width by its duration; xs[i] is where segment i starts.
  const xs = [];
  let cursor = plotLeft;
  segments.forEach(g => { xs.push(cursor); cursor += Math.max(g[1] - g[0], 1) / activeTotal * spanWidth + gapW; });
  const segmentWidth = i => Math.max(segments[i][1] - segments[i][0], 1) / activeTotal * spanWidth;
  const x = t => {
    let i = segments.findIndex(g => t <= g[1]);
    if (i < 0) i = segments.length - 1;
    const g = segments[i];
    return xs[i] + Math.min(Math.max(t - g[0], 0), Math.max(g[1] - g[0], 1)) / Math.max(g[1] - g[0], 1) * segmentWidth(i);
  };
  const H = top + rows.length * rowH + axisH;
  let svg = `<svg width="${W}" height="${H}" viewBox="0 0 ${W} ${H}" role="img" aria-label="Timeline of agent runs">`;

  // Time ticks inside each active segment, and a break mark for each compressed gap.
  let lastLabelEnd = -1e9, lastDay = null;
  segments.forEach((g, i) => {
    const sw = segmentWidth(i), count = Math.max(1, Math.floor(sw / 110));
    for (let k = 0; k <= count; k++) {
      const t = g[0] + (g[1] - g[0]) * k / count, tx = xs[i] + sw * k / count;
      svg += `<line x1="${tx}" x2="${tx}" y1="${top}" y2="${H - axisH}" stroke="var(--grid)" stroke-width="1"/>`;
      const day = fmtTime(t).slice(5, 10);
      const text = (lastDay === null || day !== lastDay) ? fmtTime(t).slice(5) : fmtClock(t);
      const width = text.length * 6, anchor = k === 0 ? "start" : k === count ? "end" : "middle";
      const left = anchor === "start" ? tx : anchor === "end" ? tx - width : tx - width / 2;
      if (left > lastLabelEnd + 4 && left + width <= W) {
        svg += `<text x="${tx}" y="${H - 5}" text-anchor="${anchor}">${text}</text>`;
        lastLabelEnd = left + width; lastDay = day;
      }
    }
    if (i < segments.length - 1) {
      const gx = xs[i] + sw, gapLength = segments[i + 1][0] - g[1], mid = gx + gapW / 2;
      svg += `<rect x="${gx}" y="${top}" width="${gapW}" height="${H - axisH - top}" fill="var(--grid)" opacity="0.6" data-tip="${esc("Idle gap, drawn short: " + fmtDuration(gapLength) + "\n" + fmtTime(g[1]) + " to " + fmtTime(segments[i + 1][0]))}"/>`;
      const zig = [];
      for (let y = top; y <= H - axisH; y += 8) zig.push(`${mid + ((y - top) / 8 % 2 ? 4 : -4)},${y}`);
      svg += `<polyline points="${zig.join(" ")}" fill="none" stroke="var(--muted)" stroke-width="1.2" stroke-dasharray="3 2" pointer-events="none"/>`;
      svg += `<text x="${mid}" y="${H - 5}" text-anchor="middle" pointer-events="none" style="font-size:9px">${fmtDuration(gapLength).replace(" ", "")}</text>`;
    }
  });

  rows.forEach((row, i) => {
    const y = top + i * rowH;
    svg += `<text class="lab-main" x="${labelW - 8}" y="${y + rowH / 2 + 4}" text-anchor="end">${esc(row.label)}</text>`;
    for (const r of row.runs) {
      const group = GROUPS.indexOf(GROUP_OF[r.role] || "Other") + 1;
      const x0 = x(r.start), w = Math.max(MIN_RUN_WIDTH, x(r.end) - x0);
      const tip = r.main
        ? `Main session activity\n${fmtTime(r.start)} to ${fmtClock(r.end)} (${fmtDuration(r.end - r.start)})\n${fmtInt(r.calls)} API calls`
        : `${row.label}${r.continuation ? " (continued by SendMessage)" : ""}\nRole: ${r.role}\n${fmtTime(r.start)} to ${fmtClock(r.end)} (${fmtDuration(r.end - r.start)})\n` +
          `${fmtInt(r.totals.turns)} turns, ${fmtInt(r.totals.tokens)} tokens\nCost ${fmtCost(r.totals.cost)}`;
      svg += `<rect class="tl-run" x="${x0}" y="${y + 6}" width="${w}" height="${rowH - 12}" rx="2" fill="var(--c${group})" data-tip="${esc(tip)}"/>`;
      if (!r.main) {
        svg += `<circle cx="${x0}" cy="${y + rowH / 2}" r="4" fill="${r.continuation ? "var(--text)" : "var(--panel)"}" stroke="var(--text)" stroke-width="1.5" data-tip="${esc(r.continuation ? "Hand-off: continued with SendMessage at " + fmtClock(r.start) : "Hand-off: started at " + fmtClock(r.start))}"/>`;
      }
    }
  });
  el.innerHTML = svg + "</svg>";
  const tableRows = rows.flatMap(row => row.runs.map(r => r.main
    ? [row.label, r.role, "activity", fmtTime(r.start), fmtClock(r.end), fmtDuration(r.end - r.start), fmtInt(r.calls), "-", "-"]
    : [row.label, r.role, r.continuation ? "continuation" : "start", fmtTime(r.start), fmtClock(r.end), fmtDuration(r.end - r.start),
      fmtInt(r.totals.turns), fmtInt(r.totals.tokens), fmtCost(r.totals.cost)]));
  $("timeline-table").innerHTML = table(["Row", "Role", "Kind", "First call", "Last call", "Span", "Turns", "Tokens", "Est. cost"], tableRows);
}

function renderAll() {
  $("m-cost").setAttribute("aria-pressed", state.metric === "cost");
  $("m-tokens").setAttribute("aria-pressed", state.metric === "tokens");
  $("by-group").setAttribute("aria-pressed", state.by === "group");
  $("by-kind").setAttribute("aria-pressed", state.by === "kind");
  renderRoles(); renderSplit(); renderCompare();
}
function renderStatic() { renderTiles(); renderKinds(); renderPicker(); renderTimeline(); renderAll(); }

$("m-cost").onclick = () => { state.metric = "cost"; renderAll(); };
$("m-tokens").onclick = () => { state.metric = "tokens"; renderAll(); };
$("by-group").onclick = () => { state.by = "group"; renderAll(); };
$("by-kind").onclick = () => { state.by = "kind"; renderAll(); };
$("pick").onchange = e => { state.session = Number(e.target.value); renderTimeline(); };

function effectiveTheme() {
  const set = document.documentElement.getAttribute("data-theme");
  return set || (matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light");
}
function setTheme(theme) {
  document.documentElement.setAttribute("data-theme", theme);
  try { localStorage.setItem("token-dashboard-theme", theme); } catch (e) { /* storage may be blocked */ }
}
$("theme").onclick = () => setTheme(effectiveTheme() === "dark" ? "light" : "dark");
(function initTheme() {
  let theme = null;
  try { theme = localStorage.getItem("token-dashboard-theme"); } catch (e) { /* storage may be blocked */ }
  if (location.hash === "#dark" || location.hash === "#light") theme = location.hash.slice(1);
  if (theme === "dark" || theme === "light") document.documentElement.setAttribute("data-theme", theme);
})();

const tip = $("tip");
function placeTip(e) {
  const m = 12, w = tip.offsetWidth, h = tip.offsetHeight;
  let left = e.clientX + m, topPos = e.clientY + m;
  if (left + w > innerWidth - 4) left = Math.max(4, e.clientX - w - m);
  if (topPos + h > innerHeight - 4) topPos = Math.max(4, e.clientY - h - m);
  tip.style.left = left + "px"; tip.style.top = topPos + "px";
}
function showTip(e) {
  const target = e.target.closest ? e.target.closest("[data-tip]") : null;
  if (!target) { tip.style.display = "none"; return; }
  tip.textContent = target.getAttribute("data-tip");
  tip.style.display = "block";
  placeTip(e);
}
document.addEventListener("mouseover", showTip);
document.addEventListener("mousemove", showTip);
document.addEventListener("mouseout", e => { if (!e.relatedTarget) tip.style.display = "none"; });
document.addEventListener("click", showTip);

let resizeTimer = null;
addEventListener("resize", () => { clearTimeout(resizeTimer); resizeTimer = setTimeout(renderTimeline, 100); });
renderStatic();
</script>
</body>
</html>
'''


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
        shown = [measure_session(path) for _, path in sessions[-arguments.last:]]
        print_session_list(shown)
    else:
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
        shown = [measure_session(chosen[0])]
        print_session_table(shown[0])

    if arguments.html:
        write_dashboard(shown, arguments.html)
        print(f"Dashboard written: {arguments.html}")


if __name__ == "__main__":
    main()
