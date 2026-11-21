# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-Vestige-Unreal-Exception

"""Pure-Python tests for SetUpCropWithering.py's row planning, with a stand-in unreal module."""

import importlib.util
from pathlib import Path
import sys
import types
import unittest
from unittest import mock


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "SetUpCropWithering.py"


def load_script():
    fake_unreal = types.ModuleType("unreal")
    fake_unreal.log = lambda message: None
    fake_unreal.log_error = lambda message: None
    with mock.patch.dict(sys.modules, {"unreal": fake_unreal}):
        spec = importlib.util.spec_from_file_location("set_up_crop_withering", SCRIPT_PATH)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    return module


class RowsToAddTests(unittest.TestCase):
    def setUp(self):
        self.script = load_script()
        self.wanted = {"Vestige.Feedback.B": "Bee.", "Vestige.Feedback.A": "Ay."}

    def test_adds_missing_rows_sorted_by_tag(self):
        to_add, different = self.script.rows_to_add({}, self.wanted)
        self.assertEqual(to_add, [("Vestige.Feedback.A", "Ay."), ("Vestige.Feedback.B", "Bee.")])
        self.assertEqual(different, [])

    def test_leaves_matching_rows_alone(self):
        to_add, different = self.script.rows_to_add(dict(self.wanted), self.wanted)
        self.assertEqual((to_add, different), ([], []))

    def test_keeps_a_row_changed_by_hand_and_names_it(self):
        existing = {"Vestige.Feedback.A": "Edited.", "Vestige.Feedback.B": "Bee."}
        to_add, different = self.script.rows_to_add(existing, self.wanted)
        self.assertEqual((to_add, different), ([], ["Vestige.Feedback.A"]))

    def test_the_rows_are_the_guides_tags(self):
        self.assertEqual(
            sorted(self.script.FEEDBACK_ROWS),
            ["Vestige.Feedback.Farm.ClearFirst", "Vestige.Feedback.Farm.Withered"],
        )


class TextLiteralTests(unittest.TestCase):
    def setUp(self):
        self.script = load_script()

    def test_writes_an_nsloctext_literal(self):
        self.assertEqual(
            self.script.text_literal("Ns", "Key", "It doesn't."),
            'NSLOCTEXT("Ns", "Key", "It doesn\'t.")',
        )

    def test_refuses_a_double_quote(self):
        with self.assertRaises(RuntimeError):
            self.script.text_literal("Ns", "Key", 'Say "hi".')


if __name__ == "__main__":
    unittest.main()
