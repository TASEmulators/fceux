import hashlib
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

from run_rom_suite import CaseFailure, classify, load_cases, run_case, suite_summary_markdown
from run_scenario import NTSC_FRAME_SECONDS, Client, TestFailure, stop_fceux, test_environment


class RunnerTests(unittest.TestCase):
    def test_gui_uses_desktop_environment(self):
        with patch.dict(os.environ, {"QT_QPA_PLATFORM": "offscreen", "DISPLAY": ":0"}, clear=True):
            env = test_environment(gui=True)
            self.assertNotIn("QT_QPA_PLATFORM", env)
            self.assertEqual(env["DISPLAY"], ":0")

        with patch.dict(os.environ, {"QT_QPA_PLATFORM": "xcb"}, clear=True):
            self.assertEqual(test_environment(gui=True)["QT_QPA_PLATFORM"], "xcb")
            self.assertEqual(test_environment(gui=False)["QT_QPA_PLATFORM"], "offscreen")

    def test_case_selection_does_not_require_unselected_roms(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            data = b"runner test fixture"
            (root / "selected.bin").write_bytes(data)
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({
                "version": 1,
                "cases": [
                    {"name": "selected", "rom": "selected.bin",
                     "sha256": hashlib.sha256(data).hexdigest()},
                    {"name": "missing", "rom": "missing.bin", "sha256": "0" * 64},
                ],
            }), encoding="utf-8")
            cases = load_cases(manifest, root, ["selected"])
            self.assertEqual([case["name"] for case, _ in cases], ["selected"])
            with self.assertRaisesRegex(TestFailure, "Unknown ROM case"):
                load_cases(manifest, root, ["unknown"])

    def test_failure_diagnostics_are_not_cut_off_at_256_bytes(self):
        diagnostic = b"test output\n" * 30 + b"specific failure at end"
        client = Mock()
        client.memory.side_effect = lambda address, length: (
            bytes((2, 0xDE, 0xB0, 0x61)) if address == 0x6000
            else (diagnostic + b"\0")[:length]
        )
        with patch("builtins.print"):
            with self.assertRaisesRegex(CaseFailure, "specific failure at end"):
                run_case(client, {"name": "diagnostic"}, Path("fixture.bin"))

    def test_realtime_advance_paces_single_frames(self):
        client = Client.__new__(Client)
        client.realtime = True
        client.next_frame_time = None
        client.call = Mock()
        clock = [100.0]
        sleeps = []

        def sleep(seconds):
            sleeps.append(seconds)
            clock[0] += seconds

        with patch("run_scenario.time.monotonic", side_effect=lambda: clock[0]), \
                patch("run_scenario.time.sleep", side_effect=sleep):
            client.advance(3, 500)

        self.assertEqual(client.call.call_count, 3)
        client.call.assert_called_with("emu.frameAdvance", {"frames": 1, "timeoutMs": 500})
        self.assertEqual(len(sleeps), 2)
        self.assertAlmostEqual(sum(sleeps), 2 * NTSC_FRAME_SECONDS)

    def test_known_failures_only_hide_the_recorded_outcome(self):
        known = {"name": "k", "known_failure": {"outcome": "status 0x02", "reason": "APU reset timing"}}
        self.assertEqual(classify({"name": "p"}, None)[0], "PASS")
        self.assertEqual(classify({"name": "f"}, "status 0x01")[0], "FAIL")
        self.assertEqual(classify(known, "status 0x02"), ("KNOWN", "APU reset timing"))
        verdict, note = classify(known, "status 0x03")
        self.assertEqual(verdict, "FAIL")
        self.assertIn("expected status 0x02, got status 0x03", note)
        verdict, note = classify(known, None)
        self.assertEqual(verdict, "FAIL")
        self.assertIn("now passes", note)

    def test_job_summary_lists_failures_first_and_marks_unrun_cases(self):
        cases = [({"name": name}, None) for name in ("ok", "known", "bad", "unrun")]
        results = [
            {"name": "ok", "verdict": "PASS", "seconds": 1.0, "detail": "60 frames"},
            {"name": "known", "verdict": "KNOWN", "seconds": 1.0, "detail": "status 0x02: a|b"},
            {"name": "bad", "verdict": "FAIL", "seconds": 2.5, "detail": "status 0x03"},
        ]
        markdown = suite_summary_markdown(cases, results, TestFailure("FCEUX closed the connection"))
        self.assertIn("### NES ROM suite: FAIL", markdown)
        self.assertIn("1 passed, 1 known failures, 1 failed, 1 not run, of 4 cases.", markdown)
        self.assertIn("Suite stopped early: FCEUX closed the connection", markdown)
        self.assertIn("status 0x02: a\\|b", markdown)
        rows = [line for line in markdown.splitlines() if line.startswith("| `")]
        self.assertEqual([row.split("`")[1] for row in rows], ["bad", "unrun", "known", "ok"])

        passing = suite_summary_markdown(cases[:2], results[:2])
        self.assertIn("### NES ROM suite: PASS", passing)

    def test_shutdown_uses_app_quit_before_signals(self):
        process = Mock()
        client = Mock()
        self.assertTrue(stop_fceux(process, client))
        client.call.assert_called_once_with("app.quit")
        process.terminate.assert_not_called()

        process = Mock()
        process.wait.side_effect = [subprocess.TimeoutExpired("fceux", 10), subprocess.TimeoutExpired("fceux", 10), 0]
        client = Mock()
        client.call.side_effect = TestFailure("FCEUX closed the connection during app.quit")
        self.assertFalse(stop_fceux(process, client))
        process.terminate.assert_called_once_with()
        process.kill.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
