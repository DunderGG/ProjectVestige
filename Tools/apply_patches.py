# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Applies patch files made in a cloud clone to this checkout's current branch, as local commits.

Run it from the project folder (Python 3.9 or later, standard library only; the engine's bundled
Python works too): it works on the checkout of the current folder, so a copy of the script kept
outside the repository works too, before the patch that adds it is applied:
    python Tools/apply_patches.py <folder> [<folder or .patch file>...]
    python Tools/apply_patches.py --check <folder>     only runs the checks below, changes nothing

A folder stands for its *.patch files sorted by name; folders and files are applied in the order
given, so pass a folder of waiting patches before the folder built on top of it. Before applying it
stops, changing nothing, when the branch is detached, an earlier git am or rebase is unfinished, a
patch has a Co-Authored-By: or Claude-Session: line, or a file a patch touches has uncommitted
changes. It then runs git am -3. On a conflict it aborts the whole git am, so the branch is as it
was, and names the conflicting files. When the new commits' author isn't the one in this checkout's
git config, it re-authors only those commits, which nobody has pushed, by rebuilding them on the same
trees (git commit-tree), so uncommitted work elsewhere in the checkout is never touched; then it
lists them. A patch that adds a Git LFS pointer is refused: the file itself can't travel in a patch.
It never pushes.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

REPOSITORY = Path.cwd()
ATTRIBUTION_LINE = re.compile(r"^(Co-Authored-By|Claude-Session):", re.IGNORECASE | re.MULTILINE)
LFS_POINTER_LINE = re.compile(r"^\+version https://git-lfs\.github\.com/spec/", re.MULTILINE)


class StopApplying(Exception):
    pass


def run_git(arguments, input_bytes=None):
    return subprocess.run(["git", "-C", str(REPOSITORY), *arguments], input=input_bytes, capture_output=True)


def git(*arguments, check=True, input_bytes=None):
    """Runs git and returns its output as text, decoded as UTF-8 whatever the console's code page."""
    result = run_git(arguments, input_bytes)
    stdout = result.stdout.decode("utf-8", errors="replace")
    stderr = result.stderr.decode("utf-8", errors="replace")
    if check and result.returncode != 0:
        raise StopApplying(f"git {' '.join(arguments)} failed:\n{stderr.strip()}")
    return result.returncode, stdout, stderr


def git_out(*arguments):
    return git(*arguments)[1].strip()


def collect_patches(sources):
    patches = []
    for source in sources:
        path = Path(source).resolve()
        if path.is_dir():
            found = sorted(path.glob("*.patch"))
            if not found:
                raise StopApplying(f"{path} has no .patch files.")
            patches.extend(found)
        elif path.is_file():
            patches.append(path)
        else:
            raise StopApplying(f"{path} not found.")
    return patches


def check_repository():
    code, branch, _ = git("symbolic-ref", "--quiet", "--short", "HEAD", check=False)
    if code != 0:
        raise StopApplying("HEAD is detached; check out the branch to apply to first.")
    git_dir = Path(git_out("rev-parse", "--absolute-git-dir"))
    for unfinished in ("rebase-apply", "rebase-merge"):
        if (git_dir / unfinished).exists():
            raise StopApplying(f"An earlier git am or rebase is unfinished ({unfinished}); finish or abort it first.")
    return branch.strip()


def patched_paths(patch):
    """Returns every path the patch touches, as git apply reads it (spaces and quoting included)."""
    fields = git("apply", "--numstat", "-z", str(patch))[1].split("\0")
    paths = set()
    index = 0
    while index < len(fields):
        field = fields[index]
        if not field:
            index += 1
            continue
        counts_and_path = field.split("\t", 2)
        if len(counts_and_path) == 3 and counts_and_path[2]:
            paths.add(counts_and_path[2])
            index += 1
        else:
            paths.update(fields[index + 1:index + 3])
            index += 3
    return paths


def changed_paths():
    """Returns every path with uncommitted changes, untracked files included."""
    fields = git("status", "--porcelain", "-z", "--untracked-files=all")[1].split("\0")
    paths = set()
    index = 0
    while index < len(fields):
        entry = fields[index]
        index += 1
        if len(entry) < 4:
            continue
        paths.add(entry[3:])
        if entry[0] in "RC":
            paths.add(fields[index])
            index += 1
    return paths


def check_patches(patches):
    touched = set()
    for patch in patches:
        text = patch.read_text(encoding="utf-8", errors="replace")
        if ATTRIBUTION_LINE.search(text):
            raise StopApplying(f"{patch.name} has a tool attribution line; make the patch again without it.")
        if LFS_POINTER_LINE.search(text):
            raise StopApplying(
                f"{patch.name} adds a Git LFS pointer: the file itself isn't in the patch. "
                "Bring binary files over as files, not as patches."
            )
        touched |= patched_paths(patch)
    overlap = sorted(touched & changed_paths())
    if overlap:
        raise StopApplying("These files have uncommitted changes that the patches touch:\n  " + "\n  ".join(overlap))


def without_hints(text):
    return [line for line in text.strip().splitlines() if line and not line.startswith("hint:") and "--continue" not in line and "--skip" not in line]


def apply(patches, start):
    code, stdout, stderr = git("am", "-3", *[str(patch) for patch in patches], check=False)
    if code == 0:
        return
    conflicts = git("diff", "--name-only", "--diff-filter=U", check=False)[1].split("\n")
    conflicts = [path for path in conflicts if path]
    git("am", "--abort", check=False)
    head = git_out("rev-parse", "HEAD")
    where = f"the branch is back at {start[:7]}. Nothing was applied" if head == start else f"the branch is at {head[:7]}, not {start[:7]}: check it by hand"
    if conflicts:
        details = "Conflicts in:\n  " + "\n  ".join(conflicts)
    else:
        details = "git am said:\n  " + "\n  ".join((without_hints(stdout) + without_hints(stderr))[-6:])
    raise StopApplying(f"git am stopped and was aborted; {where}. {details}")


def reset_authors(start):
    """Gives the commits after start the author in this checkout's git config, by rebuilding them on
    the same trees with git commit-tree, so the working tree and the index are never touched."""
    name = git("config", "user.name", check=False)[1].strip()
    email = git("config", "user.email", check=False)[1].strip()
    if not (name and email):
        print("No user.name or user.email in this checkout's git config: the authors were left as the patches had them.")
        return
    commits = git_out("rev-list", "--reverse", f"{start}..HEAD").split()
    authors = git_out("log", "--format=%an <%ae>", f"{start}..HEAD").splitlines()
    if all(author == f"{name} <{email}>" for author in authors):
        return
    head = git_out("rev-parse", "HEAD")
    parent = start
    for commit in commits:
        message = run_git(["log", "-1", "--format=%B", commit]).stdout.rstrip(b"\n") + b"\n"
        tree = git_out("rev-parse", f"{commit}^{{tree}}")
        parent = git("commit-tree", tree, "-p", parent, "-F", "-", input_bytes=message)[1].strip()
    git("update-ref", "-m", "apply_patches: reset author", "HEAD", parent, head)


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="replace")
    parser = argparse.ArgumentParser(description=__doc__.strip().split("\n")[0])
    parser.add_argument("sources", nargs="+", help="folders of .patch files, or .patch files, in order")
    parser.add_argument("--check", action="store_true", help="only run the checks, change nothing")
    args = parser.parse_args()

    try:
        patches = collect_patches(args.sources)
        branch = check_repository()
        check_patches(patches)
        if args.check:
            print(f"Checks passed: {len(patches)} patch(es) can be applied to {branch}.")
            return 0
        start = git_out("rev-parse", "HEAD")
        apply(patches, start)
        reset_authors(start)
    except StopApplying as stop:
        print(stop, file=sys.stderr)
        return 1

    print(f"Applied {len(patches)} patch(es) to {branch}:")
    print(git_out("log", "--reverse", "--format=%h %an <%ae> | %s", f"{start}..HEAD"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
