# Changelog for fceux

## [2.6.4] - 2022-05-30

### Changed

- Renamed package as "fceux" and moved to official fceux fork on github.
- (Repo https://bitbucket.org/tsone/em-fceux.git will be archived.)

## [2.0.1] - 2020-10-09

### Fixed

- Missing main in package.json for nodejs.

## [2.0.0] - 2020-10-03

### Changed

- Upgraded to emscripten 2.0.6 (caused major version bump).

## [1.0.2] - 2020-04-26

### Fixed

- iOS 13 rendering (reduce active texture units to 7).
- iOS 13 audio playback.

## [1.0.1] - 2020-03-15

### Added

- First npm package release.
- New and improved API. (See API.md.)
- Use Emscripten embind.
- Events.
- Embed data and shaders files (instead of preload).
- Use prettier.

### Removed

- Site moved to separate project (em-fceux-site).
- Input handling. (Use the new API to set controller inputs.)
- Splash screen.
- Software rendering (support WebGL only).
- IndexedDB/IDBFS and localStorage use.
- Built-in games.
- Use of window resize event.

### Changed

- Major design change as "emulator core" primarily used with npm.
- To Emscripten module (MODULARIZE=1).
- API init() creates Web Audio context.
- Migrate scons files to python3.
- Migrate Javascript to es2015 (also EM_ASM).
- Rename driver sound.cpp -> audio.cpp.
- Indentations as spaces (not in shaders though).
- Major cleanup.

### Fixed

- Web Audio context issues.
- Emscripten deprecations such as:
  - Module.canvas use.
  - Canvas element for emscripten_webgl_create_context().
- Take account window.devicePixelRatio when resizing.

## [0.5.0] - 2020-01-11

### Added

- Changelog.

### Deprecated

- asm.js site fallback (use WebAssembly only).

### Fixed

- #28 - Games not loading on Chrome.
- Web Audio context creation from user input.
- Safari audio not starting.
- Build scripts, upgrade to emscripten 1.39.4 (upstream).

## [0.4.1] - 2018-01-28

### Changed

- Migrate to WebAssembly (have asm.js site as fallback).

### Fixed

- #27 - Zapper not working in Safari on MacBook.

## [0.4.0] - 2018-01-27

### Added

- First release.
