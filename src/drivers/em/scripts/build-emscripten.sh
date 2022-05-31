#!/bin/bash
set -eEuo pipefail
cd `dirname "$0"` && SCRIPT_DIR=`pwd -P`

PACKAGE_DIR="$SCRIPT_DIR/.."
BUILD_DIR="$PACKAGE_DIR/build"

# Handle cleanup option.
if [ "${1-}" == "--clean" ] ; then
  rm -rf "$BUILD_DIR" "$PACKAGE_DIR/dist"
  exit 0
fi

# Sanity checks before build.
hash python3 2>/dev/null || { echo >&2 "ERROR: python not found. Please install python."; exit 1; }
[ -z ${EMSDK+x} ] && { echo >&2 "ERROR: emscripten env not set. Please run 'source emsdk_env.sh'."; exit 1; }
if [ -z ${EMSCRIPTEN_ROOT+x} ] ; then
  DEFAULT_ROOT="$EMSDK/upstream/emscripten"
  [ -e "$DEFAULT_ROOT/emcc" ] || echo "WARNING: Failed to generate valid env EMSCRIPTEN_ROOT=$DEFAULT_ROOT."
  export EMSCRIPTEN_ROOT=$DEFAULT_ROOT
fi
echo "Emscripten root:$EMSCRIPTEN_ROOT"

# Generate opcode table.
"$SCRIPT_DIR/gen-ops.py" -f > "$PACKAGE_DIR/opsfuncs.inc"

# Build.
BUILD_TYPE=`[ "${1-}" == "--debug" ] && echo Debug || echo Release`
if [[ ! -d "$BUILD_DIR" ]] ; then
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR" && emcmake cmake ../../../.. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
fi
cd "$BUILD_DIR" && make -j`getconf _NPROCESSORS_ONLN`

# Copy build results to dist.
mkdir -p "$PACKAGE_DIR/dist"
cp "$BUILD_DIR/src/fceux".{js,wasm} "$PACKAGE_DIR/dist"
