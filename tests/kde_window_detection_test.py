#!/usr/bin/env python3
import os
from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
from pathlib import Path
import unittest
from unittest.mock import patch


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = PROJECT_ROOT / "scripts" / "mark-shot-window-detection-kde"
LOADER = SourceFileLoader("mark_shot_window_detection_kde", str(SCRIPT_PATH))
SPEC = spec_from_loader(LOADER.name, LOADER)
KDE = module_from_spec(SPEC)
LOADER.exec_module(KDE)


class KdeWindowDetectionTest(unittest.TestCase):
    def test_fully_occluded_window_is_filtered(self):
        payload = {
            "windows": [
                {"x": 0, "y": 0, "width": 100, "height": 100, "title": "bottom"},
                {"x": 0, "y": 0, "width": 50, "height": 100, "title": "left"},
                {"x": 50, "y": 0, "width": 50, "height": 100, "title": "right"},
            ]
        }

        with patch.dict(os.environ, {}, clear=True):
            windows = KDE.selected_windows(payload)

        self.assertEqual([window["title"] for window in windows], ["left", "right"])

    def test_partially_visible_window_is_retained(self):
        payload = {
            "windows": [
                {"x": 0, "y": 0, "width": 100, "height": 100, "title": "bottom"},
                {"x": 0, "y": 0, "width": 90, "height": 100, "title": "top"},
            ]
        }

        with patch.dict(os.environ, {}, clear=True):
            windows = KDE.selected_windows(payload)

        self.assertEqual([window["title"] for window in windows], ["bottom", "top"])

    def test_topmost_duplicate_geometry_wins_and_keeps_z_order(self):
        payload = {
            "windows": [
                {"x": 0, "y": 0, "width": 100, "height": 100, "title": "bottom", "zOrder": 0},
                {"x": 20, "y": 20, "width": 20, "height": 20, "title": "middle", "zOrder": 1},
                {"x": 0, "y": 0, "width": 100, "height": 100, "title": "top", "zOrder": 2},
            ]
        }

        with patch.dict(os.environ, {}, clear=True):
            windows = KDE.selected_windows(payload)

        self.assertEqual([(window["title"], window["zOrder"]) for window in windows], [("top", 2)])


if __name__ == "__main__":
    unittest.main()
