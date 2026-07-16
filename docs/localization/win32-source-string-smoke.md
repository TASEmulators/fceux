# Win32 source string localization smoke record

Change: `localize-win32-cpp-strings`
Date: 2026-07-16

## Automated checks

- `powershell -ExecutionPolicy Bypass -File scripts\win32-localization-check.ps1`
  - Passed: 67 resources, 192 resource strings, and 192 source strings covered.
- `openspec validate localize-win32-cpp-strings --strict`
  - Passed.
- `powershell -ExecutionPolicy Bypass -File scripts\build-win32-v143.ps1`
  - Passed: `Release|Win32`, `PlatformToolset=v143`, output `output\fceux.exe`.
  - Final build summary: 0 warnings, 0 errors.

## Executable resource checks

- Output: `output\fceux.exe`
- Machine: x86 (`0x014C`)
- Subsystem: Windows GUI (`2`)
- SHA-256: `FC13C1DD97319A4C10280F64CAEDFC8FEF3D8FEDBB2BCA4E3C44413E9BEC8E2F`
- Verified `en-US` and `zh-CN` string resources by explicit `LANGID` lookup:
  - `IDS_LOC_OPEN_ROM_TITLE`: `Open ROM` / `打开 ROM`
  - `IDS_LOC_TAS_EDITOR_TITLE`: `TAS Editor` / `TAS Editor`
  - `IDS_LOC_START_CDLOGGER_TITLE`: `Start Code/Data Logger?` / `启动 Code/Data Logger？`

## Process smoke

The executable was copied to temporary directories with temporary `fceux.cfg` files so the existing `output\fceux.cfg` was not modified.

| Language | Main window title | Top-level menu count | First menu | Exit |
|---|---|---:|---|---:|
| `en-US` | `FCEUX 2.7.0-interim git1267c9880d55e49d452ab2747b5cf1c64a266ad3` | 6 | `&File` | 0 |
| `zh-CN` | `FCEUX 2.7.0-interim git1267c9880d55e49d452ab2747b5cf1c64a266ad3` | 6 | `文件` | 0 |

## Scope notes

- File dialog titles, filters, `OPENFILENAME` structures, and path buffers are intentionally unchanged.
- Runtime data such as file names, paths, addresses, register values, numeric values, disassembly, logs, and device names remains outside translation.
- Preserved terminology was checked by the localization gate. Examples include `FCEUX`, `NES`, `TAS`, `ROM`, `RAM`, `CPU`, `PPU`, `APU`, `Lua`, `FM2`, `FM3`, `Code/Data Logger`, `Trace Logger`, `TAS Editor`, `NES Header Editor`, `Game Genie`, `Inline Assembler`, `Hex Editor`, `Power-On`, `Soft-Reset`, `SaveRam`, and `Savestate`.

## Manual review boundary

This record verifies resource coverage, startup behavior, language selection, and terminology gates. Full human visual review of every tool window at multiple DPI settings was not performed in this automated pass.
