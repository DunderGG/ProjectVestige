# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Write a read-only survey of the currently open Unreal Editor world."""

import hashlib
import json
import math
import os
import re
import uuid
from datetime import datetime

import unreal


SCHEMA_VERSION = 1
ACTOR_SCOPE = {
    "api": "unreal.EditorActorSubsystem.get_all_level_actors",
    "description": (
        "Unfiltered actors returned for the currently open Editor world. This is the loaded, "
        "editable, Outliner-eligible actor scope, not every object serialized in the map."
    ),
    "exclusions": [
        "World Settings",
        "builder brush",
        "transient actors",
        "templates",
        "pending-kill actors",
        "actors not listed in the Outliner",
        "unloaded sublevels and World Partition cells",
    ],
}
UNITS = {
    "location": "centimeters",
    "bounds": "centimeters",
    "rotation": "degrees (pitch, yaw, roll)",
    "scale": "dimensionless",
}
BOUNDS_METHOD = {
    "api": "AActor.GetActorBounds",
    "python_method": "Actor.get_actor_bounds",
    "arguments": {
        "only_colliding_components": False,
        "include_from_child_actors": False,
    },
    "description": "Axis-aligned world bounds; the API returns center and half-extents.",
}
REPORT_DIR_NAME = "LevelReports"
RESERVATION_SUFFIX = ".reserve"
MARKDOWN_SUFFIX = ".md"
JSON_SUFFIX = ".json"
MANIFEST_SUFFIX = ".complete.json"


class SurveyError(Exception):
    """A fail-closed survey or report-publication error."""


def _vector_values(vector, field_name, actor_path):
    """Return finite x/y/z numbers from a UE vector-like value."""
    try:
        values = {
            "x": _finite_number(vector.x, field_name + ".x", actor_path),
            "y": _finite_number(vector.y, field_name + ".y", actor_path),
            "z": _finite_number(vector.z, field_name + ".z", actor_path),
        }
    except AttributeError as error:
        raise SurveyError(
            "Actor {!r} has an unreadable {} vector: {}".format(actor_path, field_name, error)
        )
    return values


def _finite_number(value, field_name, actor_path):
    """Convert a measurement to a standard JSON number or fail with actor context."""
    try:
        number = float(value)
    except (TypeError, ValueError, OverflowError) as error:
        raise SurveyError(
            "Actor {!r} has an invalid {} measurement: {}".format(actor_path, field_name, error)
        )
    if not math.isfinite(number):
        raise SurveyError(
            "Actor {!r} has a non-finite {} measurement: {!r}".format(
                actor_path, field_name, number
            )
        )
    return number


def _actor_path(actor, fallback):
    try:
        object_path = actor.get_path_name()
    except Exception as error:
        raise SurveyError(
            "Could not read object path for actor {}: {}".format(fallback, error)
        )
    if not isinstance(object_path, str) or not object_path:
        raise SurveyError("Actor {} returned no object path.".format(fallback))
    return object_path


def _read_bounds(actor, object_path):
    """Read the Python binding's (origin, half-extents) result defensively."""
    try:
        bounds_result = actor.get_actor_bounds(False, False)
    except Exception as error:
        raise SurveyError(
            "Actor {!r} bounds query failed: {}".format(object_path, error)
        )

    if not isinstance(bounds_result, (tuple, list)) or len(bounds_result) != 2:
        raise SurveyError(
            "Actor {!r} returned an unsupported get_actor_bounds result {!r}; "
            "expected (origin, half_extents).".format(object_path, bounds_result)
        )

    origin, half_extents = bounds_result
    center = _vector_values(origin, "bounds center", object_path)
    extents = _vector_values(half_extents, "bounds half-extents", object_path)
    full_size = {
        axis: _finite_number(
            extents[axis] * 2.0, "bounds full size.{}".format(axis), object_path
        )
        for axis in ("x", "y", "z")
    }

    if all(full_size[axis] == 0.0 for axis in ("x", "y", "z")):
        return None

    return {"center_cm": center, "full_size_cm": full_size}


def _read_actor(actor, index):
    """Read one actor without changing it."""
    fallback = "at index {}".format(index)
    object_path = _actor_path(actor, fallback)
    try:
        label = actor.get_actor_label(False)
        class_path = actor.get_class().get_path_name()
        tags = sorted(str(tag) for tag in actor.get_editor_property("tags"))
        location = _vector_values(actor.get_actor_location(), "location", object_path)
        rotation_value = actor.get_actor_rotation()
        rotation = {
            "pitch": _finite_number(rotation_value.pitch, "rotation.pitch", object_path),
            "yaw": _finite_number(rotation_value.yaw, "rotation.yaw", object_path),
            "roll": _finite_number(rotation_value.roll, "rotation.roll", object_path),
        }
        scale = _vector_values(actor.get_actor_scale3d(), "scale", object_path)
        bounds = _read_bounds(actor, object_path)
    except SurveyError:
        raise
    except Exception as error:
        raise SurveyError("Actor {!r} could not be surveyed: {}".format(object_path, error))

    if not isinstance(class_path, str) or not class_path:
        raise SurveyError("Actor {!r} returned no class path.".format(object_path))
    if label is not None:
        label = str(label)
        if not label:
            label = None

    return {
        "object_path": object_path,
        "label": label,
        "class_path": class_path,
        "tags": tags,
        "transform": {
            "location_cm": location,
            "rotation_degrees": rotation,
            "scale": scale,
        },
        "bounds_cm": bounds,
    }


def _read_map_package_path(world):
    """Return the full package identity, refusing an unavailable or malformed path."""
    try:
        package = world.get_outermost()
        package_path = package.get_name()
    except Exception as error:
        raise SurveyError("Could not obtain the open map's package path: {}".format(error))
    if not isinstance(package_path, str) or not package_path.startswith("/") or package_path == "/":
        raise SurveyError(
            "Could not obtain a full map package path; received {!r}.".format(package_path)
        )
    return package_path


def _read_dirty_status(package_path):
    """Return True, False, or None when the Editor's dirty-map query is inconclusive."""
    try:
        dirty_packages = unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    except Exception as error:
        print("[SurveyLevel] Dirty-map status is unknown: {}".format(error))
        return None

    if dirty_packages is None:
        print("[SurveyLevel] Dirty-map status is unknown: query returned no result.")
        return None

    try:
        dirty_package_paths = []
        for package in dirty_packages:
            dirty_path = package.get_name()
            if not isinstance(dirty_path, str) or not dirty_path.startswith("/"):
                raise ValueError("dirty package returned an invalid package path")
            dirty_package_paths.append(dirty_path)
    except Exception as error:
        print("[SurveyLevel] Dirty-map status is unknown: {}".format(error))
        return None

    return package_path in dirty_package_paths


def _safe_map_slug(package_path):
    """Create a filesystem-safe slug from the short map package name."""
    short_name = package_path.rsplit("/", 1)[-1]
    slug = re.sub(r"[^A-Za-z0-9_-]+", "_", short_name).strip("_-")
    return slug or "map"


def _output_directory(saved_dir, package_path):
    digest = hashlib.sha256(package_path.encode("utf-8")).hexdigest()
    folder_name = "{}_{}".format(_safe_map_slug(package_path), digest)
    return os.path.join(saved_dir, REPORT_DIR_NAME, folder_name)


def _timestamp_stem(timestamp):
    return "survey_{}".format(timestamp.strftime("%Y%m%dT%H%M%S%f%z"))


def _final_paths(directory, stem):
    return {
        "markdown": os.path.join(directory, stem + MARKDOWN_SUFFIX),
        "json": os.path.join(directory, stem + JSON_SUFFIX),
        "manifest": os.path.join(directory, stem + MANIFEST_SUFFIX),
        "reservation": os.path.join(directory, stem + RESERVATION_SUFFIX),
    }


def _has_final_output(paths):
    return any(
        os.path.lexists(paths[key])
        for key in ("markdown", "json", "manifest")
    )


def _reserve_stem(directory, timestamp):
    """Exclusively reserve the first available stem and return its final paths."""
    base_stem = _timestamp_stem(timestamp)
    suffix = 0
    while True:
        stem = base_stem if suffix == 0 else "{}_{:03d}".format(base_stem, suffix)
        paths = _final_paths(directory, stem)

        if _has_final_output(paths):
            suffix += 1
            continue

        try:
            with open(paths["reservation"], "x", encoding="utf-8"):
                pass
        except FileExistsError:
            suffix += 1
            continue

        if _has_final_output(paths):
            try:
                os.remove(paths["reservation"])
            except OSError as error:
                print(
                    "[SurveyLevel] Cleanup failed; remaining path {!r}: {}".format(
                        paths["reservation"], error
                    )
                )
                raise SurveyError(
                    "Could not release colliding reservation {!r}: {}".format(
                        paths["reservation"], error
                    )
                )
            suffix += 1
            continue

        return paths


def _json_bytes(value):
    return (json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + "\n").encode(
        "utf-8"
    )


def _markdown_cell(value):
    text = str(value)
    return text.replace("\\", "\\\\").replace("|", "\\|").replace("\n", "<br>")


def _markdown_code(value):
    text = str(value)
    return "`{}`".format(text.replace("`", "\\`"))


def _format_vector(value):
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"), allow_nan=False)


def _markdown_report(report):
    dirty_status = report["dirty_map_status"]
    lines = [
        "# Level survey",
        "",
        "- Generated: {}".format(report["generated_at"]),
        "- Map package: {}".format(_markdown_code(report["map_package"])),
        "- Actors surveyed: {}".format(len(report["actors"])),
        "- Dirty map before: {}".format(json.dumps(dirty_status["before"])),
        "- Dirty map after: {}".format(json.dumps(dirty_status["after"])),
        "- Dirty status changed: {}".format(json.dumps(dirty_status["changed"])),
        "",
        "## Scope",
        "",
        report["actor_scope"]["description"],
        "",
        "API: {}".format(_markdown_code(report["actor_scope"]["api"])),
        "",
        "Excluded from this API's results: {}.".format(
            ", ".join(report["actor_scope"]["exclusions"])
        ),
        "",
        "## Units and bounds",
        "",
        "- Position and bounds: {}.".format(report["units"]["location"]),
        "- Rotation: {}.".format(report["units"]["rotation"]),
        "- Scale: {}.".format(report["units"]["scale"]),
        "- Bounds method: {} (`only_colliding_components=false`, "
        "`include_from_child_actors=false`).".format(report["bounds_method"]["api"]),
        "- Bounds are an axis-aligned box, not an exact shape, walkable footprint, or level boundary.",
        "- Bounds with zero size on every axis are reported as unavailable (`null`).",
        "",
        "## Actors",
        "",
        "| Object path | Label | Class path | Tags | Location (cm) | Rotation (degrees) | Scale | Bounds (cm) |",
        "|---|---|---|---|---|---|---|---|",
    ]

    for actor in report["actors"]:
        transform = actor["transform"]
        row = [
            _markdown_code(actor["object_path"]),
            "unavailable" if actor["label"] is None else _markdown_cell(actor["label"]),
            _markdown_code(actor["class_path"]),
            _markdown_cell(", ".join(actor["tags"]) or "none"),
            _format_vector(transform["location_cm"]),
            _format_vector(transform["rotation_degrees"]),
            _format_vector(transform["scale"]),
            "unavailable" if actor["bounds_cm"] is None else _format_vector(actor["bounds_cm"]),
        ]
        lines.append("| {} |".format(" | ".join(row)))

    lines.extend(
        [
            "",
            "This snapshot does not establish that the map is clean, complete, playable, or correctly framed.",
            "",
        ]
    )
    return "\n".join(lines)


def _write_staged_file(directory, stem, suffix, contents, temporary_paths):
    path = os.path.join(
        directory, "{}.{}.{}.tmp".format(stem, uuid.uuid4().hex, suffix)
    )
    with open(path, "xb") as output_file:
        temporary_paths.append(path)
        output_file.write(contents)
        output_file.flush()
    return path


def _publish_non_overwriting(temporary_path, final_path, published_paths):
    if os.path.lexists(final_path):
        raise FileExistsError("Refusing to overwrite existing report path {!r}".format(final_path))
    # Unreal Editor scripting for this project runs on Windows, where os.rename refuses an
    # existing destination instead of replacing it.
    os.rename(temporary_path, final_path)
    published_paths.append(final_path)


def _sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as input_file:
        for chunk in iter(lambda: input_file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _verify_published_pair(paths, expected_manifest):
    with open(paths["manifest"], "r", encoding="utf-8") as manifest_file:
        published_manifest = json.load(manifest_file)
    if published_manifest != expected_manifest:
        raise SurveyError("Published completion manifest does not match the staged manifest.")
    if _sha256_file(paths["markdown"]) != expected_manifest["files"]["markdown"]["sha256"]:
        raise SurveyError("Published Markdown report hash does not match its manifest.")
    if _sha256_file(paths["json"]) != expected_manifest["files"]["json"]["sha256"]:
        raise SurveyError("Published JSON report hash does not match its manifest.")


def _cleanup(paths_to_remove, reservation_path):
    """Remove only paths created by this run and return all cleanup failures."""
    failures = []
    for path in paths_to_remove:
        try:
            if os.path.lexists(path):
                os.remove(path)
        except OSError as error:
            failures.append((path, error))

    if reservation_path is not None:
        try:
            if os.path.lexists(reservation_path):
                os.remove(reservation_path)
        except OSError as error:
            failures.append((reservation_path, error))
    return failures


def _print_cleanup_failures(failures):
    for path, error in failures:
        print("[SurveyLevel] Cleanup failed; remaining path {!r}: {}".format(path, error))


def _write_reports(saved_dir, report, timestamp):
    directory = _output_directory(saved_dir, report["map_package"])
    os.makedirs(directory, exist_ok=True)

    paths = _reserve_stem(directory, timestamp)
    reservation_path = paths["reservation"]
    temporary_paths = []
    published_paths = []
    try:
        json_contents = _json_bytes(report)
        markdown_contents = _markdown_report(report).encode("utf-8")
        markdown_hash = hashlib.sha256(markdown_contents).hexdigest()
        json_hash = hashlib.sha256(json_contents).hexdigest()
        manifest = {
            "schema_version": SCHEMA_VERSION,
            "files": {
                "markdown": {
                    "name": os.path.basename(paths["markdown"]),
                    "sha256": markdown_hash,
                },
                "json": {
                    "name": os.path.basename(paths["json"]),
                    "sha256": json_hash,
                },
            },
        }
        manifest_contents = _json_bytes(manifest)

        staged_markdown = _write_staged_file(
            directory, os.path.basename(paths["markdown"]), "md", markdown_contents,
            temporary_paths
        )
        staged_json = _write_staged_file(
            directory, os.path.basename(paths["json"]), "json", json_contents,
            temporary_paths
        )
        staged_manifest = _write_staged_file(
            directory, os.path.basename(paths["manifest"]), "manifest",
            manifest_contents, temporary_paths
        )

        _publish_non_overwriting(staged_markdown, paths["markdown"], published_paths)
        temporary_paths.remove(staged_markdown)
        _publish_non_overwriting(staged_json, paths["json"], published_paths)
        temporary_paths.remove(staged_json)
        _publish_non_overwriting(staged_manifest, paths["manifest"], published_paths)
        temporary_paths.remove(staged_manifest)
        _verify_published_pair(paths, manifest)

        try:
            os.remove(reservation_path)
        except OSError as error:
            raise SurveyError(
                "Could not release reservation {!r}: {}".format(reservation_path, error)
            )
        reservation_path = None
        return paths
    except Exception as error:
        # Remove the completion marker first so any cleanup failure leaves an incomplete pair.
        cleanup_order = []
        if paths["manifest"] in published_paths:
            cleanup_order.append(paths["manifest"])
        cleanup_order.extend(
            path for path in published_paths if path not in cleanup_order
        )
        cleanup_order.extend(temporary_paths)
        cleanup_failures = _cleanup(cleanup_order, reservation_path)
        _print_cleanup_failures(cleanup_failures)
        detail = "{}: {}".format(type(error).__name__, error)
        if cleanup_failures:
            detail += "; cleanup failed for {} path(s)".format(len(cleanup_failures))
        raise SurveyError("Report publication failed: {}".format(detail))


def _gather_report(actor_subsystem, package_path, generated_at):
    dirty_before = _read_dirty_status(package_path)
    try:
        actors = actor_subsystem.get_all_level_actors()
    except Exception as error:
        raise SurveyError("Could not enumerate actors in the open Editor world: {}".format(error))
    if actors is None:
        raise SurveyError("EditorActorSubsystem returned no actor list.")

    actor_records = [_read_actor(actor, index) for index, actor in enumerate(actors)]
    actor_records.sort(key=lambda actor_record: actor_record["object_path"])
    dirty_after = _read_dirty_status(package_path)
    dirty_changed = (
        None if dirty_before is None or dirty_after is None else dirty_before != dirty_after
    )
    return {
        "schema_version": SCHEMA_VERSION,
        "generated_at": generated_at,
        "map_package": package_path,
        "actor_scope": ACTOR_SCOPE,
        "units": UNITS,
        "bounds_method": BOUNDS_METHOD,
        "dirty_map_status": {
            "before": dirty_before,
            "after": dirty_after,
            "changed": dirty_changed,
        },
        "actors": actor_records,
    }


def run_survey():
    """Survey the current Editor world and publish a complete report pair."""
    try:
        editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        if editor_subsystem is None:
            raise SurveyError("UnrealEditorSubsystem is unavailable.")

        game_world = editor_subsystem.get_game_world()
        if game_world is not None:
            raise SurveyError("Refusing to run while a PIE or Simulate world is active.")

        world = editor_subsystem.get_editor_world()
        if world is None:
            raise SurveyError("There is no currently open Editor world.")

        package_path = _read_map_package_path(world)
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        if actor_subsystem is None:
            raise SurveyError("EditorActorSubsystem is unavailable.")

        timestamp = datetime.now().astimezone()
        report = _gather_report(
            actor_subsystem,
            package_path,
            timestamp.isoformat(timespec="microseconds"),
        )
        saved_dir = unreal.Paths.project_saved_dir()
        if not isinstance(saved_dir, str) or not os.path.isabs(saved_dir):
            raise SurveyError(
                "Unreal Paths did not return an absolute project Saved directory."
            )
        paths = _write_reports(saved_dir, report, timestamp)
        print(
            "[SurveyLevel] Complete: {} actors surveyed. Markdown: {} | JSON: {} | "
            "manifest: {}".format(
                len(report["actors"]), paths["markdown"], paths["json"], paths["manifest"]
            )
        )
        return True
    except Exception as error:
        print("[SurveyLevel] Failed: {}: {}".format(type(error).__name__, error))
        return False


if __name__ == "__main__":
    run_survey()
