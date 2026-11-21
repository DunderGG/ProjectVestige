# EditorScripts

Python scripts that run **inside the Unreal Editor**, through Unreal's Python Editor Script Plugin, to help with authoring work such as surveying a level. They are not game code: nothing here is built, cooked or packaged, and they run only when a developer starts them. Scripts that run from a terminal outside the Editor go in [Tools/](../Tools/README.md) instead. The rules for Editor scripts are in [Systems.md, section 23](../Docs/Systems/23-EditorScripting.md).

## Running a script

1. Check that **Edit > Plugins > Scripting > Python Editor Script Plugin** is enabled; it is off by default, and enabling it needs an Editor restart.
2. Open the map you want to work on, and leave Play In Editor stopped.
3. Choose **Tools > Execute Python Script** and select the script. Its result is printed to the Output Log (**Window > Output Log**).

| File | What it does |
|---|---|
| [SurveyLevel.py](SurveyLevel.py) | Reads the currently open Editor map, without changing it, and writes a Markdown report, a versioned JSON report and a completion manifest to `Saved/LevelReports/<map>_<hash>/`. |
| [Tests/test_survey_level.py](Tests/test_survey_level.py) | Tests for `SurveyLevel.py`, run outside the Editor with a stand-in for the `unreal` module. |

## Running the tests

The tests don't need the Editor. From the project folder, with Python 3:

```powershell
python -B -m unittest discover -s EditorScripts/Tests -v
```

`-B` keeps Python from writing `__pycache__` folders into the repository.

## Adding a script

- A new script goes here when it imports the `unreal` module; a script that runs from a terminal goes in `Tools/`.
- Follow [Systems.md, section 23](../Docs/Systems/23-EditorScripting.md): start the file with the copyright and SPDX header from [LICENSING.md](../LICENSING.md), keep it read-only unless its design says otherwise, and write its output under the ignored `Saved/` folder.
- Put its tests in `Tests/` as `test_<script>.py`, using a stand-in `unreal` module so they run without the Editor, and add its line to the table above.
- Scripts stay out of `Content/Python/`: the Editor adds that folder to Python's path and runs any `init_unreal.py` in it at startup, while these scripts are meant to run only when someone starts them.
