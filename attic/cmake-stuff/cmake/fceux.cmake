#TODO: install generated exe's, appropriately named, in bin/
include(CheckFunctionExists)
include(TestBigEndian)

option(FCEUX_FORCE_LE   "Build for a little-endian target" OFF)
option(FCEUX_FORCE_BE   "Build for a big-endian target" OFF)
option(FCEUX_FRAMESKIP  "Build legacy frameskip code" OFF)

if(WIN32)
  OPTION (NOWIN32 "Replace Win32 support with SDL" OFF)
  if (NOWIN32)
    set(WIN32 OFF)
  endif(NOWIN32)
endif(WIN32)

if (NOT WIN32)
  option(FCEUX_SDL_OPENGL "Build with OpenGL support" ON)
  find_package(SDL REQUIRED)
  find_package(ZLIB REQUIRED)
  if(FCEUX_SDL_OPENGL)
    find_package(OpenGL REQUIRED)
  endif(FCEUX_SDL_OPENGL)
endif(NOT WIN32)

set(SRC_CORE
  ${CMAKE_SOURCE_DIR}/src/asm.cpp
  ${CMAKE_SOURCE_DIR}/src/cart.cpp
  ${CMAKE_SOURCE_DIR}/src/cheat.cpp
  ${CMAKE_SOURCE_DIR}/src/conddebug.cpp
  ${CMAKE_SOURCE_DIR}/src/config.cpp
  ${CMAKE_SOURCE_DIR}/src/debug.cpp
  ${CMAKE_SOURCE_DIR}/src/drawing.cpp
  ${CMAKE_SOURCE_DIR}/src/fceu.cpp
  ${CMAKE_SOURCE_DIR}/src/fds.cpp
  ${CMAKE_SOURCE_DIR}/src/file.cpp
  ${CMAKE_SOURCE_DIR}/src/emufile.cpp
  ${CMAKE_SOURCE_DIR}/src/filter.cpp
  ${CMAKE_SOURCE_DIR}/src/ines.cpp
  ${CMAKE_SOURCE_DIR}/src/input.cpp
  ${CMAKE_SOURCE_DIR}/src/movie.cpp
  ${CMAKE_SOURCE_DIR}/src/netplay.cpp
  ${CMAKE_SOURCE_DIR}/src/nsf.cpp
  ${CMAKE_SOURCE_DIR}/src/oldmovie.cpp
  ${CMAKE_SOURCE_DIR}/src/palette.cpp
  ${CMAKE_SOURCE_DIR}/src/ppu.cpp
  ${CMAKE_SOURCE_DIR}/src/sound.cpp
  ${CMAKE_SOURCE_DIR}/src/state.cpp
  ${CMAKE_SOURCE_DIR}/src/unif.cpp
  ${CMAKE_SOURCE_DIR}/src/video.cpp
  ${CMAKE_SOURCE_DIR}/src/vsuni.cpp
  ${CMAKE_SOURCE_DIR}/src/wave.cpp
  ${CMAKE_SOURCE_DIR}/src/x6502.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/01-222.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/103.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/106.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/108.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/112.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/117.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/120.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/121.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/15.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/164.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/175.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/176.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/177.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/178.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/179.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/183.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/185.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/186.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/187.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/189.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/199.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/208.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/222.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/23.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/235.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/253.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/3d-block.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/411120-c.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/43.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/57.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/603-5052.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/68.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/8157.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/8237.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/830118C.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/88.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/90.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/95.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/__dummy_mapper.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/a9711.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/a9746.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/addrlatch.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/ax5705.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/bandai.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/bmc13in1jy110.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/bmc42in1r.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/bmc64in1nr.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/bmc70in1.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/bonza.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/bs-5.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/onebus.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/datalatch.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/deirom.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/dream.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/edu2000.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/fk23c.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/ghostbusters63in1.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/gs-2004.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/gs-2013.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/h2288.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/karaoke.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/kof97.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/vrc5.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/ks7032.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/malee.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/mmc1.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/mmc3.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/mmc5.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/n-c22m.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/n106.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/n625092.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/novel.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/sachen.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/sc-127.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/sheroes.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/sl1632.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/smb2j.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/subor.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/super24.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/supervision.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/t-227-1.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/t-262.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/tengen.cpp
  ${CMAKE_SOURCE_DIR}/src/boards/tf-1201.cpp
  ${CMAKE_SOURCE_DIR}/src/input/arkanoid.cpp
  ${CMAKE_SOURCE_DIR}/src/input/bworld.cpp
  ${CMAKE_SOURCE_DIR}/src/input/cursor.cpp
  ${CMAKE_SOURCE_DIR}/src/input/fkb.cpp
  ${CMAKE_SOURCE_DIR}/src/input/ftrainer.cpp
  ${CMAKE_SOURCE_DIR}/src/input/hypershot.cpp
  ${CMAKE_SOURCE_DIR}/src/input/mahjong.cpp
  ${CMAKE_SOURCE_DIR}/src/input/mouse.cpp
  ${CMAKE_SOURCE_DIR}/src/input/snesmouse.cpp
  ${CMAKE_SOURCE_DIR}/src/input/oekakids.cpp
  ${CMAKE_SOURCE_DIR}/src/input/powerpad.cpp
  ${CMAKE_SOURCE_DIR}/src/input/quiz.cpp
  ${CMAKE_SOURCE_DIR}/src/input/shadow.cpp
  ${CMAKE_SOURCE_DIR}/src/input/suborkb.cpp
  ${CMAKE_SOURCE_DIR}/src/input/toprider.cpp
  ${CMAKE_SOURCE_DIR}/src/input/zapper.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/151.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/16.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/17.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/18.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/193.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/201.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/202.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/203.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/204.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/21.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/212.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/213.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/214.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/215.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/217.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/22.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/225.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/227.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/228.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/229.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/230.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/231.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/232.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/234.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/241.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/242.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/244.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/246.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/24and26.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/25.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/255.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/27.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/32.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/33.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/40.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/41.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/42.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/46.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/50.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/51.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/59.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/6.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/60.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/61.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/62.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/65.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/67.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/69.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/71.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/72.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/73.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/75.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/76.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/77.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/79.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/8.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/80.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/82.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/83.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/85.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/86.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/89.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/91.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/92.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/97.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/99.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/emu2413.c
  ${CMAKE_SOURCE_DIR}/src/mappers/mmc2and4.cpp
  ${CMAKE_SOURCE_DIR}/src/mappers/simple.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/ConvertUTF.c
  ${CMAKE_SOURCE_DIR}/src/utils/crc32.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/endian.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/general.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/guid.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/md5.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/memory.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/unzip.cpp
  ${CMAKE_SOURCE_DIR}/src/utils/xstring.cpp
)

set(SRC_DRIVERS_COMMON
  ${CMAKE_SOURCE_DIR}/src/drivers/common/args.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/cheat.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/config.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/configSys.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/hq2x.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/hq3x.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/scale2x.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/scale3x.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/scalebit.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/vidblit.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/common/nes_ntsc.c
)

set(SRC_DRIVERS_SDL
  ${CMAKE_SOURCE_DIR}/src/drivers/sdl/config.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/sdl/input.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/sdl/sdl.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/sdl/sdl-joystick.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/sdl/sdl-sound.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/sdl/sdl-throttle.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/sdl/sdl-video.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/sdl/unix-netplay.cpp
)

if(FCEUX_SDL_OPENGL)
  set(SRC_DRIVERS_SDL ${SRC_DRIVERS_SDL} ${CMAKE_SOURCE_DIR}/src/drivers/sdl/sdl-opengl.cpp)
endif(FCEUX_SDL_OPENGL)

set(SRC_DRIVERS_WIN
  ${CMAKE_SOURCE_DIR}/src/lua-engine.cpp
  ${CMAKE_SOURCE_DIR}/src/lua/src/lapi.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lauxlib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lbaselib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lcode.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/ldblib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/ldebug.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/ldo.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/ldump.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lfunc.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lgc.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/linit.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/liolib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/llex.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lmathlib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lmem.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/loadlib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lobject.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lopcodes.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/loslib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lparser.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lstate.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lstring.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lstrlib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/ltable.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/ltablib.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/ltm.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lundump.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lvm.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/lzio.c
  ${CMAKE_SOURCE_DIR}/src/lua/src/print.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/archive.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/args.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/aviout.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/cdlogger.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/cheat.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/common.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/config.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/debugger.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/debuggersp.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/directories.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/gui.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/guiconfig.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/help.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/input.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/joystick.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/keyboard.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/log.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/luaconsole.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/main.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/mapinput.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/memview.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/memviewsp.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/memwatch.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/movieoptions.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/monitor.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/netplay.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/ntview.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/OutputDS.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/palette.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/ppuview.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/pref.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/ram_search.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/ramwatch.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/replay.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/res.rc
  ${CMAKE_SOURCE_DIR}/src/drivers/win/sound.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/state.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/tasedit.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/taseditlib/taseditproj.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/taseditlib/greenzone.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/taseditlib/bookmark.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/taseditlib/bookmarks.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/taseditlib/inputhistory.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/taseditlib/inputsnapshot.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/taseditlib/playback.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/taseditlib/markers.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/texthook.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/throttle.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/timing.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/tracer.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/video.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/wave.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/window.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/Win32InputBox.cpp
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/adler32.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/compress.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/crc32.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/deflate.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/gzio.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/infblock.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/infcodes.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/inffast.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/inflate.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/inftrees.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/infutil.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/trees.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/uncompr.c
  ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib/zutil.c
 # ${CMAKE_SOURCE_DIR}/src/drivers/res.rc
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories( ${CMAKE_SOURCE_DIR}/src )
add_definitions( -DNETWORK )

if(WIN32)
  set(SOURCES ${SRC_CORE} ${SRC_DRIVERS_COMMON} ${SRC_DRIVERS_WIN})
  include_directories( ${CMAKE_SOURCE_DIR}/src/drivers/win/directx ${CMAKE_SOURCE_DIR}/src/drivers/win/zlib )
  add_definitions(
    -DWIN32
    -DFCEUDEF_DEBUGGER
    -D_USE_SHARED_MEMORY_
    -DPSS_STYLE=2
    -DNOMINMAX
        -D_S9XLUA_H
  )
  link_directories( ${CMAKE_SOURCE_DIR}/src/drivers/win/directx )
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
if(CMAKE_BUILD_TYPE STREQUAL "debug")
  add_definitions( -D_DEBUG )
endif(CMAKE_BUILD_TYPE STREQUAL "debug")
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wno-write-strings -Wno-sign-compare")
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
# HACK: cmake seems to cache HAVE_ASPRINTF and I don't know how to ask it
# to forget--even if your compiler changes. So tell it mingw=>no.
if(HAVE_ASPRINTF AND NOT MINGW)
  add_definitions( -DHAVE_ASPRINTF )
endif(HAVE_ASPRINTF AND NOT MINGW)

add_executable( ${FCEUX_EXE_NAME} ${SOURCES} )

if(WIN32)
  add_dependencies( ${FCEUX_EXE_NAME} InstallHelpFile )
  #add_dependencies( ${FCEUX_EXE_NAME} res.o)
  #add_custom_command(
  #  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/res.o
  #  COMMAND windres
  #  ARGS ${CMAKE_SOURCE_DIR}/src/drivers/win/res.rc ${CMAKE_CURRENT_BINARY_DIR}/res.o --use-temp-file -D_WIN_IE=0x300
  #  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/src/drivers/win/res.rc
  #)
  set(OUTPUT_RES_RC_O CMakeFiles/${FCEUX_EXE_NAME}.dir/__/__/__/src/drivers/win/res.rc.o)
  add_custom_command(
    OUTPUT ${OUTPUT_RES_RC_O}
    COMMAND windres -o ${OUTPUT_RES_RC_O} ${CMAKE_SOURCE_DIR}/src/drivers/win/res.rc
    DEPENDS ${CMAKE_SOURCE_DIR}/src/drivers/win/res.rc
    VERBATIM
  )
  add_custom_target(
    BuildResourceFileFor${FCEUX_EXE_NAME}
    DEPENDS ${OUTPUT_RES_RC_O}
  )
  set(DEFINED_BUILD_RESOURCE_FILE TRUE CACHE GLOBAL "This is a hack to get CMake to build the .rc file")

  add_dependencies( ${FCEUX_EXE_NAME} BuildResourceFileFor${FCEUX_EXE_NAME} )
  set_property(
    TARGET ${FCEUX_EXE_NAME}
    PROPERTY LINK_FLAGS "${OUTPUT_RES_RC_O} -mwindows"
  )

  target_link_libraries( ${FCEUX_EXE_NAME} rpcrt4 comctl32 vfw32 winmm ws2_32
    comdlg32 ole32 gdi32
    dsound dxguid ddraw dinput
  )
  if(MINGW)
        include_directories(${CMAKE_SOURCE_DIR}/src/drivers/win/lua/include/)
  else(MINGW)
    target_link_libraries( ${FCEUX_EXE_NAME} htmlhelp)
  endif(MINGW)
else(WIN32)
  target_link_libraries( ${FCEUX_EXE_NAME} ${SDL_LIBRARY} ${ZLIB_LIBRARIES} )
  if(FCEUX_SDL_OPENGL)
    target_link_libraries( ${FCEUX_EXE_NAME} ${OPENGL_gl_LIBRARY} )
  endif(FCEUX_SDL_OPENGL)
endif(WIN32)


