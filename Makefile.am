AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS	=	fceu
fceu_SOURCES 	= 	fceustr.cpp cart.cpp cheat.cpp crc32.cpp config.cpp debug.cpp endian.cpp fceu.cpp fds.cpp file.cpp filter.cpp general.cpp ines.cpppp input.cpp md5.cpp memory.cpp netplay.cpp nsf.cpp palette.cpp ppu.cpp sound.cpp state.cpp unif.cpp video.cpp vsuni.cpp wave.cpp x6502.cpp movie.cpp
# soundexp.cpp

include $(srcdir)/boards/Makefile.am.inc
include $(srcdir)/input/Makefile.am.inc
include $(srcdir)/mappers/Makefile.am.inc
# include mbshare/Makefile.am.inc
include $(srcdir)/drivers/common/Makefile.am.inc

fceu_SOURCES	+=	$(sexyal_SOURCES)

if NATIVEWIN32
#include drivers/win/Makefile.am.inc
else
include drivers/pc/Makefile.am.inc
endif

fceu_SOURCES    +=      $(fceud_SOURCES)

DEFAULT_INCLUDES        =        
