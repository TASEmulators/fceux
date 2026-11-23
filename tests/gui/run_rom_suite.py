#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import queue
import socket
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

from run_scenario import (
    Client, SKIP, TestFailure, collect_output, read_server_ready,
    report_progress, stop_fceux, test_environment, append_step_summary, markdown_cell,
)


SIGNATURE = bytes((0xDE, 0xB0, 0x61))
FRAME_BATCH = 120
MAX_FRAMES = 6000
FRAME_TIMEOUT_MS = 30000


class CaseFailure(RuntimeError):
    def __init__(self, outcome, message):
        super().__init__(message)
        self.outcome = outcome


def classify(case, outcome):
    """Return (verdict, message) for a case outcome; outcome is None on pass."""
    known = case.get("known_failure")
    if known is None:
        return ("PASS", "") if outcome is None else ("FAIL", "")
    if outcome is None:
        return "FAIL", f"known failure now passes; remove known_failure from the manifest ({known['reason']})"
    if outcome != known["outcome"]:
        return "FAIL", f"known failure changed: expected {known['outcome']}, got {outcome}"
    return "KNOWN", known["reason"]


SUMMARY_ORDER = {"FAIL": 0, "NOT RUN": 1, "KNOWN": 2, "PASS": 3}


def suite_summary_markdown(cases, results, error=None):
    """Build the job-summary table; unexpected failures are listed first."""
    finished = {result["name"] for result in results}
    rows = list(results) + [
        {"name": case["name"], "verdict": "NOT RUN", "seconds": None, "detail": ""}
        for case, _ in cases if case["name"] not in finished
    ]
    counts = {verdict: sum(row["verdict"] == verdict for row in rows) for verdict in SUMMARY_ORDER}
    overall = "FAIL" if error is not None or counts["FAIL"] or counts["NOT RUN"] else "PASS"
    lines = [
        f"### NES ROM suite: {overall}",
        "",
        f"{counts['PASS']} passed, {counts['KNOWN']} known failures, {counts['FAIL']} failed, "
        f"{counts['NOT RUN']} not run, of {len(cases)} cases.",
    ]
    if error is not None:
        lines += ["", f"Suite stopped early: {markdown_cell(error)}"]
    lines += ["", "| Case | Result | Time | Details |", "|---|---|---|---|"]
    for row in sorted(rows, key=lambda row: SUMMARY_ORDER[row["verdict"]]):
        seconds = "" if row["seconds"] is None else f"{row['seconds']:.1f}s"
        verdict = f"**{row['verdict']}**" if row["verdict"] in ("FAIL", "NOT RUN") else row["verdict"]
        lines.append(f"| `{row['name']}` | {verdict} | {seconds} | {markdown_cell(row['detail'])} |")
    return "\n".join(lines)


def valid_known_failure(known):
    if known is None:
        return True
    return (
        isinstance(known, dict)
        and isinstance(known.get("outcome"), str) and known["outcome"]
        and isinstance(known.get("reason"), str) and known["reason"]
    )


def load_cases(manifest_path, rom_root, selected_names=None):
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("version") != 1 or not isinstance(manifest.get("cases"), list):
        raise TestFailure(f"Unsupported ROM suite manifest: {manifest_path}")

    selected = set(selected_names or [])
    unknown = selected - {case.get("name") for case in manifest["cases"]}
    if unknown:
        raise TestFailure(f"Unknown ROM case(s): {', '.join(sorted(unknown))}")

    cases = []
    for case in manifest["cases"]:
        name = case.get("name")
        if selected and name not in selected:
            continue
        relative_path = Path(case.get("rom", ""))
        digest = case.get("sha256", "")
        if (
            not isinstance(name, str)
            or not name
            or relative_path.is_absolute()
            or ".." in relative_path.parts
            or len(digest) != 64
            or not valid_known_failure(case.get("known_failure"))
        ):
            raise TestFailure(f"Invalid ROM suite entry: {case!r}")

        rom_path = rom_root / relative_path
        if not rom_path.is_file():
            raise TestFailure(f"Test ROM is missing: {rom_path}")
        actual_digest = hashlib.sha256(rom_path.read_bytes()).hexdigest()
        if actual_digest != digest:
            raise TestFailure(f"ROM checksum mismatch for {rom_path}: got {actual_digest}")
        cases.append((case, rom_path))

    if not cases:
        raise TestFailure("ROM suite manifest contains no test cases")
    return cases


def run_case(client, case, rom_path):
    try:
        client.call("rom.load", {"path": str(rom_path)})
    except TestFailure as error:
        if str(error).startswith("rom.load failed:"):
            raise CaseFailure("load failed", str(error)) from error
        raise
    total_frames = 0
    status_block = b""
    for _ in range(60):
        client.advance(1)
        total_frames += 1
        status_block = client.memory(0x6000, 4)
        if status_block[1:4] == SIGNATURE:
            break
    else:
        raise CaseFailure("no signature", "ROM did not publish the Blargg $DE $B0 $61 signature")

    status = status_block[0]
    report_progress(case["name"], total_frames, MAX_FRAMES, status)
    next_progress = 600
    while status >= 0x80 and total_frames < MAX_FRAMES:
        if status == 0x81:
            print(f"RESET: {case['name']}: requested by ROM; delaying 8 frames", flush=True)
            delay_frames = 8
            client.advance(delay_frames)
            total_frames += delay_frames
            client.call("emu.reset")
        else:
            frames = min(FRAME_BATCH, MAX_FRAMES - total_frames)
            client.advance(frames, FRAME_TIMEOUT_MS)
            total_frames += frames
        status = client.memory(0x6000, 1)[0]
        if total_frames >= next_progress:
            report_progress(case["name"], total_frames, MAX_FRAMES, status)
            next_progress = total_frames + 600

    text = client.memory(0x6004, 4096).split(b"\0", 1)[0].decode("ascii", errors="replace").strip()
    expected_status = case.get("expected_status", 0)
    if status >= 0x80:
        raise CaseFailure(
            "timeout", f"did not finish in {total_frames} frames; status=0x{status:02X}; {text}"
        )
    if status != expected_status:
        raise CaseFailure(
            f"status 0x{status:02X}", f"result status 0x{status:02X}, expected 0x{expected_status:02X}; {text}"
        )
    return total_frames, text


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fceux", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--case", action="append", help="Run only this manifest case name; may be repeated")
    parser.add_argument("--gui", action="store_true", help="Show the Qt window instead of running offscreen")
    parser.add_argument("--realtime", action="store_true", help="Pace emulation at normal NTSC speed for watching")
    args = parser.parse_args()

    rom_root_value = os.environ.get("FCEUX_TEST_ROM_DIR")
    if not rom_root_value:
        print("SKIP: FCEUX_TEST_ROM_DIR is not set")
        return SKIP

    cases = load_cases(Path(args.manifest), Path(rom_root_value), args.case)
    print(f"SUITE: {len(cases)} ROM cases; {'visible GUI' if args.gui else 'offscreen'}"
          f"{'; real time' if args.realtime else ''}", flush=True)

    with tempfile.TemporaryDirectory(prefix="fceux-gui-suite-") as temp_dir:
        temp_path = Path(temp_dir)
        env = test_environment(args.gui)
        env["XDG_CONFIG_HOME"] = str(temp_path / "xdg")
        env["APPDATA"] = str(temp_path / "appdata")
        env["HOME"] = str(temp_path / "home")
        Path(env["XDG_CONFIG_HOME"]).mkdir()
        Path(env["APPDATA"]).mkdir()
        Path(env["HOME"]).mkdir()

        process = subprocess.Popen(
            [
                args.fceux,
                "--config-dir",
                str(temp_path / "config"),
                "--test-server-port",
                "0",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            env=env,
        )
        lines = queue.Queue()
        output_thread = threading.Thread(target=collect_output, args=(process.stdout, lines), daemon=True)
        output_thread.start()
        client = None
        completed = False
        results = []
        suite_error = None
        try:
            print("START: waiting for FCEUX test server", flush=True)
            server = read_server_ready(process, lines, 30)
            client = Client(server["port"], server["token"])
            client.realtime = args.realtime
            print("READY: FCEUX test server connected", flush=True)
            failures = []
            known_failures = 0
            suite_start = time.monotonic()
            for index, (case, rom_path) in enumerate(cases, 1):
                print(f"[{index}/{len(cases)}] LOAD: {case['name']}: {rom_path}", flush=True)
                case_start = time.monotonic()
                try:
                    frames, output = run_case(client, case, rom_path)
                    outcome = None
                except CaseFailure as error:
                    outcome, output = error.outcome, str(error)
                verdict, note = classify(case, outcome)
                elapsed = time.monotonic() - case_start
                if verdict == "PASS":
                    detail = f"{frames} frames"
                    print(f"PASS: {case['name']} ({elapsed:.1f}s, {frames} frames): {output}", flush=True)
                elif verdict == "KNOWN":
                    known_failures += 1
                    detail = f"{outcome}: {note}"
                    print(f"KNOWN: {case['name']} ({elapsed:.1f}s): {outcome}; {note}", flush=True)
                else:
                    detail = note or output.split("\n", 1)[0]
                    failures.append(f"{case['name']}: {detail}")
                    print(f"FAIL: {case['name']} ({elapsed:.1f}s): {note + '; ' if note else ''}{output}", flush=True)
                results.append({"name": case["name"], "verdict": verdict, "seconds": elapsed, "detail": detail})
                print(
                    f"RESULTS: {index - len(failures) - known_failures} passed, "
                    f"{known_failures} known failures, {len(failures)} failed, "
                    f"{len(cases) - index} remaining",
                    flush=True,
                )

            print(
                f"SUMMARY: {len(cases) - len(failures) - known_failures} passed, "
                f"{known_failures} known failures, {len(failures)} failed "
                f"in {time.monotonic() - suite_start:.1f}s",
                flush=True,
            )
            if failures:
                print("FAILED CASES:\n" + "\n".join(f"  {failure}" for failure in failures), flush=True)
                raise TestFailure(f"{len(failures)} of {len(cases)} ROM cases failed")
            print(f"PASS: all {len(cases)} ROM cases matched the expected results", flush=True)
            completed = True
            return 0
        except Exception as error:
            if len(results) < len(cases):
                suite_error = error
            raise
        finally:
            append_step_summary(suite_summary_markdown(cases, results, suite_error))
            if not stop_fceux(process, client):
                print("WARNING: FCEUX did not exit through app.quit; it was terminated", file=sys.stderr)
            remaining = []
            while not lines.empty():
                remaining.append(lines.get_nowait())
            # FCEUX's log is only useful when the run failed.
            if not completed and remaining:
                print("\n".join(remaining), file=sys.stderr)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, TestFailure, socket.error) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
