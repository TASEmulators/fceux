#!/usr/bin/env python3
import argparse
import base64
import hashlib
import json
import os
import queue
import re
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path


SKIP = 77
READY_PATTERN = re.compile(r"FCEUX_TEST_SERVER_READY (\{.*\})")
SIGNATURE = bytes((0xDE, 0xB0, 0x61))
NTSC_FRAME_SECONDS = 1 / 60.0988


class TestFailure(RuntimeError):
    pass


def test_environment(gui):
    env = os.environ.copy()
    if gui:
        if env.get("QT_QPA_PLATFORM") in ("offscreen", "minimal"):
            env.pop("QT_QPA_PLATFORM")
    else:
        env["QT_QPA_PLATFORM"] = "offscreen"
    return env


def append_step_summary(markdown):
    """Append Markdown to the GitHub Actions job summary when running under Actions."""
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if path:
        with open(path, "a", encoding="utf-8") as summary:
            summary.write(markdown.rstrip("\n") + "\n\n")


def markdown_cell(text):
    return " ".join(str(text).split()).replace("|", "\\|")


def report_progress(name, frames, maximum, status):
    print(
        f"RUNNING: {name}: {frames}/{maximum} frames; status=0x{status:02X}",
        flush=True,
    )


def collect_output(stream, lines):
    for line in iter(stream.readline, ""):
        lines.put(line.rstrip())
    stream.close()


def read_server_ready(process, lines, timeout):
    deadline = time.monotonic() + timeout
    captured = []
    while time.monotonic() < deadline:
        try:
            line = lines.get(timeout=max(0.01, deadline - time.monotonic()))
        except queue.Empty:
            break
        captured.append(line)
        match = READY_PATTERN.search(line)
        if match:
            return json.loads(match.group(1))
        if process.poll() is not None:
            break
    raise TestFailure("FCEUX did not start its test server:\n" + "\n".join(captured))


class Client:
    def __init__(self, port, token):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=30)
        self.socket.settimeout(120)
        self.reader = self.socket.makefile("rb")
        self.token = token
        self.next_id = 1
        self.realtime = False
        self.next_frame_time = None

    def close(self):
        self.reader.close()
        self.socket.close()

    def call(self, method, params=None):
        request_id = self.next_id
        self.next_id += 1
        request = {
            "id": request_id,
            "token": self.token,
            "method": method,
            "params": params or {},
        }
        self.socket.sendall((json.dumps(request) + "\n").encode())
        line = self.reader.readline()
        if not line:
            raise TestFailure(f"FCEUX closed the connection during {method}")
        response = json.loads(line)
        if response.get("id") != request_id:
            raise TestFailure(f"Unexpected response id during {method}: {response!r}")
        if "error" in response:
            raise TestFailure(f"{method} failed: {response['error'].get('message', response['error'])}")
        return response["result"]

    def advance(self, frames, timeout_ms=None):
        params = {} if timeout_ms is None else {"timeoutMs": timeout_ms}
        if not self.realtime:
            return self.call("emu.frameAdvance", {"frames": frames, **params})

        # Pace single frames at the NTSC rate so the run can be watched.
        for _ in range(frames):
            now = time.monotonic()
            if self.next_frame_time is None or now - self.next_frame_time > 0.25:
                self.next_frame_time = now
            elif self.next_frame_time > now:
                time.sleep(self.next_frame_time - now)
            self.call("emu.frameAdvance", {"frames": 1, **params})
            self.next_frame_time += NTSC_FRAME_SECONDS
        return {"frames": frames}

    def memory(self, address, length):
        result = self.call("mem.read", {"address": address, "length": length})
        return bytes.fromhex(result["bytes"])


def stop_fceux(process, client, timeout=10):
    """Request a normal shutdown through app.quit, then fall back to signals."""
    if client is not None:
        try:
            client.call("app.quit")
        except (OSError, ValueError, TestFailure):
            pass
        client.close()
        try:
            process.wait(timeout=timeout)
            return True
        except subprocess.TimeoutExpired:
            pass
    process.terminate()
    try:
        process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
    return False


def test_scenario(client, rom_path, scenario, scenario_path, screenshot_dir, update_golden):
    print(f"LOAD: {scenario['name']}: {rom_path}", flush=True)
    client.call("rom.load", {"path": str(rom_path)})

    print("CHECK: gamepad set/press/release", flush=True)
    pad = client.call("pad.set", {"port": 1, "buttons": ["A"]})
    if pad["buttons"] != ["A"]:
        raise TestFailure(f"Unexpected pad state after pad.set: {pad!r}")
    pad = client.call("pad.press", {"port": 1, "buttons": ["B"]})
    if pad["buttons"] != ["A", "B"]:
        raise TestFailure(f"Unexpected pad state after pad.press: {pad!r}")
    pad = client.call("pad.release", {"port": 1, "buttons": ["A"]})
    if pad["buttons"] != ["B"]:
        raise TestFailure(f"Unexpected pad state after pad.release: {pad!r}")
    client.call("pad.set", {"port": 1, "buttons": []})

    total_frames = 0
    status_block = b""
    for _ in range(60):
        client.advance(1)
        total_frames += 1
        status_block = client.memory(0x6000, 4)
        if status_block[1:4] == SIGNATURE:
            break
    else:
        raise TestFailure("ROM did not publish the Blargg $DE $B0 $61 signature")

    status = status_block[0]
    report_progress(scenario["name"], total_frames, scenario["max_frames"], status)
    next_progress = 600
    while status >= 0x80 and total_frames < scenario["max_frames"]:
        frames = min(scenario["frame_batch"], scenario["max_frames"] - total_frames)
        client.advance(frames, scenario["frame_timeout_ms"])
        total_frames += frames
        status = client.memory(0x6000, 1)[0]
        if total_frames >= next_progress:
            report_progress(scenario["name"], total_frames, scenario["max_frames"], status)
            next_progress = total_frames + 600

    text = client.memory(0x6004, 256).split(b"\0", 1)[0].decode("ascii", errors="replace")
    if status >= 0x80:
        raise TestFailure(f"ROM did not finish in {total_frames} frames; status=0x{status:02X}; {text}")
    if status != scenario["expected_status"]:
        raise TestFailure(
            f"ROM result status 0x{status:02X}, expected 0x{scenario['expected_status']:02X}; {text}"
        )

    print("CHECK: stable frame hash and golden", flush=True)
    first_hash = client.call("frame.hash")
    second_hash = client.call("frame.hash")
    if first_hash != second_hash:
        raise TestFailure("A paused frame hash changed without advancing emulation")
    if first_hash.get("format") != "palette-index-256x240":
        raise TestFailure(f"Unexpected frame hash format: {first_hash!r}")
    frame_hash = first_hash.get("value")
    if not update_golden and frame_hash != scenario.get("frame_sha256"):
        raise TestFailure(
            f"Frame hash {frame_hash} does not match golden {scenario.get('frame_sha256')}"
        )

    print("CHECK: screenshot PNG and dimensions", flush=True)
    screenshot = client.call("frame.screenshot")
    if screenshot.get("format") != "png":
        raise TestFailure(f"Unexpected screenshot format: {screenshot!r}")
    png_data = base64.b64decode(screenshot["data"], validate=True)
    if len(png_data) < 24 or png_data[:8] != b"\x89PNG\r\n\x1a\n":
        raise TestFailure("Screenshot response is not a valid PNG")
    width, height = struct.unpack(">II", png_data[16:24])
    if (width, height) != (screenshot.get("width"), screenshot.get("height")):
        raise TestFailure("Screenshot dimensions do not match its PNG header")
    if (width, height) != (256, 240):
        raise TestFailure(f"Unexpected screenshot dimensions: {width}x{height}")
    if screenshot_dir is not None:
        screenshot_dir.mkdir(parents=True, exist_ok=True)
        (screenshot_dir / f"{scenario['name']}.png").write_bytes(png_data)
    if update_golden:
        scenario["frame_sha256"] = frame_hash
        scenario_path.write_text(json.dumps(scenario, indent=2) + "\n", encoding="utf-8")

    print("CHECK: soft reset restarts the ROM", flush=True)
    client.call("emu.reset")
    for _ in range(60):
        client.advance(1)
        reset_status = client.memory(0x6000, 4)
        if reset_status[1:4] == SIGNATURE and reset_status[0] >= 0x80:
            break
    else:
        raise TestFailure("Soft reset did not restart the Blargg test")

    return total_frames, text, frame_hash


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fceux", required=True)
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--screenshot-dir", type=Path)
    parser.add_argument("--update-golden", action="store_true")
    parser.add_argument("--gui", action="store_true", help="Show the Qt window instead of running offscreen")
    parser.add_argument("--realtime", action="store_true", help="Pace emulation at normal NTSC speed for watching")
    args = parser.parse_args()

    scenario_path = Path(args.scenario)
    scenario = json.loads(scenario_path.read_text(encoding="utf-8"))
    rom_root = os.environ.get("FCEUX_TEST_ROM_DIR")
    if not rom_root:
        print("SKIP: FCEUX_TEST_ROM_DIR is not set")
        return SKIP

    rom_path = Path(rom_root) / scenario["rom"]
    if not rom_path.is_file():
        print(f"SKIP: test ROM is missing: {rom_path}")
        return SKIP
    digest = hashlib.sha256(rom_path.read_bytes()).hexdigest()
    if digest != scenario["sha256"]:
        raise TestFailure(f"ROM checksum mismatch for {rom_path}: got {digest}")

    with tempfile.TemporaryDirectory(prefix="fceux-gui-test-") as temp_dir:
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
        summary = None
        try:
            print(f"START: FCEUX ({'visible GUI' if args.gui else 'offscreen'})", flush=True)
            server = read_server_ready(process, lines, 30)
            client = Client(server["port"], server["token"])
            client.realtime = args.realtime
            frames, output, frame_hash = test_scenario(
                client,
                rom_path,
                scenario,
                scenario_path,
                args.screenshot_dir,
                args.update_golden,
            )
            print(f"PASS: {scenario['name']} completed in {frames} frames; frame SHA-256 {frame_hash}: {output}", flush=True)
            completed = True
            summary = f"PASS: {frames} frames; {markdown_cell(output)}"
            return 0
        except Exception as error:
            summary = f"FAIL: {markdown_cell(error)}"
            raise
        finally:
            if summary is not None:
                append_step_summary(f"### GUI scenario `{scenario['name']}`\n\n{summary}")
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
