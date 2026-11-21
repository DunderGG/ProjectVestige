# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Set up the audio content (Docs/Systems/14-Audio.md), headless, with the editor closed and VestigeEditor built:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script=EditorScripts/SetUpAudio.py
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

Run Art/Audio/make_water_fill_can.py first, so Art/Export/S_Water_FillCan.wav exists.

1. SCL_Overall and SCL_Effects (/Game/Vestige/Audio/Mix): made if missing; Effects is made a child of Overall.
2. S_Water_FillCan (/Game/Vestige/Audio/Water): imported from the WAV if missing (or again when the
   environment variable VESTIGE_REIMPORT_AUDIO is 1), its sound class set to SCL_Effects and its loading behaviour to Retain On Load.
3. DA_FeedbackResponses: sets Sound on the Water.CanFilled and Water.WellRanDry rows, leaving their texts.

It changes and saves those assets and nothing else. Run it again at any time: it only writes what differs. Every line it
prints starts with "SetUpAudio:".
"""

import os

import unreal

MIX_FOLDER = "/Game/Vestige/Audio/Mix"
WATER_FOLDER = "/Game/Vestige/Audio/Water"
OVERALL_NAME = "SCL_Overall"
EFFECTS_NAME = "SCL_Effects"
SPLASH_NAME = "S_Water_FillCan"
SPLASH_FILE = os.path.join(unreal.Paths.project_dir(), "Art", "Export", "S_Water_FillCan.wav")
FEEDBACK_RESPONSES = "/Game/Vestige/UI/DA_FeedbackResponses"
SPLASH_TAGS = ("Vestige.Feedback.Water.CanFilled", "Vestige.Feedback.Water.WellRanDry")
REIMPORT = os.environ.get("VESTIGE_REIMPORT_AUDIO") == "1"


def log(message):
    unreal.log("SetUpAudio: " + message)


def fail(message):
    unreal.log_error("SetUpAudio: " + message)
    raise RuntimeError(message)


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        fail("Can't load " + path)
    return asset


def save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        fail("Can't save " + asset.get_path_name())
    log("saved " + asset.get_path_name())


def make_sound_class(name):
    """Returns the class and whether it was just made."""
    path = MIX_FOLDER + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        log(path + " exists")
        return load(path), False
    sound_class = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, MIX_FOLDER, unreal.SoundClass, unreal.SoundClassFactory())
    if sound_class is None:
        fail("Can't create " + path)
    log("created " + path)
    return sound_class, True


def set_up_sound_classes():
    overall, overall_made = make_sound_class(OVERALL_NAME)
    effects, effects_made = make_sound_class(EFFECTS_NAME)
    changed = overall_made or effects_made
    children = list(overall.get_editor_property("child_classes"))
    if effects not in children:
        children.append(effects)
        overall.modify()
        overall.set_editor_property("child_classes", children)
        log("made " + EFFECTS_NAME + " a child of " + OVERALL_NAME)
        changed = True
    # ParentClass isn't editable: the parent's PostEditChangeProperty sets it. Check, since nothing repairs a mismatch on load.
    if effects.get_editor_property("parent_class") != overall:
        fail(EFFECTS_NAME + "'s parent class isn't " + OVERALL_NAME + " after setting Overall's child classes")
    if changed:
        save(overall)
        save(effects)
    return effects


def import_splash():
    path = WATER_FOLDER + "/" + SPLASH_NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not REIMPORT:
        log(path + " exists")
        return load(path)
    if not os.path.isfile(SPLASH_FILE):
        fail("Can't find " + SPLASH_FILE + "; run Art/Audio/make_water_fill_can.py first")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SPLASH_FILE)
    task.set_editor_property("destination_path", WATER_FOLDER)
    task.set_editor_property("destination_name", SPLASH_NAME)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    wave = unreal.EditorAssetLibrary.load_asset(path)
    if wave is None or not isinstance(wave, unreal.SoundWave):
        fail("Importing " + SPLASH_FILE + " didn't give a SoundWave at " + path)
    log("imported " + path)
    return wave


def set_up_splash(effects):
    wave = import_splash()
    wave.modify()
    changed = False
    if wave.get_editor_property("sound_class_object") != effects:
        wave.set_editor_property("sound_class_object", effects)
        changed = True
    retain = unreal.SoundWaveLoadingBehavior.RETAIN_ON_LOAD
    if wave.get_editor_property("loading_behavior") != retain:
        wave.set_editor_property("loading_behavior", retain)
        changed = True
    if wave.get_editor_property("looping"):
        wave.set_editor_property("looping", False)
        changed = True
    if changed:
        save(wave)
    else:
        log(SPLASH_NAME + " already has its sound class and loading behaviour")
    return wave


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text('(TagName="{0}")'.format(name))
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag):
        fail("The tag manager doesn't know " + name + "; build VestigeEditor first")
    return tag


def set_feedback_sounds(wave):
    asset = load(FEEDBACK_RESPONSES)
    responses = asset.get_editor_property("responses")
    changed = False
    for tag_name in SPLASH_TAGS:
        make_tag(tag_name)
        matches = [key for key in responses.keys() if str(unreal.GameplayTagLibrary.get_tag_name(key)) == tag_name]
        if not matches:
            fail("DA_FeedbackResponses has no row " + tag_name + "; run SetUpWell.py first")
        response = responses[matches[0]]
        if response.get_editor_property("sound") == wave:
            log("the row " + tag_name + " already plays the splash")
            continue
        # Sound is EditDefaultsOnly, which set_editor_property refuses on a struct; import_text sets only the named field.
        response.import_text("(Sound={0})".format(wave.get_path_name()))
        if response.get_editor_property("sound") != wave:
            fail("Couldn't set the sound of the row " + tag_name)
        responses[matches[0]] = response
        log("set the sound of the row " + tag_name)
        changed = True
    if not changed:
        return
    asset.modify()
    asset.set_editor_property("responses", responses)
    save(asset)


effects_class = set_up_sound_classes()
splash = set_up_splash(effects_class)
set_feedback_sounds(splash)
log("done")
