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
| [SetUpCropWithering.py](SetUpCropWithering.py) | Makes `MI_Crop_Withered` (an instance of `M_Crop`), sets it as `BP_FarmGrid`'s withered crop material, and adds the `Farm.Withered` and `Farm.ClearFirst` rows to `DA_FeedbackResponses` ([Systems.md, sections 15 and 22](../Docs/Systems.md)). Changes and saves those assets; a rerun writes only what differs and never overwrites a text edited by hand. Also runs headless (the command is in its docstring). |
| [SetUpWell.py](SetUpWell.py) | Adds the water container fragment (Capacity 8) to `DA_Item_WateringCan`, the five `Water.*` rows to `DA_FeedbackResponses`, makes `BP_Well` with `SM_Well` and places it in `L_Farm` ([Systems.md, section 24](../Docs/Systems/24-Water.md)). Saves those assets and the map; a rerun writes only what differs and never overwrites a text edited by hand. Also runs headless (the command is in its docstring). |
| [Tests/test_set_up_crop_withering.py](Tests/test_set_up_crop_withering.py) | Tests for `SetUpCropWithering.py`'s row planning, with a stand-in `unreal` module. |

## Running the tests

The tests don't need the Editor. From the project folder, with Python 3:

```powershell
python -B -m unittest discover -s EditorScripts/Tests -v
```

`-B` keeps Python from writing `__pycache__` folders into the repository.

## Adding a script

- A new script goes here when it imports the `unreal` module; a script that runs from a terminal goes in `Tools/`.
- Follow [Systems.md, section 23](../Docs/Systems/23-EditorScripting.md): start the file with the copyright and SPDX header from [LICENSING.md](../LICENSING.md) and check that the path table there covers it, keep it read-only unless its design says otherwise, and write its output under the ignored `Saved/` folder.
- Put its tests in `Tests/` as `test_<script>.py`, using a stand-in `unreal` module so they run without the Editor, and add its line to the table above.
- Scripts stay out of `Content/Python/`: the Editor adds that folder to Python's path and runs any `init_unreal.py` in it at startup, while these scripts are meant to run only when someone starts them.
