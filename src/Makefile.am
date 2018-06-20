AUTOMAKE_OPTIONS = subdir-objects
fceux_CPPFLAGS = -std=c++0x
fceux_LDADD =

bin_PROGRAMS	=	fceux
fceux_SOURCES = fceu.cpp asm.cpp debug.cpp file.cpp movie.cpp ppu.cpp vsuni.cpp cart.cpp drawing.cpp filter.cpp netplay.cpp sound.cpp wave.cpp cheat.cpp emufile.cpp ines.cpp nsf.cpp state.cpp x6502.cpp conddebug.cpp input.cpp oldmovie.cpp unif.cpp config.cpp fds.cpp palette.cpp video.cpp
if LUA
TMP_CPPFLAGS = $(lua51_CFLAGS)
TMP_LUA = lua-engine.cpp
endif
if LUA_BUILTIN
TMP_LUA = lua-engine.cpp
endif

include lua/Makefile.am.inc


include utils/Makefile.am.inc
include boards/Makefile.am.inc
include drivers/common/Makefile.am.inc
include drivers/videolog/Makefile.am.inc
include input/Makefile.am.inc
include fir/Makefile.am.inc

fceux_CPPFLAGS += $(TMP_CPPFLAGS)
fceux_SOURCES += $(TMP_LUA)

#if NATIVEWIN32
#include drivers/win/Makefile.am.inc
#else
include drivers/sdl/Makefile.am.inc
#endif
# TODO

# TODO

fceux_SOURCES    +=      $(fceud_SOURCES)

