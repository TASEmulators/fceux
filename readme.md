# fceux [![Build status](https://ci.appveyor.com/api/projects/status/github/TASEmulators/fceux?branch=master&svg=true)](https://ci.appveyor.com/project/zeromus/fceux) [![macOS build](https://github.com/TASEmulators/fceux/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/TASEmulators/fceux/actions/workflows/macos.yml) [![Linux build](https://github.com/TASEmulators/fceux/actions/workflows/linux.yml/badge.svg?branch=master)](https://github.com/TASEmulators/fceux/actions/workflows/linux.yml) [![Win64 Qt build](https://github.com/TASEmulators/fceux/actions/workflows/windows-qt.yml/badge.svg?branch=master)](https://github.com/TASEmulators/fceux/actions/workflows/windows-qt.yml)

An open source NES Emulator for Windows and Unix that features solid emulation accuracy and state of the art tools for power users. For some reason casual gamers use it too.

## Builds and Releases

2.7.0 is the most recent release but most people are using the autobuilds.

Interim builds:
* Win32: [fceux-win32.zip](https://github.com/TASEmulators/fceux/releases/download/interim-build/fceux-win32.zip)
* Win64: [fceux-win64.zip](https://github.com/TASEmulators/fceux/releases/download/interim-build/fceux-win64.zip)
* Win64 Qt/SDL: [fceux-win64-QtSDL.zip](https://github.com/TASEmulators/fceux/releases/download/interim-build/fceux-win64-QtSDL.zip)
* Ubuntu: [fceux-ubuntu-x64.deb](https://github.com/TASEmulators/fceux/releases/download/interim-build/fceux-ubuntu-x64.deb)
* macOS (Apple Silicon): [fceux-Darwin.dmg](https://github.com/TASEmulators/fceux/releases/download/interim-build/fceux-Darwin.dmg)
* Status: [AppVeyor](https://ci.appveyor.com/project/zeromus/fceux/) · [macOS GitHub Actions](https://github.com/TASEmulators/fceux/actions/workflows/macos.yml) · [Linux GitHub Actions](https://github.com/TASEmulators/fceux/actions/workflows/linux.yml) · [Win64 Qt GitHub Actions](https://github.com/TASEmulators/fceux/actions/workflows/windows-qt.yml)

But you might like Mesen more: https://github.com/SourMesen/Mesen2

## Qt frontend documentation

See the [Qt README](src/drivers/Qt/readme.md) for Qt-specific documentation,
including [configuration and storage](src/drivers/Qt/readme.md#configuration-and-storage).
Future Qt-specific documentation belongs there; workspace setup remains below.

## Windows Qt development (Visual Studio 2022)

Install **Visual Studio 2022** with **Desktop development with C++**, including
the MSVC v143 x64 tools, a Windows SDK, and **C++ CMake tools for Windows**,
plus [Git for Windows](https://gitforwindows.org/).

Run the following commands in PowerShell to set up a **Visual Studio 2022
project workspace for the FCEUX Qt Win64 GUI**. The setup script downloads the required
dependencies and uses CMake to generate the VS2022 solution:

```powershell
git clone https://github.com/TASEmulators/fceux.git
cd fceux
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\setup-qt-windows.ps1
```

By default, the script installs project dependencies under `build\qt-deps` and
generates the workspace under `build\qt-vs2022`. It does not install dependencies
system-wide or permanently change PATH; Visual Studio and Git are prerequisites
installed separately. Dependency tools may also use temporary files and
per-user caches outside `build`.

Open **`build\qt-vs2022\fceux.sln` in Visual Studio 2022**, select **Debug / x64**,
and press **F5** to build and debug.

For release-like performance with breakpoint debugging, select **RelWithDebInfo / x64**
and press **F5**. This enables optimization and debug symbols, but some variables
may be optimized away and stepping or breakpoints may not match source lines
exactly. Use **Debug / x64** for predictable stepping and variable inspection.

After the first build of a configuration, normal builds are incremental: only
changed files and affected dependents are recompiled. Small edits should build
very quickly compared with the initial build. Use **Build** or **F5**, not
**Rebuild**, to preserve incremental builds; no setup rerun is needed for code edits.

The `build` folder is generated locally and excluded from version control.
Do not add its files to the source project or commit them to the repository.
You can delete the entire folder at any time and rerun the setup command above
to download dependencies again and regenerate the solution and workspace.
Keep source changes outside `build`; anything stored there will be lost on deletion.

To delete the entire `build` folder and set up a fresh default workspace, close
Visual Studio and run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\setup-qt-windows.ps1 -Clean
```

**This deletes everything in `build`, including downloads and any user files.**
Add `-WhatIf` to preview without deleting or installing anything. `-Clean` cannot
be combined with custom build, dependency, Qt, or Vulkan SDK paths.

- **Disk space:** approximately **6 GB** for dependencies and one Debug build;
  allow **10-15 GB free** for installation and additional configurations,
  excluding Visual Studio itself.
- **Initial Workspace Setup Time:** budget approximately **15-30 minutes** for dependency
  setup and the first build with Visual Studio already installed. This is an
  estimate; download speed and CPU performance can make it take longer.
