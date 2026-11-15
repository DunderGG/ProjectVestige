# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Pure-Python tests for the Editor-only level survey."""

import contextlib
import hashlib
import importlib.util
import io
import json
import os
from datetime import datetime, timezone
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest import mock


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "SurveyLevel.py"


class FakeVector:
    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x = x
        self.y = y
        self.z = z


class FakeRotation:
    def __init__(self, pitch=0.0, yaw=0.0, roll=0.0):
        self.pitch = pitch
        self.yaw = yaw
        self.roll = roll


class FakePackage:
    def __init__(self, name):
        self.name = name

    def get_name(self):
        return self.name


class FakeWorld:
    def __init__(self, package_path="/Game/Maps/L_Farm"):
        self.package = FakePackage(package_path)

    def get_outermost(self):
        return self.package


class FakeActorClass:
    def get_path_name(self):
        return "/Script/Engine.StaticMeshActor"


class FakeActor:
    def __init__(
        self,
        object_path,
        label="Actor",
        location=None,
        bounds=None,
        tags=None,
    ):
        self.object_path = object_path
        self.label = label
        self.location = location or FakeVector(1.0, 2.0, 3.0)
        self.bounds = bounds or (FakeVector(4.0, 5.0, 6.0), FakeVector(1.0, 2.0, 3.0))
        self.tags = tags or []
        self.label_arguments = []

    def get_path_name(self):
        return self.object_path

    def get_actor_label(self, mark_dirty=True):
        self.label_arguments.append(mark_dirty)
        return "GeneratedLabel" if mark_dirty and self.label is None else self.label

    def get_class(self):
        return FakeActorClass()

    def get_editor_property(self, property_name):
        if property_name != "tags":
            raise AssertionError("Unexpected editor property: {}".format(property_name))
        return self.tags

    def get_actor_location(self):
        return self.location

    def get_actor_rotation(self):
        return FakeRotation(10.0, 20.0, 30.0)

    def get_actor_scale3d(self):
        return FakeVector(1.0, 1.0, 1.0)

    def get_actor_bounds(self, only_colliding_components, include_from_child_actors):
        if only_colliding_components or include_from_child_actors:
            raise AssertionError("Survey must use the designed bounds arguments.")
        return self.bounds


class FakeActorSubsystem:
    def __init__(self, actors=None):
        self.actors = [] if actors is None else actors

    def get_all_level_actors(self):
        return self.actors


class FakeEditorSubsystem:
    def __init__(self, world=None, game_world=None):
        self.world = world
        self.game_world = game_world
        self.editor_world_calls = 0

    def get_game_world(self):
        return self.game_world

    def get_editor_world(self):
        self.editor_world_calls += 1
        return self.world


class SurveyLevelTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.saved_dir = self.temp_dir.name
        self.unreal = types.ModuleType("unreal")
        self.unreal.UnrealEditorSubsystem = type("UnrealEditorSubsystem", (), {})
        self.unreal.EditorActorSubsystem = type("EditorActorSubsystem", (), {})
        self.unreal.Paths = types.SimpleNamespace(
            project_saved_dir=lambda: self.saved_dir
        )
        self.unreal.EditorLoadingAndSavingUtils = types.SimpleNamespace(
            get_dirty_map_packages=lambda: []
        )
        self.unreal.get_editor_subsystem = lambda subsystem_type: None
        self.original_unreal = sys.modules.get("unreal")
        sys.modules["unreal"] = self.unreal

        spec = importlib.util.spec_from_file_location("survey_level_under_test", SCRIPT_PATH)
        self.survey = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.survey)

    def tearDown(self):
        if self.original_unreal is None:
            sys.modules.pop("unreal", None)
        else:
            sys.modules["unreal"] = self.original_unreal
        self.temp_dir.cleanup()

    def configure_editor(self, actors=None, world=None, game_world=None, dirty_queries=None):
        self.editor = FakeEditorSubsystem(
            world if world is not None else FakeWorld(),
            game_world=game_world,
        )
        self.actor_subsystem = FakeActorSubsystem(actors)
        subsystems = {
            self.unreal.UnrealEditorSubsystem: self.editor,
            self.unreal.EditorActorSubsystem: self.actor_subsystem,
        }
        self.unreal.get_editor_subsystem = lambda subsystem_type: subsystems.get(
            subsystem_type
        )
        if dirty_queries is not None:
            query_iterator = iter(dirty_queries)

            def get_dirty_packages():
                result = next(query_iterator)
                if isinstance(result, Exception):
                    raise result
                return [FakePackage(path) for path in result]

            self.unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages = (
                get_dirty_packages
            )

    def output_files(self):
        return list(Path(self.saved_dir).rglob("*"))

    def test_bounds_are_center_and_full_size_or_unavailable(self):
        actor = FakeActor(
            "/Game/Maps/L_Farm.L_Farm:PersistentLevel.Actor",
            bounds=(FakeVector(1, 2, 3), FakeVector(4, 5, 6)),
        )

        bounds = self.survey._read_bounds(actor, actor.object_path)
        self.assertEqual(
            bounds,
            {
                "center_cm": {"x": 1.0, "y": 2.0, "z": 3.0},
                "full_size_cm": {"x": 8.0, "y": 10.0, "z": 12.0},
            },
        )

        actor.bounds = (FakeVector(0, 0, 0), FakeVector(0, 0, 0))
        self.assertIsNone(self.survey._read_bounds(actor, actor.object_path))

    def test_bounds_reject_non_finite_measurements_with_actor_context(self):
        actor = FakeActor(
            "/Game/Maps/L_Farm.L_Farm:PersistentLevel.BadActor",
            bounds=(FakeVector(0, 0, 0), FakeVector(float("nan"), 1, 1)),
        )

        with self.assertRaisesRegex(self.survey.SurveyError, "BadActor.*non-finite"):
            self.survey._read_bounds(actor, actor.object_path)

    def test_non_finite_transform_is_rejected_with_actor_context(self):
        actor = FakeActor(
            "/Game/Maps/L_Farm.L_Farm:PersistentLevel.BadTransform",
            location=FakeVector(0, float("inf"), 0),
        )

        with self.assertRaisesRegex(
            self.survey.SurveyError, "BadTransform.*non-finite"
        ):
            self.survey._read_actor(actor, 0)

    def test_labels_are_read_without_creating_them(self):
        for label in (None, ""):
            with self.subTest(label=label):
                actor = FakeActor(
                    "/Game/Maps/L_Farm.L_Farm:PersistentLevel.Unlabeled",
                    label=label,
                )

                record = self.survey._read_actor(actor, 0)

                self.assertIsNone(record["label"])
                self.assertEqual(actor.label_arguments, [False])

    def test_duplicate_labels_remain_distinct_and_sort_by_object_path(self):
        first = FakeActor("/Game/Maps/L_Farm.L_Farm:PersistentLevel.Zed", "Same")
        second = FakeActor("/Game/Maps/L_Farm.L_Farm:PersistentLevel.Ada", "Same")
        self.configure_editor([first, second])

        report = self.survey._gather_report(
            self.actor_subsystem, "/Game/Maps/L_Farm", "timestamp"
        )

        self.assertEqual(
            [actor["object_path"] for actor in report["actors"]],
            [second.object_path, first.object_path],
        )
        self.assertEqual([actor["label"] for actor in report["actors"]], ["Same", "Same"])

    def test_full_map_identity_separates_same_named_map_output_paths(self):
        first = self.survey._output_directory(
            self.saved_dir, "/Game/Maps/Interior/L_House"
        )
        second = self.survey._output_directory(
            self.saved_dir, "/Game/Maps/Exterior/L_House"
        )

        self.assertNotEqual(first, second)
        self.assertTrue(first.endswith(hashlib.sha256(
            b"/Game/Maps/Interior/L_House"
        ).hexdigest()))

    def test_json_and_markdown_reports_include_identity_and_actor_data(self):
        actor = FakeActor(
            "/Game/Maps/L_Farm.L_Farm:PersistentLevel.Actor",
            "A|B",
            tags=["Z", "A"],
        )
        self.configure_editor([actor])
        report = self.survey._gather_report(
            self.actor_subsystem, "/Game/Maps/L_Farm", "2026-11-14T12:00:00+01:00"
        )

        decoded = json.loads(self.survey._json_bytes(report).decode("utf-8"))
        markdown = self.survey._markdown_report(report)

        self.assertEqual(decoded["map_package"], "/Game/Maps/L_Farm")
        self.assertEqual(decoded["actors"][0]["tags"], ["A", "Z"])
        self.assertIn("/Game/Maps/L_Farm", markdown)
        self.assertIn("A\\|B", markdown)
        self.assertIn("axis-aligned box", markdown)

    def test_empty_map_survey_does_not_assume_farm_actors(self):
        self.configure_editor([])
        output = io.StringIO()

        with contextlib.redirect_stdout(output):
            succeeded = self.survey.run_survey()

        self.assertTrue(succeeded)
        self.assertIn("0 actors surveyed", output.getvalue())
        json_path = next(
            path
            for path in Path(self.saved_dir).rglob("*.json")
            if not path.name.endswith(self.survey.MANIFEST_SUFFIX)
        )
        report = json.loads(json_path.read_text(encoding="utf-8"))
        self.assertEqual(report["actors"], [])
        self.assertEqual(report["map_package"], "/Game/Maps/L_Farm")
        self.assertEqual(self.editor.editor_world_calls, 1)

    def test_dirty_status_reports_unchanged_and_unknown_truthfully(self):
        actor_subsystem = FakeActorSubsystem([])
        self.configure_editor(dirty_queries=[[], []])
        unchanged = self.survey._gather_report(
            actor_subsystem, "/Game/Maps/L_Farm", "timestamp"
        )
        self.assertEqual(
            unchanged["dirty_map_status"],
            {"before": False, "after": False, "changed": False},
        )

        self.configure_editor(
            dirty_queries=[RuntimeError("query unavailable"), []]
        )
        unknown = self.survey._gather_report(
            actor_subsystem, "/Game/Maps/L_Farm", "timestamp"
        )
        self.assertEqual(
            unknown["dirty_map_status"],
            {"before": None, "after": False, "changed": None},
        )

    def test_play_world_is_refused_before_editor_world_or_file_access(self):
        self.configure_editor(
            world=FakeWorld(),
            game_world=object(),
        )
        output = io.StringIO()

        with contextlib.redirect_stdout(output):
            succeeded = self.survey.run_survey()

        self.assertFalse(succeeded)
        self.assertEqual(self.editor.editor_world_calls, 0)
        self.assertEqual(self.output_files(), [])
        self.assertIn("PIE or Simulate", output.getvalue())

    def test_missing_world_and_map_identity_fail_closed_without_writing(self):
        self.configure_editor()
        self.editor.world = None
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertFalse(self.survey.run_survey())
        self.assertEqual(self.output_files(), [])

        class BadWorld:
            def get_outermost(self):
                return FakePackage("L_Farm")

        self.configure_editor(world=BadWorld())
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertFalse(self.survey.run_survey())
        self.assertEqual(self.output_files(), [])

    def test_existing_reservation_uses_next_stem_exclusively(self):
        directory = Path(self.saved_dir) / "reports"
        directory.mkdir()
        timestamp = datetime(2026, 11, 14, tzinfo=timezone.utc)
        first_stem = self.survey._timestamp_stem(timestamp)
        first_reservation = directory / (first_stem + self.survey.RESERVATION_SUFFIX)
        first_reservation.write_text("owned by another run", encoding="utf-8")

        paths = self.survey._reserve_stem(str(directory), timestamp)

        self.assertTrue(paths["markdown"].endswith(first_stem + "_001.md"))
        self.assertEqual(first_reservation.read_text(encoding="utf-8"), "owned by another run")
        self.assertTrue(os.path.exists(paths["reservation"]))

    def test_existing_final_report_is_never_overwritten(self):
        directory = Path(self.saved_dir) / "reports"
        directory.mkdir()
        timestamp = datetime(2026, 11, 14, tzinfo=timezone.utc)
        first_stem = self.survey._timestamp_stem(timestamp)
        existing_report = directory / (first_stem + ".md")
        existing_report.write_text("keep this report", encoding="utf-8")

        paths = self.survey._reserve_stem(str(directory), timestamp)

        self.assertTrue(paths["markdown"].endswith(first_stem + "_001.md"))
        self.assertEqual(existing_report.read_text(encoding="utf-8"), "keep this report")

    def test_reports_publish_manifest_last_and_hashes_verify(self):
        report = self.minimal_report()
        timestamp = datetime(2026, 11, 14, tzinfo=timezone.utc)
        publish_order = []
        original_publish = self.survey._publish_non_overwriting

        def record_publish(temp_path, final_path, published_paths):
            publish_order.append(Path(final_path).name)
            return original_publish(temp_path, final_path, published_paths)

        with mock.patch.object(
            self.survey, "_publish_non_overwriting", side_effect=record_publish
        ):
            paths = self.survey._write_reports(self.saved_dir, report, timestamp)

        self.assertEqual(
            publish_order,
            [
                Path(paths["markdown"]).name,
                Path(paths["json"]).name,
                Path(paths["manifest"]).name,
            ],
        )
        self.assertTrue(publish_order[0].endswith(".md"))
        self.assertTrue(publish_order[1].endswith(".json"))
        self.assertTrue(publish_order[2].endswith(self.survey.MANIFEST_SUFFIX))
        self.survey._verify_published_pair(paths, json.loads(
            Path(paths["manifest"]).read_text(encoding="utf-8")
        ))
        manifest = json.loads(Path(paths["manifest"]).read_text(encoding="utf-8"))
        self.assertEqual(
            manifest["files"]["markdown"]["sha256"],
            hashlib.sha256(Path(paths["markdown"]).read_bytes()).hexdigest(),
        )
        self.assertTrue(Path(paths["json"]).exists())
        self.assertFalse(Path(paths["reservation"]).exists())

    def test_second_report_write_failure_cleans_staged_files_and_reservation(self):
        timestamp = datetime(2026, 11, 14, tzinfo=timezone.utc)
        original_write = self.survey._write_staged_file
        calls = []

        def fail_second_write(*args, **kwargs):
            calls.append(args[2])
            if len(calls) == 2:
                raise OSError("second write failed")
            return original_write(*args, **kwargs)

        with mock.patch.object(
            self.survey, "_write_staged_file", side_effect=fail_second_write
        ):
            with self.assertRaisesRegex(self.survey.SurveyError, "second write failed"):
                self.survey._write_reports(self.saved_dir, self.minimal_report(), timestamp)

        self.assertEqual(list(Path(self.saved_dir).rglob("*.tmp")), [])
        self.assertEqual(list(Path(self.saved_dir).rglob("*.reserve")), [])
        self.assertEqual(list(Path(self.saved_dir).rglob("*.complete.json")), [])

    def test_second_report_publish_failure_removes_published_first_report(self):
        timestamp = datetime(2026, 11, 14, tzinfo=timezone.utc)
        original_publish = self.survey._publish_non_overwriting
        calls = []

        def fail_second_publish(temp_path, final_path, published_paths):
            calls.append(final_path)
            if len(calls) == 2:
                raise OSError("second publish failed")
            return original_publish(temp_path, final_path, published_paths)

        with mock.patch.object(
            self.survey, "_publish_non_overwriting", side_effect=fail_second_publish
        ):
            with self.assertRaisesRegex(self.survey.SurveyError, "second publish failed"):
                self.survey._write_reports(self.saved_dir, self.minimal_report(), timestamp)

        self.assertEqual(list(Path(self.saved_dir).rglob("*.md")), [])
        self.assertEqual(list(Path(self.saved_dir).rglob("*.tmp")), [])
        self.assertEqual(list(Path(self.saved_dir).rglob("*.reserve")), [])

    def test_cleanup_failure_is_reported_and_entrypoint_does_not_claim_success(self):
        self.configure_editor([])
        original_write = self.survey._write_staged_file
        write_calls = []

        def fail_second_write(*args, **kwargs):
            write_calls.append(args[2])
            if len(write_calls) == 2:
                raise OSError("second write failed")
            return original_write(*args, **kwargs)

        original_remove = os.remove

        def fail_reservation_remove(path):
            if path.endswith(self.survey.RESERVATION_SUFFIX):
                raise PermissionError("reservation is locked")
            return original_remove(path)

        output = io.StringIO()
        with mock.patch.object(
            self.survey, "_write_staged_file", side_effect=fail_second_write
        ), mock.patch.object(self.survey.os, "remove", side_effect=fail_reservation_remove):
            with contextlib.redirect_stdout(output):
                succeeded = self.survey.run_survey()

        self.assertFalse(succeeded)
        self.assertIn("Cleanup failed", output.getvalue())
        self.assertIn("Failed:", output.getvalue())
        self.assertNotIn("Complete:", output.getvalue())

    def minimal_report(self):
        return {
            "schema_version": 1,
            "generated_at": "2026-11-14T12:00:00+01:00",
            "map_package": "/Game/Maps/L_Farm",
            "actor_scope": self.survey.ACTOR_SCOPE,
            "units": self.survey.UNITS,
            "bounds_method": self.survey.BOUNDS_METHOD,
            "dirty_map_status": {"before": False, "after": False, "changed": False},
            "actors": [],
        }


if __name__ == "__main__":
    unittest.main()
