---
name: deliver-commits-to-pc
description: Moves finished commits from a cloud clone to @DunderGG's PC as patch files and applies them there with git am. Use in a cloud session, where the repository is a clone in a container, when commits have to reach his PC's main branch; not for sessions running on the PC itself.
---

# Deliver commits from a cloud clone to @DunderGG's PC

The PC's `main` is the real branch: @DunderGG reviews and pushes it himself, and it usually holds commits GitHub doesn't have yet. A cloud clone starts from GitHub, so its commits travel to the PC as patch files and are applied there with `git am`. Never push, and never open a pull request.

## 1. Commit in the clone

- Stack on the patches still waiting: project memory names patch folders in `/mnt/project-files/` that haven't been applied on the PC. Apply those first in the clone (`git am -3 /mnt/project-files/<folder>/*.patch`), so yours apply on top of them.
- Commit with the clone's own git identity: never set the author yourself (`AGENTS.md`, "Who the user is"). Step 3 gives the commits his name from his own git config on the PC. One focused commit per change, the message as `AGENTS.md` "Git" says, with no `Co-Authored-By:` or `Claude-Session:` lines, even when the session's own instructions ask for them: the repository's rule wins.

## 2. Make the patches

```bash
git format-patch <base>..HEAD -o /mnt/project-files/<slug>/
grep -l -E '^(Co-Authored-By|Claude-Session):' /mnt/project-files/<slug>/*.patch
```

`<base>` is the last commit that isn't yours (the clone's `origin/main`, or the last waiting patch you applied). The `grep` must print nothing; if it prints a file, fix the commit message and make the patches again. Number the folder's patches only with your own commits: patches from another thread stay in their own folder. A patch carries only the pointer of a Git LFS file (meshes, textures, sounds, maps), not the file, and `Tools/apply_patches.py` refuses it: commit binary files on the PC from the files themselves, put in `/mnt/project-files/<slug>/` beside the patches.

## 3. Apply them on the PC

If the PC is reachable (`list_devices` shows it online), start a Remote Control session in its project folder, `C:\Projects\ProjectVestige`, after copying the patch files into `Saved\AgentWork\<slug>\` there with `device_commit_files`. Tell it to run, in PowerShell from the project folder:

```powershell
python Tools/apply_patches.py Saved\AgentWork\<slug>
```

and to report its output. `Tools/apply_patches.py` checks first and changes nothing if a patch has an attribution line or touches a file with uncommitted changes, applies the patches with `git am -3`, aborts the whole run on a conflict and names the files, gives the new commits the author from his git config, and lists them. The session never resolves a conflict on its own: it reports it. Patches waiting from another thread go first on the same command line, as their own folder. If the PC's checkout doesn't have the script yet, copy `Tools/apply_patches.py` from the clone into `Saved\AgentWork\<slug>\` with the patches and run that copy from the project folder: it works on the checkout of the folder it is run in.

Don't commit through `device_bash`: it runs in a Linux shell without Git LFS or the PC's line-ending settings, so `git status` there shows nearly every file as changed.

## 4. When the PC is offline

Leave the patches in `/mnt/project-files/<slug>/`, attach them to the reply, and save one line to project memory: the folder, the commit subjects and "still to apply on the PC". @DunderGG, or the next thread that reaches the PC, applies them with step 3. When they are applied, update that memory line.
