# FCEUX Qt frontend

This README is the home for Qt-specific documentation for Windows, macOS,
and Linux. Workspace setup information remains in the
[main README](../../../readme.md#windows-qt-development-visual-studio-2022).

## Pause on menu access

**Options > GUI Config > Pause On Menu Access** pauses a running game while a
main-window menu, the gamepad menu, a state-slot context menu, or a ROM file
browser is open. This includes both the standard Open ROM dialog and the
gamepad-controlled file browser. The option is on by default; existing saved
settings are preserved. Closing the last menu or browser resumes a game that
the scopes paused, but leaves an already-paused game paused. A manual pause or
resume overrides the temporary pause. Other tool-window menus are not included.

## Style presets

In **GUI Config**, select a **Preset** under **Style**:

- **Default** uses the selected base style without a custom palette or stylesheet.
- **Dark 2026** provides a VS Code-inspired dark theme.
- **Darkeum** has near-black surfaces, white text, and blue accents.
- **Integrid** has light gray surfaces, black text, and blue accents.
- **Combinear** has charcoal surfaces and gold accents.
- **Adaptic** has blue-gray gradients, red buttons, and green input text.
- **Custom** lets you load your own palette (`.qpal`) and stylesheet (`.qss`) files.

Built-in presets need no external theme files. Changes apply immediately and
your choice is remembered. Switching presets preserves your custom theme
settings and selected base style.

The window title bar's appearance is controlled by your operating system.
State Slots and State Recorder Timeline contents follow the video background
color, regardless of the selected preset.

## Configuration and storage

On Windows, macOS, and Linux, the Qt frontend checks for `fceux.cfg` before
opening the main window. Explicit environment overrides take priority:
`FCEUX_CONFIG_DIR` selects the storage directory directly, while `FCEUX_HOME`
selects its `.fceux` subdirectory. These overrides do not show a chooser.

Without an override, FCEUX checks the previously selected directory, then
existing portable and legacy configurations (the portable folder described
below and `~/.fceux`), then Qt's platform-standard application configuration locations.
Windows prefers the portable location over the legacy home directory; other
platforms prefer the legacy home directory. Existing configurations are used
in place, not migrated.

If no configuration is found, a startup dialog offers a recommended per-user
folder (Qt's `AppConfigLocation`), portable storage, or a custom folder.
The selected directory contains `fceux.cfg` and the default
save, save-state, movie, cheat, and screenshot folders. Explicitly configured
data paths remain unchanged. FCEUX checks that the location is writable and
remembers the choice in per-user Qt settings (`Application/StorageDirectory`).
Cancel exits without starting the emulator. If the selected config is later
removed and no other config is found, the chooser appears again.

Portable storage uses these layouts:

- Windows, Linux, and unbundled macOS executables: a `config` folder one level
  above the executable directory (`../config`). For example,
  `FCEUX/bin/fceux.exe` uses `FCEUX/config/fceux.cfg`.
- macOS application bundles: a `config` folder beside the `.app`, outside the
  bundle. For example, `FCEUX/FCEUX.app/Contents/MacOS/fceux` uses
  `FCEUX/config/fceux.cfg` (`../../../config` relative to the executable directory).

The portable choice is remembered as the corresponding relative path, resolved
against the executable directory rather than the working directory, so it
follows the installation when moved. Custom selections remain absolute paths.
FCEUX no longer automatically searches for `fceux.cfg` beside the executable.
A previously remembered absolute location or an explicit override there is
still honored; use `--choose-config-location` to select the new portable layout.
Existing files are not moved automatically.

Use `fceux --choose-config-location` to open the selector even when a config
already exists. This flag takes no value and can be combined with a ROM filename
and other emulator options. It overrides `FCEUX_CONFIG_DIR` and `FCEUX_HOME` for
that launch only. The selected directory is remembered, but environment
overrides still take priority on subsequent launches without the flag.
Existing files are not moved or copied: FCEUX loads the destination's config
if present, otherwise creates one using defaults and any supplied command-line
options. Cancel exits without changing the remembered directory.

Use `fceux --config-dir "path to folder"` to override the storage directory for
one launch without opening the selector or changing the remembered location.
It takes priority over environment overrides and existing config discovery.
The directory holds `fceux.cfg` and the default data subfolders; it is created
if needed. Existing files are not moved or copied. Relative paths are resolved
against the startup working directory. The per-user Qt UI settings INI stays
in its usual location. A missing or empty path, repeated `--config-dir`, or
combining it with `--choose-config-location` is an error.

## Language

English is built in. Other languages are translations (`src/translations/fceux_<locale>.ts`)
that the build compiles and embeds in the executable. When at least one translation is
available and no language has been chosen yet, a language selector is shown at startup.
The choice is stored as `Language` under `[Application]` in the Qt settings `fceux.ini`
(for example `Language=de`) and the selector is not shown again. Closing the selector
without choosing uses the system language for that launch and asks again next time.

Use `fceux --choose-language` to open the selector again. This flag takes no value and
can be combined with a ROM filename and other emulator options. Use
`scripts/updateTranslations.sh <locale>` to create or update a translation file.

Startup messages report the Qt settings location, selected storage directory,
each configuration file loaded, and the new config path on first run. These
messages are also available in the Qt message log. The main Qt UI settings are
stored in a human-readable `fceux.ini` on every platform, normally:

- Windows: `%APPDATA%\TasEmulators\fceux.ini`
- Linux: `~/.config/TasEmulators/fceux.ini` (honors `XDG_CONFIG_HOME`)
- macOS: `~/.config/TasEmulators/fceux.ini`

The file is created at startup. Edit it while FCEUX is closed so running
instances do not overwrite manual changes. Existing native Qt preferences
(Windows registry, Linux `.conf`, or macOS `.plist`) are copied once into the
INI without overwriting keys already present. The original store is left
untouched but is no longer used for UI settings after migration.
`Application/NativeSettingsMigrated` records completion of the migration.
Qt settings include the storage selection, window geometry, and other UI
preferences and remain per-user even when portable storage is selected.
Builds with RetroAchievements also report the separate per-user
`retroachievements_leaderboards.ini` results store. On non-Windows platforms,
additional configuration fragments can be loaded from the selected directory's
`cfg.d` subfolder.

## Optional GUI test server

The Qt GUI can be built with a local JSON command server for automated tests.
The feature is off by default and cannot be enabled in `PUBLIC_RELEASE` builds.
It listens only on `127.0.0.1`; each process prints a one-time authentication
token and the selected port to standard error when the server is ready.

Configure and build with:

```sh
cmake -S . -B build-gui-tests \
  -DFCEU_TEST_SERVER=ON \
  -DFCEUX_TEST_ROM_DIR=/path/to/nes-test-roms
cmake --build build-gui-tests --target fceux
ctest --test-dir build-gui-tests --output-on-failure
```

`FCEUX_TEST_ROM_DIR` may instead be set in the environment when running CTest.
ROM files are read in place and must not be copied into this repository. If the
directory is unset, the CTest scenarios are skipped. The manifest checks each
ROM's SHA-256 so a different external ROM revision is not tested accidentally.
`gui_blargg_rom_suite` runs 27 deterministic CPU, PPU, APU, and mapper tests
that report through Blargg's `$DE $B0 $61` memory protocol. Interactive,
audible-only, visual-only, and documented timing-sensitive tests are excluded
rather than given guessed pass criteria. A missing ROM or checksum mismatch
fails the suite with the affected path.

Run just the ROM suite with:

```sh
ctest --test-dir build-gui-tests --verbose -R '^gui_blargg_rom_suite$'
```

Use `--verbose` (`-V`) with CTest to see output while tests run; otherwise CTest
captures it until completion. The runner flushes each test's start, periodic
frame/status progress, pass/fail result, and running totals immediately. ROM
diagnostics and a list of failed cases are printed on failure. A failed ROM
does not stop the remaining cases from running.

### Known-failure baseline

Cases that current FCEUX is known to fail carry a `known_failure` entry in
`tests/gui/rom_suite.json`:

```json
"known_failure": {
  "outcome": "status 0x02",
  "reason": "Power/reset does not apply the implied $4017 write"
}
```

`outcome` is one of `load failed`, `no signature`, `timeout`, or
`status 0xNN` (the Blargg result code). A case that fails with exactly the
recorded outcome is reported as `KNOWN` and does not fail the suite. The suite
fails on any unexpected failure, on a known failure whose outcome changes, and
on a known failure that starts passing. After an emulator fix, remove or update
the matching `known_failure` entry in the same change so the baseline tracks
the improvement.

### Continuous integration

The `gui-tests` job in `.github/workflows/linux.yml` checks out
`christopherpow/nes-test-roms` at a pinned commit next to the FCEUX sources,
builds with `FCEU_TEST_SERVER=ON`, runs the runner unit tests in `tests/gui`,
and then runs the `gui_` CTest cases offscreen with the dummy SDL audio driver.
The ROMs are fetched at job time only and are never stored in this repository.
When updating the pinned commit, refresh the SHA-256 values in the manifests.
When `GITHUB_STEP_SUMMARY` is set, both runners append their results to the
GitHub Actions job summary: the scenario result and a table of every ROM case,
with unexpected failures and cases that did not run listed first.

To watch one test in the normal Qt window, run the Python runner directly:

```sh
FCEUX_TEST_ROM_DIR=/path/to/nes-test-roms \
python3 tests/gui/run_rom_suite.py \
  --fceux build-gui-tests/src/fceux \
  --manifest tests/gui/rom_suite.json \
  --case instr_test_v5_official --gui --realtime
```

Omit `--case` to watch the whole suite, or repeat it to select several cases.
`--gui` is also available on `run_scenario.py`. It requires a working desktop
display, uses isolated test settings, and closes the window when the run
finishes. Add `--realtime` to pace frames at the NTSC rate (about 60 fps)
so the run can be watched; without it, frames run as fast as possible.
Pacing happens between frames, so ROM results are the same in both modes.
Without `--gui`, the runners remain offscreen, including when run by CTest.

To start the server manually, pass `--test-server-port 0` to choose an
available port. The initial command set is `rom.load`, `emu.frameAdvance`,
`mem.read`, and `frame.hash`. Frame advancement runs in test mode without
real-time throttling; memory reads are CPU-bus reads, and frame hashes cover
the raw 256x240 palette-index buffer rather than renderer output. Requests are
newline-delimited JSON objects with `id`, `token`, `method`, and `params`.

The test server also supports `pad.set`, `pad.press`, and `pad.release` for
gamepad ports 1 and 2. Button names are `A`, `B`, `Select`, `Start`, `Up`,
`Down`, `Left`, and `Right`. Those states override physical input at frame
boundaries while test mode is active. `frame.screenshot` returns a base64 PNG
at 256x240, colorized with the active FCEUX palette.
`emu.reset` performs a soft reset of the loaded ROM; the suite uses it for test
ROMs that request a delayed reset between subtests.
`app.quit` replies `{"quitting": true}` and then shuts FCEUX down through the
same path as File > Quit. The runners use it at the end of every run and only
fall back to SIGTERM/SIGKILL if FCEUX has not exited within 10 seconds.

The GUI scenario stores its expected raw-frame SHA-256 alongside the ROM
checksum. To intentionally refresh that golden after validating an emulator
change, run the scenario runner with `--update-golden`. Use `--screenshot-dir`
to save the captured PNG outside the repository.
