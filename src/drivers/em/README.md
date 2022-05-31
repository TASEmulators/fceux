# em-fceux

https://github.com/tsone/fceux/tree/master/src/drivers/em

Web port of the [FCEUX](https://github.com/TASVideos/fceux/) Nintendo
Entertainment System/Famicom (NES/FC) emulator. Powered by
[Emscripten](https://emscripten.org/) and WebAssembly.

Try it at https://tsone.kapsi.fi/em-fceux/.

## Overview

em-fceux enables many core FCEUX features on the web browsers through a
convenient API and it is available on npm. It achieves high emulation
performance by building the FCEUX source code to WebAssembly. It additionally
incorporates NTSC composite video emulation shaders.

## Features

There are some modifications in FCEUX to make the code suitable for Emscripten.
Primary addition is WebGL renderer which enables the use of shaders.

Supported FCEUX features:

- All mappers supported by FCEUX.
- NTSC, PAL and Dendy system emulation.
- Save states and battery-backed SRAM.
- Speed throttling.
- Support for two game controllers.
- Zapper support.
- Support for NES, ZIP and NSF file formats.

_Unsupported_ FCEUX features:

- New PPU emulation (old PPU is used for its performance).
- FDS disk system.
- VS system.
- Special peripherals (Family Keyboard, Mahjong controller, etc.)
- Screenshots and movie recording.
- Cheats, debugging, TAS and Lua scripting features.

New features:

- NTSC composite and CRT TV video shaders.

## Examples and API

For basic usage, see
[em-fceux-example-minimal](https://bitbucket.org/tsone/em-fceux-example-minimal/).
For advanced usage (a full web emulator), see
[em-fceux-site](https://bitbucket.org/tsone/em-fceux-site/).

API documentation can be found in
[API.md](https://github.com/tsone/fceux/tree/master/src/drivers/em/API.md).

## Build

Setup:

1. Have python 3.x.
2. Install cmake.
3. [Install and activate Emscripten 3.1.12](https://emscripten.org/docs/getting_started/downloads.html).
4. Run `source emsdk_env.sh` to add Emscripten in your shell env.
   - This also adds `npm` in the env.
5. Run `npm install`.

Then build for debug with `npm run build:debug` and for release with
`npm run build`. The build results will are under `dist/`.

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
