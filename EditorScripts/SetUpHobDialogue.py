# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Set up Hob's data and the dialogue box's assets (Docs/Systems/10-Robots.md, 11-Dialogue.md, 06-UserInterface.md), headless,
with the editor closed and VestigeEditor built:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script=EditorScripts/SetUpHobDialogue.py
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

Needs /Game/Vestige/Dialogue/ST_Dialogue (the string table, keyed by the writer's line IDs) and the widget Blueprints in
/Game/Vestige/UI (WBP_Layout, WBP_Dialogue, WBP_DialogueChoice). It makes or updates, and saves:

1. IA_DialogueNext, IA_DialogueEnd and IMC_Dialogue in /Game/Vestige/Input (F, Space, Enter, gamepad B and A for Next, B first as the key that opened the talk; Escape for End).
2. DA_Character_Hob (Base.Hob, robot) in /Game/Vestige/Characters.
3. The six conversations in /Game/Vestige/Dialogue (first meeting 100, four greetings 50, again today 10), each text linked to
   its ST_Dialogue entry with TextFromStringTable, never a plain string.
4. DA_FeedbackResponses: the row Vestige.Feedback.Dialogue.NothingToSay.
5. WBP_Dialogue's defaults (choice button, actions, mapping context, typing speed) and BP_HUD's layout and dialogue classes.

It changes nothing else. A rerun rewrites the same values. Every line it prints starts with "SetUpHobDialogue:".
"""

import unreal

TABLE_ID = "/Game/Vestige/Dialogue/ST_Dialogue.ST_Dialogue"
INPUT_FOLDER = "/Game/Vestige/Input"
CHARACTER_FOLDER = "/Game/Vestige/Characters"
DIALOGUE_FOLDER = "/Game/Vestige/Dialogue"
UI_FOLDER = "/Game/Vestige/UI"
FEEDBACK_RESPONSES = UI_FOLDER + "/DA_FeedbackResponses"
NOTHING_TO_SAY = ("Vestige.Feedback.Dialogue.NothingToSay", "Nothing to say right now.")

# (asset name, mappable name tag, description key in ST_Dialogue, keys)
ACTIONS = [
    ("IA_DialogueNext", "Vestige.Input.DialogueNext", "UI.DialogueNext",
     ["F", "SpaceBar", "Enter", "Gamepad_FaceButton_Right", "Gamepad_FaceButton_Bottom"]),
    ("IA_DialogueEnd", "Vestige.Input.DialogueEnd", "UI.DialogueEnd", ["Escape"]),
]

CHARACTER_NAME = "DA_Character_Hob"
CHARACTER_ID = "Base.Hob"

FIRST_MEETING = {"name": "DA_Dialogue_HobFirstMeeting", "id": "Base.HobFirstMeeting", "priority": 100}
# The bands are the writer's assumption until the game designer sets them (11-Dialogue.md): 6-12, 12-18, 18-24, 24-30.
GREETINGS = [
    ("DA_Dialogue_HobGreetingMorning", "Base.HobGreetingMorning", "Hob.Greet.Morning", 6.0, 12.0),
    ("DA_Dialogue_HobGreetingAfternoon", "Base.HobGreetingAfternoon", "Hob.Greet.Afternoon", 12.0, 18.0),
    ("DA_Dialogue_HobGreetingEvening", "Base.HobGreetingEvening", "Hob.Greet.Evening", 18.0, 24.0),
    ("DA_Dialogue_HobGreetingLate", "Base.HobGreetingLate", "Hob.Greet.Late", 24.0, 30.0),
]
GREETING_PRIORITY = 50
AGAIN = ("DA_Dialogue_HobAgainToday", "Base.HobAgainToday", "Hob.Again", 10)


def log(message):
    unreal.log("SetUpHobDialogue: " + message)


def fail(message):
    unreal.log_error("SetUpHobDialogue: " + message)
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


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text('(TagName="{0}")'.format(name))
    if not unreal.GameplayTagLibrary.is_gameplay_tag_valid(tag):
        fail("The tag manager doesn't know " + name + "; build VestigeEditor first")
    return tag


def text(key):
    """A text linked to its ST_Dialogue entry, so it is translated by its key."""
    return unreal.TextLibrary.text_from_string_table(TABLE_ID, key)


def make_id(asset_type, name):
    return unreal.PrimaryAssetId(unreal.PrimaryAssetType(asset_type), name)


def get_or_create(folder, name, asset_class, factory):
    path = folder + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return load(path)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, asset_class, factory)
    if asset is None:
        fail("Can't create " + path)
    log("created " + path)
    return asset


def make_key(name):
    key = unreal.Key()
    key.import_text(name)
    return key


def set_up_input():
    context = get_or_create(INPUT_FOLDER, "IMC_Dialogue", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
    context.unmap_all()
    actions = {}
    for name, tag_name, text_key, keys in ACTIONS:
        action = get_or_create(INPUT_FOLDER, name, unreal.InputAction, unreal.InputAction_Factory())
        action.modify()
        action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
        action.set_editor_property("action_description", text(text_key))

        # The settings carry no Common Input metadata: that would make the router inject the action instead of firing the
        # box's UI binding (06-UserInterface.md, pitfalls). The name is the tag's name, the key a saved remap is stored under.
        settings = unreal.new_object(unreal.PlayerMappableKeySettings, outer=action)
        settings.set_editor_property("name", tag_name)
        settings.set_editor_property("display_name", text(text_key))
        action.set_editor_property("player_mappable_key_settings", settings)
        make_tag(tag_name)
        save(action)
        actions[name] = action
        for key_name in keys:
            context.map_key(action, make_key(key_name))
    context.modify()
    save(context)
    for mapping in context.get_editor_property("default_key_mappings").get_editor_property("mappings"):
        log("fact: IMC_Dialogue {0} = {1}".format(mapping.get_editor_property("action").get_name(),
                                                  mapping.get_editor_property("key").get_editor_property("key_name")))
    return actions, context


def set_up_character():
    character = get_or_create(CHARACTER_FOLDER, CHARACTER_NAME, unreal.VestigeCharacterData, unreal.DataAssetFactory())
    character.modify()
    character.set_editor_property("content_id", CHARACTER_ID)
    character.set_editor_property("display_name", text("Character.Hob.Name"))
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="Vestige.Character.Robot")))')
    character.set_editor_property("character_tags", tags)
    save(character)


def new_condition(owner, condition_class, **values):
    condition = unreal.new_object(condition_class, outer=owner)
    for name, value in values.items():
        condition.set_editor_property(name, value)
    return condition


def make_line(line_id, text_keys, next_id="", choices=()):
    line = unreal.VestigeDialogueLine()
    line.set_editor_property("line_id", line_id)
    line.set_editor_property("texts", [text(key) for key in text_keys])
    line.set_editor_property("next", next_id)
    line.set_editor_property("choices", list(choices))
    return line


def make_choice(reply_id, next_id):
    choice = unreal.VestigeDialogueChoice()
    choice.set_editor_property("reply_id", reply_id)
    choice.set_editor_property("text", text(reply_id))
    choice.set_editor_property("next", next_id)
    return choice


def write_dialogue(name, content_id, priority, conditions, lines):
    dialogue = get_or_create(DIALOGUE_FOLDER, name, unreal.VestigeDialogueData, unreal.DataAssetFactory())
    dialogue.modify()
    dialogue.set_editor_property("content_id", content_id)
    dialogue.set_editor_property("speaker", make_id("VestigeCharacter", CHARACTER_ID))
    dialogue.set_editor_property("priority", priority)
    dialogue.set_editor_property("conditions", [new_condition(dialogue, cls, **values) for cls, values in conditions])
    dialogue.set_editor_property("lines", lines)
    save(dialogue)


def set_up_dialogues():
    has_met = unreal.VestigeDialogueCondition_HasMet
    talked = unreal.VestigeDialogueCondition_TalkedToday
    band = unreal.VestigeDialogueCondition_TimeOfDay

    # The first meeting: two lines, three replies under the second, each reply's answer, then the same last line.
    answers = ["Thanks", "Name", "Silent"]
    meeting = [
        make_line("Hob.Meet.010", ["Hob.Meet.010"]),
        make_line("Hob.Meet.020", ["Hob.Meet.020"],
                  choices=[make_choice("Hob.Meet.Reply." + a, "Hob.Meet.Answer." + a) for a in answers]),
    ]
    meeting += [make_line("Hob.Meet.Answer." + a, ["Hob.Meet.Answer." + a], next_id="Hob.Meet.030") for a in answers]
    meeting.append(make_line("Hob.Meet.030", ["Hob.Meet.030"]))
    write_dialogue(FIRST_MEETING["name"], FIRST_MEETING["id"], FIRST_MEETING["priority"], [(has_met, {"met": False})], meeting)

    # A greeting is one line with a pool of three texts.
    for name, content_id, line_id, from_hour, to_hour in GREETINGS:
        write_dialogue(name, content_id, GREETING_PRIORITY,
                       [(has_met, {}), (talked, {"talked": False}), (band, {"from_hour": from_hour, "to_hour": to_hour})],
                       [make_line(line_id, [line_id + "." + str(n) for n in (1, 2, 3)])])

    name, content_id, line_id, priority = AGAIN
    write_dialogue(name, content_id, priority, [(has_met, {}), (talked, {})],
                   [make_line(line_id, [line_id + "." + str(n) for n in (1, 2, 3)])])


def set_up_feedback_row():
    tag_name, message = NOTHING_TO_SAY
    asset = load(FEEDBACK_RESPONSES)
    responses = asset.get_editor_property("responses")
    existing = [str(unreal.GameplayTagLibrary.get_tag_name(tag)) for tag in responses.keys()]
    if tag_name in existing:
        log("DA_FeedbackResponses already has " + tag_name)
        return
    response = unreal.VestigeFeedbackResponse()
    key = tag_name[len("Vestige.Feedback."):]
    response.import_text('(Message=NSLOCTEXT("VestigeFeedback", "{0}", "{1}"))'.format(key, message))
    responses[make_tag(tag_name)] = response
    asset.modify()
    asset.set_editor_property("responses", responses)
    save(asset)


def set_up_ui(actions, context):
    choice = load(UI_FOLDER + "/WBP_DialogueChoice")
    layout = load(UI_FOLDER + "/WBP_Layout")
    box = load(UI_FOLDER + "/WBP_Dialogue")
    unreal.BlueprintEditorLibrary.compile_blueprint(box)
    defaults = unreal.get_default_object(box.generated_class())
    defaults.set_editor_property("choice_button_class", choice.generated_class())
    defaults.set_editor_property("next_action", actions["IA_DialogueNext"])
    defaults.set_editor_property("end_action", actions["IA_DialogueEnd"])
    defaults.set_editor_property("input_mapping", context)
    defaults.set_editor_property("characters_per_second", 40.0)
    save(box)

    # WBP_Hud keeps its layout; compiling it picks up its new Common UI base.
    hud_widget = load(UI_FOLDER + "/WBP_Hud")
    unreal.BlueprintEditorLibrary.compile_blueprint(hud_widget)
    save(hud_widget)

    hud = load("/Game/Vestige/Blueprints/BP_HUD")
    hud_defaults = unreal.get_default_object(hud.generated_class())
    hud_defaults.set_editor_property("layout_widget_class", layout.generated_class())
    hud_defaults.set_editor_property("dialogue_widget_class", box.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(hud)
    save(hud)


def main():
    if not unreal.EditorAssetLibrary.does_asset_exist(TABLE_ID.split(".")[0]):
        fail("ST_Dialogue is missing")
    # A text links to a table that is loaded: a table nothing has loaded yet answers with a missing entry.
    load(TABLE_ID.split(".")[0])
    actions, context = set_up_input()
    set_up_character()
    set_up_dialogues()
    set_up_feedback_row()
    set_up_ui(actions, context)
    log("done")


if __name__ == "__main__":
    main()
