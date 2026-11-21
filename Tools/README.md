# Tools

Scripts that run from a terminal on the developer's PC, **outside the Unreal Editor**: building, testing and checking the project and its workflow. They never edit code or assets. Scripts that run inside the Editor go in [EditorScripts/](../EditorScripts/README.md) instead.

| File | What it does | How to run it |
|---|---|---|
| [Verify-Vestige.ps1](Verify-Vestige.ps1) | Builds the `VestigeEditor` target, runs the `Vestige` automation tests and prints a short summary ending in a `VERIFY:` line; the full logs go to `Saved/AgentWork/<Task>/`. `-BuildOnly` skips the tests, `-Filter Vestige.<Area>` runs one area. | From PowerShell, with the Unreal Editor closed: `& "C:\Unreal Projects\Vestige\Tools\Verify-Vestige.ps1" -Task <name>` |
| [measure_agent_tokens.py](measure_agent_tokens.py) | Reads this project's Claude Code transcripts and prints how many tokens each session and each agent role used, with an estimated cost. Prints numbers only, never transcript text. `--html [PATH]` also writes one self-contained HTML dashboard (charts per role, main session against agents, token kinds, task comparison and a run timeline, light and dark, with tables) to `Saved/TokenDashboard/token-dashboard.html`. | From the project folder, with Python 3.9 or later (standard library only): `python Tools/measure_agent_tokens.py`, `--session latest` for one session, `--last 10 --html` for the dashboard |

Each script's header comment has its full options.

## Adding a script

- A new script goes here when it runs from a terminal and doesn't need the Editor; one that uses the `unreal` Python module goes in `EditorScripts/`.
- Start it with the copyright and SPDX header from [LICENSING.md](../LICENSING.md) (plain `GPL-3.0-or-later`, since these never link with Unreal Engine), and check that the path table there covers it.
- Put a usage comment at the top, add its line to the table above, and write its output under the ignored `Saved/` folder.
