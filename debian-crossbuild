#!/bin/sh
if [ -f /usr/bin/i586-mingw32msvc-windres ]; then HOST=i586-mingw32msvc
else HOST=i586-mingw32
fi
PLATFORM=win32 CC=${HOST}-gcc CXX=${HOST}-g++ WRC=${HOST}-windres WINDRES=${HOST}-windres scons $@
