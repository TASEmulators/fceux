# em-fceux

https://bitbucket.org/tsone/em-fceux/

Web port of the [FCEUX](https://github.com/TASVideos/fceux/) Nintendo
Entertainment System/Famicom (NES/FC) emulator. Powered by
[Emscripten](https://emscripten.org/).

Try it at https://tsone.kapsi.fi/em-fceux/.

## Overview

em-fceux enables the core features of the FCEUX emulator on the web browsers.
The emulation runs at 60 FPS on modern computers, has low input latency and good
audio quality. It also supports save states and battery-backed save RAM.

It also incorporates a new shader-based NTSC composite video emulation
technique.

## Features

There are some modifications in FCEUX to make the code suitable for Emscripten.
Primary addition is WebGL renderer which enables the use of shaders.

Supported FCEUX features:

- Both NTSC and PAL system emulation.
- All mappers supported by FCEUX.
- Save states and battery-backed SRAM.
- Speed throttling.
- Support for two game controllers.
- Zapper support.
- Support for NES, ZIP and NSF file formats.

_Unsupported_ FCEUX features:

- New PPU emulation (old PPU emulation used for performance).
- FDS disk system.
- VS system.
- Special peripherals (Family Keyboard, Mahjong controller, etc.)
- Screenshots and movie recording.
- Cheats, debug, TAS and Lua scripting features.

New features:

- NTSC composite video emulation.
- CRT TV emulation.

## Examples and API

For basic usage, see
[em-fceux-example-minimal](https://bitbucket.org/tsone/em-fceux-example-minimal/).
For advanced usage (a full web emulator), see
[em-fceux-site](https://bitbucket.org/tsone/em-fceux-site/).

API documentation can be found in
[API.md](https://bitbucket.org/tsone/em-fceux/src/master/API.md).

## Build

Setup:

1. [Install and activate Emscripten 2.0.6](https://emscripten.org/docs/getting_started/downloads.html).
2. Have python 2.7.x.
3. Install [scons](https://scons.org/pages/download.html).
4. Run `source emsdk_env.sh` to setup the Emscripten env.
   - Note, this also sets `npm` to env.
5. Run `npm install`.

Build for debug with `npm run build:debug` and for release with `npm run build`
(results will be in `dist/`).

### Building Shaders

You only need to rebuild the shaders when the shader sources in
`src/drivers/em/assets/shaders/` are changed. Do this with
`npm run build:shaders`. This will download and build `glsl-optimizer` from
source, and requires cmake and a C++ compiler.

Note, `npm run build:shaders` must be run before `npm run build` as the shaders
are embedded in the output binary.

## Browser Requirements

- [WebAssembly](https://webassembly.org/).
- [WebGL](https://www.khronos.org/webgl/).
- [Web Audio API](https://www.w3.org/TR/webaudio/).

## NTSC Composite Video Emulation Details

The NTSC composite video emulation emulates a signal path of an early 1990s TV.
The separation to luminance (Y) and chroma (IQ) uses a
[two-line comb filter](http://www.cockam.com/vidcomb.htm#TwoLine) to reduce
chroma fringing. Conversion of the separated YIQ to RGB uses a large lookup
texture in the fragment shader.

## Contact

Authored by Valtteri "tsone" Heikkilä.

Please submit bugs and feature requests in the
[em-fceux issue tracker](https://bitbucket.org/tsone/em-fceux/issues?status=new&status=open).

## License

Licensed under [GNU GPL 2](https://www.gnu.org/licenses/gpl-2.0.txt).

em-fceux is based on
[FCEUX 2.2.2 source code release](http://sourceforge.net/projects/fceultra/files/Source%20Code/2.2.2%20src/fceux-2.2.2.src.tar.gz/download).
