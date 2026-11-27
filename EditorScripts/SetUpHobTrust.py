# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Set up Hob's trust track and passing lines (Docs/Systems/10-Robots.md, 11-Dialogue.md), headless, with the editor closed and VestigeEditor built:

    UnrealEditor-Cmd.exe Vestige.uproject -run=pythonscript -script=EditorScripts/SetUpHobTrust.py
        -EnablePlugins=PythonScriptPlugin -unattended -nopause -nullrhi -nosplash

Needs the first dialogue's assets (EditorScripts/SetUpHobDialogue.py has run: ST_Dialogue, DA_Character_Hob, BP_Hob). It makes or
updates, and saves:

1. ST_Dialogue: an entry for each line of Docs/Spoilers/Dialogue/Hob-Trust.md (the writer's Ready lines, read from the file's
   tables, key = the line ID), keeping the entries that are there.
2. DA_Character_Hob: TrustFromMilestones, 1 point each for the first harvest, the first sale and the first cleaning.
3. The 23 conversations in /Game/Vestige/Dialogue: nine farm topics, two attentive talks (H-05), "I'd like to" (H-06), the memory
   (H-07), two watered-morning lines (H-09) and the warmer greetings (H-08) of the four bands, each in two pools (with and
   without the memory lines). Every text is linked to its ST_Dialogue entry with TextFromStringTable, never a plain string.
   The priorities, the topic spacing and the thresholds are the TOPICS, STEPS and GREETINGS tables below, typed once.
4. BP_Hob: a UVestigeWateringHelpComponent with the help flags and 4 tiles.
5. The three passing lines (H-10, "Passing lines" in 11-Dialogue.md): DA_Dialogue_HobPassingHarvest, ...Bloom and ...Late, with
   delivery Passing, and the Harvest and Bloom topics' condition that keeps them off the day their passing line was said.
6. BP_Hob: a UVestigePassingLineComponent (its earshot is the component's default, 1800 cm, not overridden).

It changes nothing else. A rerun rewrites the same values. Every line it prints starts with "SetUpHobTrust:".
"""

import csv
import io
import os
import re

import unreal

TABLE_PATH = "/Game/Vestige/Dialogue/ST_Dialogue"
TABLE_ID = TABLE_PATH + ".ST_Dialogue"
LINES_FILE = "Docs/Spoilers/Dialogue/Hob-Trust.md"
CHARACTER_FOLDER = "/Game/Vestige/Characters"
CHARACTER_NAME = "DA_Character_Hob"
CHARACTER_ID = "Base.Hob"
BLUEPRINT_NAME = "BP_Hob"
DIALOGUE_FOLDER = "/Game/Vestige/Dialogue"

FLAG_ROOT = "Vestige.Relationship."
MILESTONE_ROOT = "Vestige.Milestone."
END_TALK = "End"

# Trust the farm's firsts give toward Hob (Systems/11-Dialogue.md).
TRUST_FROM_MILESTONES = ["Farm.Harvested", "Farm.Sold", "Farm.Cleaned"]

# The steps (Systems/11-Dialogue.md, "Hob's trust track"): trust to reach each.
ATTENTIVE_TRUST = 3
WOULD_LIKE_TRUST = 7
MEMORY_TRUST = 10
TRUST_PER_ASK = 1

# Hob's dawn watering (Systems/10-Robots.md).
HELP_TILES = 4
HELP_REQUIRED_FLAG = FLAG_ROOT + "Help.Watering"
HELP_HELPED_FLAG = FLAG_ROOT + "Helped.Watering"

# The topics, one asset each: (name, priority, gate conditions). Each has its own priority, the game designer's order (news before
# what is true on any day), because several are due at once from day 2. A topic is never offered two days running (TOPIC_SPACING_DAYS
# is the days a spacing flag blocks the next topic: 1 means today and yesterday), and not on day 1 (TOPIC_FROM_DAY).
TOPIC_FROM_DAY = 2
TOPIC_SPACING_DAYS = 1
TOPICS = [
    ("SeasonEnd", 64, [("date", TOPIC_FROM_DAY, "Vestige.Season.Spring", 19, 25)]),
    ("Harvest", 63, [("milestone", "Farm.Harvested")]),
    ("Bin", 62, [("milestone", "Farm.Sold")]),
    ("Bloom", 61, [("milestone", "Farm.HarvestedMutated")]),
    ("Sinkwort", 60, [("milestone", "Farm.PlantedCleaningCrop")]),
    ("Planting", 59, [("milestone", "Farm.Planted")]),
    ("Dispensers", 58, []),
    ("FarRows", 57, []),
    ("Well", 56, []),
]

# The passing lines (H-10), one asset each: (name, line ID, priority, event conditions, flag it sets). At most one a day (PassingSaid
# set today blocks the rest), once per game (its own flag), never after a talk about the event (the topic's flag), and never to a
# stranger. The first harvest and Bloom lines are due only on the day of their milestone ("today"); the late line from 00:15 on.
PASSING_LINES = [
    ("Harvest", 30, [("milestone", "Farm.Harvested", True)], "Topic.Harvest"),
    ("Bloom", 20, [("milestone", "Farm.HarvestedMutated", True)], "Topic.Bloom"),
    ("Late", 10, [("band", 24.25, 30.0)], None),
]

# The watered-morning lines (H-09). The first outranks every topic, a later one ranks below them and plays only when none was said in
# the last WATERED_QUIET_DAYS days.
WATERED_FIRST_PRIORITY = 65
WATERED_LATER_PRIORITY = 55
WATERED_QUIET_DAYS = 2

# The warmer greetings (H-08) join each band's everyday pool (H-02, made by SetUpHobDialogue.py): (band, from hour, to hour).
# Playing from the "I'd like to" step, and with the memory lines too from the memory.
GREETING_BANDS = [("Morning", 6.0, 12.0), ("Afternoon", 12.0, 18.0), ("Evening", 18.0, 24.0), ("Late", 24.0, 30.0)]
WARM_GREETING_PRIORITY = 51
MEMORY_GREETING_PRIORITY = 52
STEP_PRIORITIES = {"Memory": 90, "WouldLike": 85, "AttentiveAsked": 81, "Attentive": 80}


def log(message):
    unreal.log("SetUpHobTrust: " + message)


def fail(message):
    unreal.log_error("SetUpHobTrust: " + message)
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


def get_or_create(folder, name, asset_class, factory):
    path = folder + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return load(path)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, asset_class, factory)
    if asset is None:
        fail("Can't create " + path)
    log("created " + path)
    return asset


# ---- The writer's lines -> ST_Dialogue

def read_lines():
    """Every line of the writer's file as {line ID: (speaker, text)}, in file order, read from its tables (a row has the line ID in
    backticks, then HOB or PLAYER, then the text)."""
    path = os.path.join(unreal.SystemLibrary.get_project_directory(), LINES_FILE)
    if not os.path.exists(path):
        fail("Can't find " + path)
    lines = {}
    order = []
    with open(path, encoding="utf-8") as handle:
        for row in handle:
            match = (re.match(r"^\|\s*`(Hob\.[^`]+)`\s*\|\s*(HOB|PLAYER)\s*\|\s*(.*?)\s*\|\s*$", row)
                     or re.match(r"^\|[^|]*\|\s*`(Hob\.[^`]+)`\s*\|\s*(HOB|PLAYER)\s*\|\s*(.*?)\s*\|\s*$", row))
            if match:
                line_id, speaker, line_text = match.groups()
                if line_id in lines and lines[line_id][1] != line_text:
                    fail(line_id + " is in the file twice with different texts")
                if line_id not in lines:
                    order.append(line_id)
                lines[line_id] = (speaker, line_text)
    log("read {0} lines from {1}".format(len(order), LINES_FILE))
    return lines, order


def fill_string_table(lines, order):
    table = load(TABLE_PATH)
    table_id = unreal.StringTableLibrary.get_table_id(table)
    result = unreal.StringTableLibrary.export_table_to_csv_string(table_id)
    csv_text = result[1] if isinstance(result, tuple) else result
    rows = list(csv.reader(io.StringIO(csv_text, newline="")))
    if not rows:
        fail("ST_Dialogue exported no CSV")
    header = rows[0]
    key_column = header.index("Key")
    source_column = header.index("SourceString")
    entries = {}
    kept = []
    for row in rows[1:]:
        if row and row[key_column]:
            entries[row[key_column]] = len(kept)
            kept.append(row + [""] * (len(header) - len(row)))
    added = 0
    for line_id in order:
        line_text = lines[line_id][1]
        if line_id in entries:
            kept[entries[line_id]][source_column] = line_text
        else:
            row = [""] * len(header)
            row[key_column] = line_id
            row[source_column] = line_text
            kept.append(row)
            added += 1
    out = io.StringIO(newline="")
    writer = csv.writer(out, lineterminator="\n")
    writer.writerow(header)
    writer.writerows(kept)
    if not unreal.StringTableLibrary.import_table_from_csv_string(table_id, out.getvalue()):
        fail("Can't import the CSV into ST_Dialogue")
    for line_id in order:
        if unreal.StringTableLibrary.get_table_entry_source_string(table_id, line_id) != lines[line_id][1]:
            fail("ST_Dialogue's entry " + line_id + " isn't the writer's text after the import")
    save(table)
    log("fact: ST_Dialogue has {0} entries, {1} added".format(len(kept), added))


# ---- Data

def set_up_character():
    character = load(CHARACTER_FOLDER + "/" + CHARACTER_NAME)
    points = {}
    for milestone in TRUST_FROM_MILESTONES:
        points[make_tag(MILESTONE_ROOT + milestone)] = 1
    character.modify()
    character.set_editor_property("trust_from_milestones", points)
    save(character)
    log("fact: {0} trust from milestones: {1}".format(
        CHARACTER_NAME, sorted(str(unreal.GameplayTagLibrary.get_tag_name(tag)) for tag in
                               character.get_editor_property("trust_from_milestones").keys())))


def new_condition(owner, spec):
    kind = spec[0]
    if kind == "met":
        return configure(unreal.new_object(unreal.VestigeDialogueCondition_HasMet, outer=owner), met=spec[1])
    if kind == "talked":
        return configure(unreal.new_object(unreal.VestigeDialogueCondition_TalkedToday, outer=owner), talked=spec[1])
    if kind == "band":
        return configure(unreal.new_object(unreal.VestigeDialogueCondition_TimeOfDay, outer=owner), from_hour=spec[1], to_hour=spec[2])
    if kind == "trust":
        return configure(unreal.new_object(unreal.VestigeDialogueCondition_Trust, outer=owner), min_trust=spec[1])
    if kind == "flag":
        _, flag, is_set, min_days, max_days = spec
        return configure(unreal.new_object(unreal.VestigeDialogueCondition_Flag, outer=owner), flag=make_tag(FLAG_ROOT + flag),
                         set=is_set, min_days_since=min_days, max_days_since=max_days)
    if kind == "milestone":
        today = len(spec) > 2 and spec[2]
        return configure(unreal.new_object(unreal.VestigeDialogueCondition_Milestone, outer=owner),
                         milestone=make_tag(MILESTONE_ROOT + spec[1]), reached=True, today=today)
    if kind == "date":
        _, from_day, season, from_day_of_season, to_day_of_season = spec
        return configure(unreal.new_object(unreal.VestigeDialogueCondition_Date, outer=owner), from_day=from_day,
                         season=make_tag(season) if season else unreal.GameplayTag(),
                         from_day_of_season=from_day_of_season, to_day_of_season=to_day_of_season)
    fail("Unknown condition " + str(spec))


def new_outcome(owner, spec):
    if spec[0] == "trust":
        return configure(unreal.new_object(unreal.VestigeDialogueOutcome_AddTrust, outer=owner), amount=spec[1])
    if spec[0] == "flag":
        return configure(unreal.new_object(unreal.VestigeDialogueOutcome_SetFlag, outer=owner), flag=make_tag(FLAG_ROOT + spec[1]))
    fail("Unknown outcome " + str(spec))


def configure(obj, **values):
    for name, value in values.items():
        obj.set_editor_property(name, value)
    return obj


def not_flag(flag, max_days=-1):
    return ("flag", flag, False, 0, max_days)


def flag_set(flag, min_days=0, max_days=-1):
    return ("flag", flag, True, min_days, max_days)


def line(line_id, texts=None, next_id="", choices=(), outcomes=()):
    """A line spec: its ID, its text keys (the ID itself when none are given), where it goes next, its replies and its outcomes."""
    return {"id": line_id, "texts": texts or [line_id], "next": next_id, "choices": list(choices), "outcomes": list(outcomes)}


def reply(reply_id, next_id, text_key=None, conditions=(), outcomes=()):
    return {"id": reply_id, "text": text_key or reply_id, "next": next_id, "conditions": list(conditions), "outcomes": list(outcomes)}


def realize_line(owner, spec):
    result = unreal.VestigeDialogueLine()
    result.set_editor_property("line_id", spec["id"])
    result.set_editor_property("texts", [text(key) for key in spec["texts"]])
    result.set_editor_property("next", spec["next"])
    choices = []
    for choice_spec in spec["choices"]:
        choice = unreal.VestigeDialogueChoice()
        choice.set_editor_property("reply_id", choice_spec["id"])
        choice.set_editor_property("text", text(choice_spec["text"]))
        choice.set_editor_property("next", choice_spec["next"])
        choice.set_editor_property("conditions", [new_condition(owner, c) for c in choice_spec["conditions"]])
        choice.set_editor_property("outcomes", [new_outcome(owner, o) for o in choice_spec["outcomes"]])
        choices.append(choice)
    result.set_editor_property("choices", choices)
    result.set_editor_property("outcomes", [new_outcome(owner, o) for o in spec["outcomes"]])
    return result


def write_dialogue(name, priority, conditions, lines, delivery=unreal.VestigeDialogueDelivery.TALK):
    dialogue = get_or_create(DIALOGUE_FOLDER, "DA_Dialogue_" + name, unreal.VestigeDialogueData, unreal.DataAssetFactory())
    dialogue.modify()
    dialogue.set_editor_property("content_id", "Base." + name)
    dialogue.set_editor_property("speaker", unreal.PrimaryAssetId(unreal.PrimaryAssetType("VestigeCharacter"), CHARACTER_ID))
    dialogue.set_editor_property("priority", priority)
    dialogue.set_editor_property("delivery", delivery)
    dialogue.set_editor_property("conditions", [new_condition(dialogue, c) for c in conditions])
    dialogue.set_editor_property("lines", [realize_line(dialogue, spec) for spec in lines])
    save(dialogue)


# ---- The conversations

def topic_lines(name, line_ids):  # line_ids: the file's line IDs in order
    """H-04: the opening, the replies in the writer's order and the answers. Every answer ends the talk. The order reply is entered
    twice, one listed before the attentive step and one from it; only one is ever listed."""
    prefix = "Hob.Topic.{0}.".format(name)
    reply_names = [i[len(prefix + "Reply."):] for i in line_ids if i.startswith(prefix + "Reply.")]
    topic_flag = "Topic." + name
    choices = []
    for reply_name in reply_names:
        reply_id = prefix + "Reply." + reply_name
        if reply_name == "Ask":
            choices.append(reply(reply_id, prefix + "Answer.Ask", outcomes=[("trust", TRUST_PER_ASK), ("flag", "Asked"), ("flag", topic_flag)]))
        elif reply_name == "Order":
            choices.append(reply(reply_id, prefix + "Answer.Order", conditions=[not_flag("Step.Attentive")], outcomes=[("flag", topic_flag)]))
            choices.append(reply(prefix + "Reply.OrderView", prefix + "Answer.OrderView", text_key=reply_id,
                                 conditions=[flag_set("Step.Attentive")], outcomes=[("flag", topic_flag)]))
        elif reply_name == "Leave":
            choices.append(reply(reply_id, prefix + "Answer.Leave", outcomes=[("flag", topic_flag)]))
        else:
            fail("Unknown reply " + reply_id)
    lines = [line(prefix + "Open", choices=choices, outcomes=[("flag", "TopicOffered")])]
    for answer in ("Ask", "Order", "OrderView", "Leave"):
        answer_id = prefix + "Answer." + answer
        if answer_id in line_ids:
            lines.append(line(answer_id, next_id=END_TALK))
    return lines


def set_up_topics(order):
    passing_topics = {line[3] for line in PASSING_LINES if line[3]}
    for name, priority, gate in TOPICS:
        conditions = [("talked", False), not_flag("Topic." + name), not_flag("TopicOffered", TOPIC_SPACING_DAYS)] + gate
        if "Topic." + name in passing_topics:
            conditions.append(not_flag("Passing." + name, 0))
        if not any(condition[0] == "date" for condition in gate):
            conditions.append(("date", TOPIC_FROM_DAY, "", 1, 0))
        write_dialogue("HobTopic" + name, priority, conditions, topic_lines(name, order))


def set_up_steps():
    memory_flag = "Step.Memory"

    # H-05: the attentive talk, two versions of line 2. The one after an ask is higher, so with Asked set it wins.
    attentive = ["Hob.Step.Attentive.010", None, "Hob.Step.Attentive.030", "Hob.Step.Attentive.040"]
    for name, second, conditions in (
            ("HobAttentiveAsked", "Hob.Step.Attentive.020A", [flag_set("Asked")]),
            ("HobAttentive", "Hob.Step.Attentive.020B", [])):
        ids = [second if i is None else i for i in attentive]
        lines = [line(ids[0], outcomes=[("flag", "Step.Attentive")])] + [line(i) for i in ids[1:]]
        priority = STEP_PRIORITIES["AttentiveAsked" if conditions else "Attentive"]
        write_dialogue(name, priority, [("trust", ATTENTIVE_TRUST), not_flag("Step.Attentive")] + conditions, lines)

    # H-06: "I'd like to". The offer is made once a day at most and counts as made when line 1 shows; a reply decides.
    yes = reply("Hob.Step.Wish.Reply.Yes", "Hob.Step.Wish.Answer.Yes", outcomes=[("flag", "Help.Watering"), ("flag", "Step.WouldLike")])
    no = reply("Hob.Step.Wish.Reply.No", "Hob.Step.Wish.Answer.No", outcomes=[("flag", "Step.WouldLike")])
    write_dialogue("HobWouldLike", STEP_PRIORITIES["WouldLike"],
                   [("trust", WOULD_LIKE_TRUST), flag_set("Step.Attentive", 1), not_flag("Step.WouldLike"),
                    not_flag("Step.WouldLikeOffered", 0)],
                   [line("Hob.Step.Wish.010", outcomes=[("flag", "Step.WouldLikeOffered")]), line("Hob.Step.Wish.020"),
                    line("Hob.Step.Wish.030", choices=[yes, no]),
                    line("Hob.Step.Wish.Answer.Yes", next_id=END_TALK), line("Hob.Step.Wish.Answer.No", next_id=END_TALK)])

    # H-07: the memory. Three replies, each answer goes to the last line.
    prefix = "Hob.Step.Memory."
    replies = [reply(prefix + "Reply." + r, prefix + "Answer." + r) for r in ("Else", "Show", "Silent")]
    memory_lines = [line(prefix + "010", outcomes=[("flag", memory_flag)]), line(prefix + "020"), line(prefix + "030"),
                    line(prefix + "040", choices=replies)]
    memory_lines += [line(prefix + "Answer." + r, next_id=prefix + "050") for r in ("Else", "Show", "Silent")]
    memory_lines.append(line(prefix + "050"))
    write_dialogue("HobMemory", STEP_PRIORITIES["Memory"],
                   [("trust", MEMORY_TRUST), ("milestone", "Farm.Cleaned"), flag_set("Step.WouldLike", 1), not_flag(memory_flag)],
                   memory_lines)


def set_up_watered():
    pool = ["Hob.Watered.1", "Hob.Watered.2", "Hob.Watered.3"]
    watered = [("talked", False), flag_set("Helped.Watering", 0, 0)]
    said = [line("Hob.Watered", texts=pool, outcomes=[("flag", "WateredMentioned")])]
    write_dialogue("HobWateredFirst", WATERED_FIRST_PRIORITY, watered + [not_flag("WateredMentioned")], said)
    write_dialogue("HobWateredLater", WATERED_LATER_PRIORITY, watered + [not_flag("WateredMentioned", WATERED_QUIET_DAYS)], said)


def set_up_greetings():
    for band, from_hour, to_hour in GREETING_BANDS:
        everyday = ["Hob.Greet.{0}.{1}".format(band, n) for n in (1, 2, 3)]
        warm = ["Hob.Greet.Warm.{0}.{1}".format(band, n) for n in (1, 2)]
        memory = ["Hob.Greet.Memory.{0}".format(n) for n in (1, 2, 3)]
        base = [("met", True), ("talked", False), ("band", from_hour, to_hour)]
        write_dialogue("HobGreetingWarm" + band, WARM_GREETING_PRIORITY, base + [flag_set("Step.WouldLike")],
                       [line("Hob.Greet.Warm." + band, texts=everyday + warm)])
        write_dialogue("HobGreetingMemory" + band, MEMORY_GREETING_PRIORITY, base + [flag_set("Step.Memory")],
                       [line("Hob.Greet.Memory." + band, texts=everyday + warm + memory)])


def set_up_passing_lines():
    for name, priority, event, topic in PASSING_LINES:
        line_id = "Hob.Passing." + name
        conditions = [("met", True)] + event + [not_flag("Passing." + name), not_flag("PassingSaid", 0)]
        if topic:
            conditions.append(not_flag(topic))
        outcomes = [("flag", "Passing." + name), ("flag", "PassingSaid")]
        write_dialogue("HobPassing" + name, priority, conditions, [line(line_id, outcomes=outcomes)],
                       delivery=unreal.VestigeDialogueDelivery.PASSING)


# ---- BP_Hob

def set_up_watering_help():
    blueprint = load(CHARACTER_FOLDER + "/" + BLUEPRINT_NAME)
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    root_handle = None
    component = None
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        obj = library.get_associated_object(subsystem.k2_find_subobject_data_from_handle(handle))
        if isinstance(obj, unreal.CapsuleComponent):
            root_handle = handle
        elif isinstance(obj, unreal.VestigeWateringHelpComponent):
            component = obj
    if component is None:
        if root_handle is None:
            fail("BP_Hob has no capsule to add the watering help to")
        params = unreal.AddNewSubobjectParams(parent_handle=root_handle, new_class=unreal.VestigeWateringHelpComponent,
                                              blueprint_context=blueprint)
        handle, reason = subsystem.add_new_subobject(params)
        if not library.is_handle_valid(handle):
            fail("Can't add the watering help: " + str(reason))
        subsystem.rename_subobject(handle, "WateringHelp")
        component = library.get_associated_object(subsystem.k2_find_subobject_data_from_handle(handle))
        log("added the watering help to " + BLUEPRINT_NAME)

    # Set every run, so a rerun fixes a value that was changed by hand.
    component.set_editor_property("required_flag", make_tag(HELP_REQUIRED_FLAG))
    component.set_editor_property("helped_flag", make_tag(HELP_HELPED_FLAG))
    component.set_editor_property("max_tiles", HELP_TILES)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    save(blueprint)
    log("fact: WateringHelp required {0} helped {1} max tiles {2}".format(
        component.get_editor_property("required_flag"), component.get_editor_property("helped_flag"),
        component.get_editor_property("max_tiles")))


def set_up_passing_line_component():
    blueprint = load(CHARACTER_FOLDER + "/" + BLUEPRINT_NAME)
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    root_handle = None
    component = None
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        obj = library.get_associated_object(subsystem.k2_find_subobject_data_from_handle(handle))
        if isinstance(obj, unreal.CapsuleComponent):
            root_handle = handle
        elif isinstance(obj, unreal.VestigePassingLineComponent):
            component = obj
    if component is None:
        if root_handle is None:
            fail("BP_Hob has no capsule to add the passing line component to")
        params = unreal.AddNewSubobjectParams(parent_handle=root_handle, new_class=unreal.VestigePassingLineComponent,
                                              blueprint_context=blueprint)
        handle, reason = subsystem.add_new_subobject(params)
        if not library.is_handle_valid(handle):
            fail("Can't add the passing line component: " + str(reason))
        subsystem.rename_subobject(handle, "PassingLine")
        component = library.get_associated_object(subsystem.k2_find_subobject_data_from_handle(handle))
        log("added the passing line component to " + BLUEPRINT_NAME)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    save(blueprint)
    log("fact: PassingLine earshot radius {0}".format(component.get_editor_property("sphere_radius")))


def main():
    lines, order = read_lines()
    load(TABLE_PATH)
    fill_string_table(lines, order)
    set_up_character()
    set_up_topics(order)
    set_up_steps()
    set_up_watered()
    set_up_greetings()
    set_up_watering_help()
    set_up_passing_lines()
    set_up_passing_line_component()
    log("done")


if __name__ == "__main__":
    main()
