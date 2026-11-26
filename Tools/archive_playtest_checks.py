# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Moves play-test headings whose checks have all passed from Docs/PlayTest.md to Docs/PlayTestArchive.md.

Run it after ticking checks in PlayTest.md (Python 3.9 or later, standard library only; the
engine's bundled Python works too); it finds the documents from its own folder:
    python Tools/archive_playtest_checks.py             moves them and lists what moved
    python Tools/archive_playtest_checks.py --dry-run   only lists what would move

A heading is a "### " heading in section 2 ("## 2. Pending checks"), with everything under it up to
the next heading. It moves when it has at least one check ("- [x] ") and no open one ("- [ ] ", at
any indent), so a heading with only notes, such as "Editor chores", stays. Moved headings go to the
end of PlayTestArchive.md in the order they stood, unchanged except as section 2 says: a link to one
of PlayTest.md's own headings ("](#3-smoke-test)") gets "PlayTest.md" in front. Line endings are
kept as each file has them. It exits with 1, writing nothing, if a file or section 2 is missing.
"""

import argparse
import re
import sys
from pathlib import Path

DOCS = Path(__file__).resolve().parent.parent / "Docs"
PLAYTEST = DOCS / "PlayTest.md"
ARCHIVE = DOCS / "PlayTestArchive.md"
PENDING_SECTION = "## 2. Pending checks"
PASSED_CHECK = re.compile(r"^\s*- \[[xX]\] ")
OPEN_CHECK = re.compile(r"^\s*- \[ \] ")
OWN_ANCHOR_LINK = "](#"
PLAYTEST_ANCHOR_LINK = "](PlayTest.md#"


def read_lines(path):
    with path.open(encoding="utf-8", newline="") as file:
        text = file.read()
    newline = "\r\n" if "\r\n" in text else "\n"
    return text.replace("\r\n", "\n").split("\n"), newline


def write_lines(path, lines, newline):
    path.write_text(newline.join(lines), encoding="utf-8", newline="")


def is_heading(line):
    return line.startswith("## ") or line.startswith("### ")


def is_blank(line):
    return line.strip() == ""


def find_pending_section(lines):
    """Returns (start, end): the line after section 2's heading and the next "## " heading."""
    start = lines.index(PENDING_SECTION) + 1
    end = next((index for index in range(start, len(lines)) if lines[index].startswith("## ")), len(lines))
    return start, end


def is_passed(block):
    return any(PASSED_CHECK.match(line) for line in block) and not any(OPEN_CHECK.match(line) for line in block)


def take_passed_headings(lines):
    """Returns PlayTest.md without its passed headings, and the headings as lists of lines without
    their trailing blank lines."""
    start, end = find_pending_section(lines)
    kept = lines[:start]
    moved = []
    index = start
    while index < end:
        if not lines[index].startswith("### "):
            kept.append(lines[index])
            index += 1
            continue
        block_end = next((later for later in range(index + 1, end) if is_heading(lines[later])), end)
        block = lines[index:block_end]
        if is_passed(block):
            while block and is_blank(block[-1]):
                block.pop()
            moved.append([line.replace(OWN_ANCHOR_LINK, PLAYTEST_ANCHOR_LINK) for line in block])
        else:
            kept.extend(block)
        index = block_end
    kept.extend(lines[end:])
    return kept, moved


def append_headings(lines, moved):
    trailing = 0
    while lines and is_blank(lines[-1]):
        lines.pop()
        trailing += 1
    for block in moved:
        lines.extend(["", *block])
    lines.extend([""] * max(trailing, 1))


def main():
    # Check texts can hold any character; don't let a narrow console encoding stop the run.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="replace")
    parser = argparse.ArgumentParser(description=__doc__.strip().split("\n")[0])
    parser.add_argument("--dry-run", action="store_true", help="only list what would move")
    args = parser.parse_args()

    for path in (PLAYTEST, ARCHIVE):
        if not path.is_file():
            print(f"{path} not found.", file=sys.stderr)
            return 1

    playtest, playtest_newline = read_lines(PLAYTEST)
    archive, archive_newline = read_lines(ARCHIVE)
    if PENDING_SECTION not in playtest:
        print(f'{PLAYTEST} has no "{PENDING_SECTION}" heading.', file=sys.stderr)
        return 1
    kept, moved = take_passed_headings(playtest)
    if not moved:
        print("No heading in PlayTest.md has all its checks passed.")
        return 0

    for block in moved:
        print(block[0][4:])
    append_headings(archive, moved)
    verb = "Would move" if args.dry_run else "Moved"
    print(f"{verb} {len(moved)} heading(s) to {ARCHIVE}.")
    if not args.dry_run:
        write_lines(PLAYTEST, kept, playtest_newline)
        write_lines(ARCHIVE, archive, archive_newline)
    return 0


if __name__ == "__main__":
    sys.exit(main())
