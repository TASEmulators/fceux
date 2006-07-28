file_list = Split("""
cart.cpp
cheat.cpp
crc32.cpp
config.cpp
debug.cpp
endian.cpp
fceu.cpp
fds.cpp
file.cpp 
filter.cpp 
general.cpp
ines.cpp
input.cpp
md5.cpp
memory.cpp
netplay.cpp
nsf.cpp
palette.cpp
ppu.cpp
sound.cpp
state.cpp
unif.cpp
video.cpp
vsuni.cpp
wave.cpp
x6502.cpp
movie.cpp
unzip.c""")

Export('file_list')

SConscript(Split("""
boards/SConscript
input/SConscript
fir/SConscript
mappers/SConscript
palettes/SConscript
drivers/common/SConscript
drivers/pc/SConscript
"""))

Import('file_list')

# XXX path separator fixed right now
opts = Options()
opts.Add('PSS_STYLE', 'Path separator style', 1)
opts.Add('LSB_FIRST', 'Least significant byte first?', 1)

env = Environment(options = opts,
                  CPPDEFINES={'PSS_STYLE' : '${PSS_STYLE}',
                              'LSB_FIRST' : '${LSB_FIRST}'})

# use sdl-config to get the cflags and libpath
import os;
sdl_cflags = os.popen("sdl-config --cflags");
CCFLAGS = sdl_cflags.read();
CCFLAGS = CCFLAGS.rstrip(os.linesep);
sdl_cflags.close();

sdl_libflags = os.popen("sdl-config --libs");
LINKFLAGS = sdl_libflags.read();
LINKFLAGS = " -lz " + LINKFLAGS.rstrip(os.linesep);
sdl_libflags.close();

env.Program('fceu', file_list, CCFLAGS=CCFLAGS, LIBPATH=LINKFLAGS)
