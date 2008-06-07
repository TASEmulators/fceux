cmake_minimum_required(VERSION 2.6)
include(CheckFunctionExists)
include(TestBigEndian)

project( fceux )

option(FCEUX_FORCE_LE   "Build for a little-endian target" OFF)
option(FCEUX_FORCE_BE   "Build for a big-endian target" OFF)
option(FCEUX_FRAMESKIP  "Build legacy frameskip code" OFF)

if(NOT WIN32)
  option(FCEUX_SDL_OPENGL "Build with OpenGL support" ON)
  find_package(SDL REQUIRED)
  find_package(ZLIB REQUIRED)
  if(FCEUX_SDL_OPENGL)
    find_package(OpenGL REQUIRED)
  endif(FCEUX_SDL_OPENGL)
endif(NOT WIN32)

set(SRC_CORE
  src/asm.cpp
  src/cart.cpp
  src/cheat.cpp
  src/conddebug.cpp
  src/config.cpp
  src/debug.cpp
  src/drawing.cpp
  src/fceu.cpp
  src/fds.cpp
  src/file.cpp
  src/filter.cpp
  src/ines.cpp
  src/input.cpp
  src/movie.cpp
  src/netplay.cpp
  src/nsf.cpp
  src/palette.cpp
  src/ppu.cpp
  src/sound.cpp
  src/state.cpp
  src/unif.cpp
  src/video.cpp
  src/vsuni.cpp
  src/wave.cpp
  src/x6502.cpp
  src/boards/01-222.cpp
  src/boards/112.cpp
  src/boards/117.cpp
  src/boards/164.cpp
  src/boards/183.cpp
  src/boards/185.cpp
  src/boards/186.cpp
  src/boards/187.cpp
  src/boards/189.cpp
  src/boards/199.cpp
  src/boards/208.cpp
  src/boards/222.cpp
  src/boards/235.cpp
  src/boards/43.cpp
  src/boards/57.cpp
  src/boards/603-5052.cpp
  src/boards/8157.cpp
  src/boards/8237.cpp
  src/boards/88.cpp
  src/boards/90.cpp
  src/boards/95.cpp
  src/boards/__dummy_mapper.cpp
  src/boards/a9711.cpp
  src/boards/addrlatch.cpp
  src/boards/bmc13in1jy110.cpp
  src/boards/bmc42in1r.cpp
  src/boards/bmc64in1nr.cpp
  src/boards/bmc70in1.cpp
  src/boards/bonza.cpp
  src/boards/datalatch.cpp
  src/boards/deirom.cpp
  src/boards/dream.cpp
  src/boards/edu2000.cpp
  src/boards/fk23c.cpp
  src/boards/h2288.cpp
  src/boards/karaoke.cpp
  src/boards/kof97.cpp
  src/boards/konami-qtai.cpp
  src/boards/malee.cpp
  src/boards/mmc1.cpp
  src/boards/mmc3.cpp
  src/boards/mmc5.cpp
  src/boards/n-c22m.cpp
  src/boards/n106.cpp
  src/boards/novel.cpp
  src/boards/sachen.cpp
  src/boards/sheroes.cpp
  src/boards/sl1632.cpp
  src/boards/subor.cpp
  src/boards/super24.cpp
  src/boards/supervision.cpp
  src/boards/t-262.cpp
  src/boards/tengen.cpp
  src/input/arkanoid.cpp
  src/input/bworld.cpp
  src/input/cursor.cpp
  src/input/fkb.cpp
  src/input/ftrainer.cpp
  src/input/hypershot.cpp
  src/input/mahjong.cpp
  src/input/mouse.cpp
  src/input/oekakids.cpp
  src/input/powerpad.cpp
  src/input/quiz.cpp
  src/input/shadow.cpp
  src/input/suborkb.cpp
  src/input/toprider.cpp
  src/input/zapper.cpp
  src/mappers/15.cpp
  src/mappers/151.cpp
  src/mappers/16.cpp
  src/mappers/17.cpp
  src/mappers/18.cpp
  src/mappers/193.cpp
  src/mappers/200.cpp
  src/mappers/201.cpp
  src/mappers/202.cpp
  src/mappers/203.cpp
  src/mappers/204.cpp
  src/mappers/21.cpp
  src/mappers/212.cpp
  src/mappers/213.cpp
  src/mappers/214.cpp
  src/mappers/215.cpp
  src/mappers/217.cpp
  src/mappers/22.cpp
  src/mappers/225.cpp
  src/mappers/227.cpp
  src/mappers/228.cpp
  src/mappers/229.cpp
  src/mappers/23.cpp
  src/mappers/230.cpp
  src/mappers/231.cpp
  src/mappers/232.cpp
  src/mappers/234.cpp
  src/mappers/240.cpp
  src/mappers/241.cpp
  src/mappers/242.cpp
  src/mappers/244.cpp
  src/mappers/246.cpp
  src/mappers/24and26.cpp
  src/mappers/25.cpp
  src/mappers/255.cpp
  src/mappers/27.cpp
  src/mappers/32.cpp
  src/mappers/33.cpp
  src/mappers/40.cpp
  src/mappers/41.cpp
  src/mappers/42.cpp
  src/mappers/43.cpp
  src/mappers/46.cpp
  src/mappers/50.cpp
  src/mappers/51.cpp
  src/mappers/59.cpp
  src/mappers/6.cpp
  src/mappers/60.cpp
  src/mappers/61.cpp
  src/mappers/62.cpp
  src/mappers/65.cpp
  src/mappers/67.cpp
  src/mappers/68.cpp
  src/mappers/69.cpp
  src/mappers/71.cpp
  src/mappers/72.cpp
  src/mappers/73.cpp
  src/mappers/75.cpp
  src/mappers/76.cpp
  src/mappers/77.cpp
  src/mappers/79.cpp
  src/mappers/8.cpp
  src/mappers/80.cpp
  src/mappers/82.cpp
  src/mappers/83.cpp
  src/mappers/85.cpp
  src/mappers/86.cpp
  src/mappers/89.cpp
  src/mappers/91.cpp
  src/mappers/92.cpp
  src/mappers/97.cpp
  src/mappers/99.cpp
  src/mappers/__226.cpp
  src/mappers/emu2413.c
  src/mappers/mmc2and4.cpp
  src/mappers/simple.cpp
  src/utils/crc32.cpp
  src/utils/endian.cpp
  src/utils/general.cpp
  src/utils/md5.cpp
  src/utils/memory.cpp
  src/utils/unzip.cpp
  src/utils/xstring.cpp
)

set(SRC_DRIVERS_COMMON
  src/drivers/common/args.cpp
  src/drivers/common/cheat.cpp
  src/drivers/common/config.cpp
  src/drivers/common/configSys.cpp
  src/drivers/common/hq2x.cpp
  src/drivers/common/hq3x.cpp
  src/drivers/common/scale2x.cpp
  src/drivers/common/scale3x.cpp
  src/drivers/common/scalebit.cpp
  src/drivers/common/vidblit.cpp
)

set(SRC_DRIVERS_SDL
  src/drivers/sdl/config.cpp
  src/drivers/sdl/input.cpp
  src/drivers/sdl/sdl.cpp
  src/drivers/sdl/sdl-joystick.cpp
  src/drivers/sdl/sdl-sound.cpp
  src/drivers/sdl/sdl-throttle.cpp
  src/drivers/sdl/sdl-video.cpp
  src/drivers/sdl/unix-netplay.cpp
)

if(FCEUX_SDL_OPENGL)
  set(SRC_DRIVERS_SDL ${SRC_DRIVERS_SDL} src/drivers/sdl/sdl-opengl.cpp)
endif(FCEUX_SDL_OPENGL)

set(SRC_DRIVERS_WIN
  src/drivers/win/args.cpp
  src/drivers/win/aviout.cpp
  src/drivers/win/basicbot.cpp
  src/drivers/win/cdlogger.cpp
  src/drivers/win/cheat.cpp
  src/drivers/win/common.cpp
  src/drivers/win/config.cpp
  src/drivers/win/debugger.cpp
  src/drivers/win/debuggersp.cpp
  src/drivers/win/directories.cpp
  src/drivers/win/gui.cpp
  src/drivers/win/guiconfig.cpp
  src/drivers/win/help.cpp
  src/drivers/win/input.cpp
  src/drivers/win/joystick.cpp
  src/drivers/win/keyboard.cpp
  src/drivers/win/log.cpp
  src/drivers/win/main.cpp
  src/drivers/win/mapinput.cpp
  src/drivers/win/memview.cpp
  src/drivers/win/memviewsp.cpp
  src/drivers/win/memwatch.cpp
  src/drivers/win/monitor.cpp
  src/drivers/win/netplay.cpp
  src/drivers/win/ntview.cpp
  src/drivers/win/OutputDS.cpp
  src/drivers/win/palette.cpp
  src/drivers/win/ppuview.cpp
  src/drivers/win/pref.cpp
  src/drivers/win/replay.cpp
  src/drivers/win/res.rc
  src/drivers/win/sound.cpp
  src/drivers/win/state.cpp
  src/drivers/win/tasedit.cpp
  src/drivers/win/throttle.cpp
  src/drivers/win/timing.cpp
  src/drivers/win/tracer.cpp
  src/drivers/win/video.cpp
  src/drivers/win/wave.cpp
  src/drivers/win/window.cpp
  src/drivers/win/zlib/adler32.c
  src/drivers/win/zlib/compress.c
  src/drivers/win/zlib/crc32.c
  src/drivers/win/zlib/deflate.c
  src/drivers/win/zlib/gzio.c
  src/drivers/win/zlib/infblock.c
  src/drivers/win/zlib/infcodes.c
  src/drivers/win/zlib/inffast.c
  src/drivers/win/zlib/inflate.c
  src/drivers/win/zlib/inftrees.c
  src/drivers/win/zlib/infutil.c
  src/drivers/win/zlib/trees.c
  src/drivers/win/zlib/uncompr.c
  src/drivers/win/zlib/zutil.c
)

include_directories( .  src )
add_definitions( -DNETWORK )

if(WIN32)
  set(SOURCES ${SRC_CORE} ${SRC_DRIVERS_COMMON} ${SRC_DRIVERS_WIN})
  include_directories( src/drivers/win/directx src/drivers/win/zlib )
  add_definitions( 
    -DWIN32
    -DFCEUDEF_DEBUGGER
    -D_USE_SHARED_MEMORY_
    -DPSS_STYLE=2
    -DNOMINMAX
  )
  link_directories( src/drivers/win/directx )
else(WIN32)
  set(SOURCES ${SRC_CORE} ${SRC_DRIVERS_COMMON} ${SRC_DRIVERS_SDL})
  include_directories( ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} )
  add_definitions( ${SDL_DEFINITIONS} ${ZLIB_DEFINITIONS} )
  if(FCEUX_SDL_OPENGL)
    add_definitions( -DOPENGL )
    include_directories( ${OPENGL_INCLUDE_DIR} )
  endif(FCEUX_SDL_OPENGL)
endif(WIN32)

if(APPLE)
  add_definitions( -DPSS_STYLE=4 )
else(APPLE)
  if(UNIX)
    add_definitions( -DPSS_STYLE=1 )
  endif(UNIX)
endif(APPLE)

if(MINGW)
  add_definitions( -DNEED_MINGW_HACKS -D_WIN32_IE=0x0600 )
endif(MINGW)
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-write-strings -Wno-sign-compare")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(FCEUX_FRAMESKIP)
  add_definitions( -DFRAMESKIP )
endif(FCEUX_FRAMESKIP)

if(NOT FCEUX_FORCE_BE)
  if(FCEUX_FORCE_LE)
    add_definitions( -DLSB_FIRST )
  else(FCEUX_FORCE_LE)
    test_big_endian(SYS_IS_BE)
    if(NOT SYS_IS_BE)
      add_definitions( -DLSB_FIRST )
    endif(NOT SYS_IS_BE)
  endif(FCEUX_FORCE_LE)
endif(NOT FCEUX_FORCE_BE)

check_function_exists(asprintf HAVE_ASPRINTF)
if(HAVE_ASPRINTF)
  add_definitions( -DHAVE_ASPRINTF )
endif(HAVE_ASPRINTF)

add_executable( fceux ${SOURCES} )

if(WIN32)
  target_link_libraries( fceux rpcrt4 comctl32 vfw32 winmm ws2_32 htmlhelp
    comdlg32 ole32 gdi32
    dsound dxguid ddraw dinput
  )
else(WIN32)
  target_link_libraries( fceux ${SDL_LIBRARY} ${ZLIB_LIBRARIES} )
  if(FCEUX_SDL_OPENGL)
    target_link_libraries( fceux ${OPENGL_gl_LIBRARY} )
  endif(FCEUX_SDL_OPENGL)
endif(WIN32)

#TODO: Add rules
# Put built executable in bin/
# if(WIN32), copy src/drivers/win/help/fceux.chm into bin/
