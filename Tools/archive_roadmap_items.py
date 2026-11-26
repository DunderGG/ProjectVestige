# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Moves ticked items from Docs/Roadmap.md to Docs/RoadmapArchive.md, under the same headings.

Run it after ticking the finished items in Roadmap.md (Python 3.9 or later, standard library only;
the engine's bundled Python works too); it finds the documents from its own folder:
    python Tools/archive_roadmap_items.py             moves them and lists what moved
    python Tools/archive_roadmap_items.py --dry-run   only lists what would move

A ticked item is a top-level list line starting "- [x] " together with the indented lines under it
(its sub-points). A ticked sub-point inside an open item stays where it is, with its parent. Each
item goes to the end of the list under the same "## " heading, and "### " heading if it has one, in
RoadmapArchive.md. A heading the archive doesn't have yet is added there in the order the headings
stand in Roadmap.md. Roadmap.md keeps every heading, so links to its sections keep working.

The one change to a moved line: a link to one of Roadmap.md's own headings ("](#42-farming-depth)")
gets "Roadmap.md" in front, so it still points at the roadmap's section, as the archive's first
paragraph says. Line endings are kept as each file has them. It exits with 1, writing nothing, if a
file is missing.
"""

import argparse
import re
import sys
from pathlib import Path

DOCS = Path(__file__).resolve().parent.parent / "Docs"
ROADMAP = DOCS / "Roadmap.md"
ARCHIVE = DOCS / "RoadmapArchive.md"
TICKED_ITEM = re.compile(r"^- \[[xX]\] ")
OWN_ANCHOR_LINK = "](#"
ROADMAP_ANCHOR_LINK = "](Roadmap.md#"


def read_lines(path):
    with path.open(encoding="utf-8", newline="") as file:
        text = file.read()
    newline = "\r\n" if "\r\n" in text else "\n"
    return text.replace("\r\n", "\n").split("\n"), newline


def write_lines(path, lines, newline):
    path.write_text(newline.join(lines), encoding="utf-8", newline="")


def heading_level(line):
    if line.startswith("### "):
        return 3
    if line.startswith("## "):
        return 2
    return 0


def is_continuation(line):
    return line[:1] in (" ", "\t") and line.strip() != ""


def is_blank(line):
    return line.strip() == ""


def take_ticked_items(lines):
    """Returns Roadmap.md without its ticked items, the items as (section, lines), and the order of
    every section in the file. A section is a (h2, h3) pair of heading lines; h3 is None for the
    part directly under an h2."""
    kept, items, order = [], [], []
    section = (None, None)
    index = 0
    while index < len(lines):
        line = lines[index]
        level = heading_level(line)
        if level == 2:
            section = (line, None)
            order.append(section)
        elif level == 3 and section[0] is not None:
            section = (section[0], line)
            order.append(section)
        if TICKED_ITEM.match(line) and section[0] is not None:
            end = index + 1
            while end < len(lines) and is_continuation(lines[end]):
                end += 1
            item = [line.replace(OWN_ANCHOR_LINK, ROADMAP_ANCHOR_LINK) for line in lines[index:end]]
            items.append((section, item))
            if kept and is_blank(kept[-1]) and end < len(lines) and is_blank(lines[end]):
                end += 1
            index = end
            continue
        kept.append(line)
        index += 1
    return kept, items, order


def find_section(lines, section):
    """Returns (start, end) of the section's body in the archive: start is the line after its
    heading, end the next heading or the end of the file. None if the archive lacks the section.
    The body of an h2 section ends at its first h3."""
    h2, h3 = section
    in_h2 = False
    start = None
    for index, line in enumerate(lines):
        level = heading_level(line)
        if start is not None and level:
            return start, index
        if level == 2:
            in_h2 = line == h2
        if start is None and in_h2:
            if (h3 is None and level == 2) or (h3 is not None and level == 3 and line == h3):
                start = index + 1
    return (start, len(lines)) if start is not None else None


def h2_end(lines, h2):
    """Returns the index of the h2 after this one, or the end of the file."""
    found = False
    for index, line in enumerate(lines):
        if heading_level(line) == 2:
            if found:
                return index
            found = line == h2
    return len(lines)


def heading_line(lines, section):
    found = find_section(lines, section)
    return None if found is None else found[0] - 1


def insert_heading(lines, section, order):
    """Adds a missing section's heading to the archive (its h2 first, when that is missing too),
    before the next section of Roadmap.md that the archive has, or at the end of its h2."""
    h2, h3 = section
    if h3 is not None and heading_line(lines, (h2, None)) is None:
        insert_heading(lines, (h2, None), order)
    heading = h3 if h3 is not None else h2
    later_sections = order[order.index(section) + 1:]
    if h3 is None:
        later_sections = [later for later in later_sections if later[1] is None]
    else:
        later_sections = [later for later in later_sections if later[0] == h2 and later[1] is not None]
    for later in later_sections:
        position = heading_line(lines, later)
        if position is not None:
            lines[position:position] = [heading, ""]
            return
    position = h2_end(lines, h2) if h3 is not None else len(lines)
    while position > 0 and is_blank(lines[position - 1]):
        position -= 1
    lines[position:position] = ["", heading]


def append_item(lines, section, item, order):
    if heading_line(lines, section) is None:
        insert_heading(lines, section, order)
    start, end = find_section(lines, section)
    content_end = end
    while content_end > start and is_blank(lines[content_end - 1]):
        content_end -= 1
    if content_end == start:
        lines[start:end] = ["", *item, ""]
        return
    last = lines[content_end - 1]
    if not (last.startswith("- ") or is_continuation(last)):
        item = ["", *item]
    lines[content_end:content_end] = item


def main():
    # Item texts can hold any character; don't let a narrow console encoding stop the run.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="replace")
    parser = argparse.ArgumentParser(description=__doc__.strip().split("\n")[0])
    parser.add_argument("--dry-run", action="store_true", help="only list what would move")
    args = parser.parse_args()

    for path in (ROADMAP, ARCHIVE):
        if not path.is_file():
            print(f"{path} not found.", file=sys.stderr)
            return 1

    roadmap, roadmap_newline = read_lines(ROADMAP)
    archive, archive_newline = read_lines(ARCHIVE)
    kept, items, order = take_ticked_items(roadmap)
    if not items:
        print("No ticked items in Roadmap.md.")
        return 0

    for section, item in items:
        heading = section[1] or section[0]
        print(f"{heading.lstrip('# ')}: {item[0][6:80]}")
        append_item(archive, section, item, order)

    verb = "Would move" if args.dry_run else "Moved"
    print(f"{verb} {len(items)} item(s) to {ARCHIVE}.")
    if not args.dry_run:
        write_lines(ROADMAP, kept, roadmap_newline)
        write_lines(ARCHIVE, archive, archive_newline)
    return 0


if __name__ == "__main__":
    sys.exit(main())
