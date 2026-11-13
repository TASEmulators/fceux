# FCEUX Qt frontend

This README is the home for Qt-specific documentation for Windows, macOS,
and Linux. Workspace setup information remains in the
[main README](../../../readme.md#windows-qt-development-visual-studio-2022).

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
