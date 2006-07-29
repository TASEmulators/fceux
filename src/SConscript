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
drivers/common/SConscript
drivers/pc/SConscript
"""))
#palettes/SConscript
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
sdl_cflags_pipe = os.popen("sdl-config --cflags");
sdl_cflags = sdl_cflags_pipe.read();
sdl_cflags = sdl_cflags.rstrip(os.linesep);
sdl_cflags_pipe.close();

sdl_libpath = [];
sdl_libs = [];

sdl_libflags_pipe = os.popen("sdl-config --libs");
sdl_libflags = sdl_libflags_pipe.read();
for flag in sdl_libflags.split(' '):
    if flag.find("-L") == 0:
        sdl_libpath.append(flag.strip("-L"));
    else:
        sdl_libs.append(flag.strip("-l"));
sdl_libflags_pipe.close();

# add zlib
libs = sdl_libs;
libs.append('z');

env.Program('fceu', file_list, CCFLAGS=sdl_cflags, LIBS=libs, LIBPATH=sdl_libpath)
