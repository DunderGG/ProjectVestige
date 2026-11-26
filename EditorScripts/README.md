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
| [SetUpSeedShop.py](SetUpSeedShop.py) | Makes `DA_Shop_Farm` (`Base.FarmShop`: turnip, potato and cabbage seeds at 10, 20, 40), adds the two `Shop.*` rows to `DA_FeedbackResponses`, makes `BP_ShopDispenser` and its three children, and places them in `L_Farm` beside the shipping bin ([Systems.md, section 26](../Docs/Systems/26-Shops.md)). Until `SM_ShopDispenser` is imported the cabinets are the engine cube, scaled to size; a rerun after the import sets the real mesh. A rerun writes only what differs and never overwrites a text edited by hand. Also runs headless (the command is in its docstring). |
| [SetUpAudio.py](SetUpAudio.py) | Makes the sound classes `SCL_Overall` and `SCL_Effects`, imports `Art/Export/S_Water_FillCan.wav` as `S_Water_FillCan` and sets it as the sound of the `Water.CanFilled` and `Water.WellRanDry` rows of `DA_FeedbackResponses` ([Systems.md, section 14](../Docs/Systems/14-Audio.md)). Run `Art/Audio/make_water_fill_can.py` first. A rerun writes only what differs. Also runs headless (the command is in its docstring). |
| [SetUpHobDialogue.py](SetUpHobDialogue.py) | Makes or updates Hob's data and the dialogue box's assets headless: `IA_DialogueNext`, `IA_DialogueEnd` and `IMC_Dialogue`, `DA_Character_Hob` (`Base.Hob`), the six Hob conversations in `Content/Vestige/Dialogue` with their texts linked to `ST_Dialogue`, the `NothingToSay` row of `DA_FeedbackResponses`, and the defaults of `WBP_Dialogue` and `BP_HUD` ([Systems.md, sections 6, 10 and 11](../Docs/Systems.md)). Needs `ST_Dialogue` and the widget Blueprints first. A rerun rewrites the same values. Runs headless (the command is in its docstring). |
| [SetUpHobBody.py](SetUpHobBody.py) | Makes `M_HobEyes` (an emissive cyan that blinks), `MI_Hob_Cream` and `MI_Hob_Olive`, builds `BP_Hob` with a placeholder body from engine shapes (each part set by name on every run), and places one `BP_Hob` in `L_Farm` if the map has none ([Systems.md, section 10](../Docs/Systems/10-Robots.md)). Saves those assets and the map. Runs headless (the command is in its docstring). |
| [ImportProp.py](ImportProp.py) | Imports or reimports one prop mesh built on `Art/Blender/vestige_art/` (the name is its argument, such as `SM_ShippingBin`) from `Art/Export/` into `Content/Vestige/Meshes` as [Art/README.md, "Importing into Unreal"](../Art/README.md#importing-into-unreal) says: Nanite with a 100% fallback, `UCX_` collision only, each slot the `MI_Prop_*` instance of its name (a missing one made from `M_Prop` and the palette), cast shadow per prop. Prints what Unreal made. Saves the mesh and any instance it made; places nothing. Runs headless (the command is in its docstring). |
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
