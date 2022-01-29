include(GNUInstallDirs)

set( APP_NAME fceux)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if ( ${QT6} )
	message( STATUS "GUI Frontend: Qt6")
	set( Qt Qt6 )
else()
	message( STATUS "GUI Frontend: Qt5")
	set( Qt Qt5 )
endif()

if ( ${QHELP} )
	set(QtHelpModule Help)
	add_definitions( -D_USE_QHELP )
endif()

if ( ${QT6} )
	find_package( Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets ${QtHelpModule})
  	add_definitions( ${Qt6Widgets_DEFINITIONS} ${Qt6Help_DEFINITIONS} ${Qt6OpenGLWidgets_DEFINITIONS} )
  	include_directories( ${Qt6Widgets_INCLUDE_DIRS} ${Qt6Help_INCLUDE_DIRS} ${Qt6OpenGLWidgets_INCLUDE_DIRS} )
else()
	find_package( Qt5 REQUIRED COMPONENTS Widgets OpenGL ${QtHelpModule})
	add_definitions( ${Qt5Widgets_DEFINITIONS} ${Qt5Help_DEFINITIONS} )
	include_directories( ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Help_INCLUDE_DIRS} )
endif()

if(WIN32)
     find_package(OpenGL REQUIRED)
     #find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)
     #add_definitions( ${Qt5Widgets_DEFINITIONS}  )
     #include_directories( ${Qt5Widgets_INCLUDE_DIRS} )
     add_definitions( -DMSVC -D_CRT_SECURE_NO_WARNINGS )
     add_definitions( -D__SDL__  -D__QT_DRIVER__  -DQT_DEPRECATED_WARNINGS )
     add_definitions( -DFCEUDEF_DEBUGGER )
     include_directories( ${SDL_INSTALL_PREFIX}/SDL2/include )
     include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib )
     set( OPENGL_LDFLAGS  OpenGL::GL )
     set( SDL2_LDFLAGS  ${SDL_INSTALL_PREFIX}/SDL2/lib/x64/SDL2.lib )
     set( SYS_LIBS  wsock32 ws2_32 vfw32 Htmlhelp )
     set(APP_ICON_RESOURCES_WINDOWS  ${CMAKE_SOURCE_DIR}/icons/fceux.rc )

     if ( ${USE_LIBAV} )
        add_definitions( -D_USE_LIBAV ${LIBAV_CFLAGS} )
        include_directories( ${FFMPEG_INSTALL_PREFIX}/ffmpeg/include )
        set( LIBAV_LDFLAGS  ${FFMPEG_INSTALL_PREFIX}/ffmpeg/lib/avcodec.lib
        	                 ${FFMPEG_INSTALL_PREFIX}/ffmpeg/lib/avformat.lib
                	         ${FFMPEG_INSTALL_PREFIX}/ffmpeg/lib/avutil.lib
                        	 ${FFMPEG_INSTALL_PREFIX}/ffmpeg/lib/swscale.lib
                        	 ${FFMPEG_INSTALL_PREFIX}/ffmpeg/lib/swresample.lib )
     endif()

else(WIN32)
  # Non Windows System
  # UNIX (Linux or Mac OSX)

	if ( ${GLVND} )
		message( STATUS "OpenGL preference: GLVND")
		set (OpenGL_GL_PREFERENCE  GLVND)
	else()
		message( STATUS "OpenGL preference: LEGACY")
		set (OpenGL_GL_PREFERENCE  LEGACY)
	endif()

  # Use the built-in cmake find_package functions to find dependencies
  # Use package PkgConfig to detect headers/library what find_package cannot find.
  find_package(PkgConfig REQUIRED)
  find_package(OpenGL REQUIRED)
  find_package(ZLIB REQUIRED)

  add_definitions( -Wall  -Wno-write-strings  -Wno-sign-compare  -Wno-parentheses  -Wno-unused-local-typedefs  -fPIC )
  add_definitions( -DFCEUDEF_DEBUGGER )

  #if ( ${QT6} )
  #	find_package( Qt6 COMPONENTS Widgets OpenGL OpenGLWidgets REQUIRED)
  #	add_definitions( ${Qt6Widgets_DEFINITIONS} ${Qt6OpenGLWidgets_DEFINITIONS} )
  #	include_directories( ${Qt6Widgets_INCLUDE_DIRS} ${QtOpenGLWidgets_INCLUDE_DIRS} )
  #else()
  #	find_package( Qt5 COMPONENTS Widgets OpenGL REQUIRED)
  #	add_definitions( ${Qt5Widgets_DEFINITIONS}  )
  #	include_directories( ${Qt5Widgets_INCLUDE_DIRS} )
  #endif()
  add_definitions( -D__QT_DRIVER__  -DQT_DEPRECATED_WARNINGS )

  # Check for libminizip
  pkg_check_modules( MINIZIP REQUIRED minizip)

  if ( ${MINIZIP_FOUND} )
	  message( STATUS "Using System minizip ${MINIZIP_VERSION}" )
	  add_definitions( -D_SYSTEM_MINIZIP ${MINIZIP_CFLAGS} )
  endif()

  pkg_check_modules( X264 x264)

  if ( ${X264_FOUND} )
	  message( STATUS "Using System X264 Encoder Library ${X264_VERSION}" )
	  add_definitions( -D_USE_X264 ${X264_CFLAGS} )
  endif()

  pkg_check_modules( X265 x265)

  if ( ${X265_FOUND} )
	  message( STATUS "Using System X265 Encoder Library ${X265_VERSION}" )
	  add_definitions( -D_USE_X265 ${X265_CFLAGS} )
  endif()

  pkg_check_modules( LIBAV  libavcodec  libavformat  libavutil  libswresample  libswscale)

  if ( ${LIBAV_FOUND} )
	  message( STATUS "Using System Libav Library ${LIBAV_VERSION}" )
	  add_definitions( -D_USE_LIBAV ${LIBAV_CFLAGS} )
  endif()

  #pkg_check_modules( GL gl) # Use built in find package instead for OpenGL

  # Check for OpenGL
  if ( ${OPENGL_FOUND} )
	include_directories( ${OPENGL_INCLUDE_DIR} )
  endif()

  # Check for SDL2
  pkg_check_modules( SDL2 REQUIRED sdl2)

  if ( ${SDL2_FOUND} )
	  add_definitions( ${SDL2_CFLAGS} -D__SDL__ )
  endif()

  # Check for LUA
  pkg_search_module( LUA lua5.1 lua-5.1 )

  add_definitions( -DHAVE_ASPRINTF ) # What system wouldn't have this?
  add_definitions( -DLUA_USE_LINUX ) # This needs to be set when link LUA internally for linux and macosx

  if(APPLE)
	set( OPENGL_LDFLAGS  "-framework OpenGL" )
  else()
	set( OPENGL_LDFLAGS  ${OPENGL_LIBRARIES} )
	# OpenBSD  refuses  to  implement  the POSIX  realtime  interface  and
	# refuses to provide a stub library for -ldl
	if(${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
	  	set( SYS_LIBS  -lpthread)
        else()
          	set( SYS_LIBS  -lrt  -lpthread  -ldl)
        endif()
  endif()

endif(WIN32)

if ( ${LUA_FOUND} )
   # Use System LUA
        message( STATUS "Using System Lua ${LUA_VERSION}" )

        add_definitions( -D_S9XLUA_H  ${LUA_CFLAGS} )

        set( LUA_ENGINE_SOURCE  ${CMAKE_CURRENT_SOURCE_DIR}/lua-engine.cpp )

else ()

   # Use Internal LUA
        message( STATUS "Using Internal Lua" )

   add_definitions( -D_S9XLUA_H  -I${CMAKE_CURRENT_SOURCE_DIR}/lua/src )

        set( LUA_ENGINE_SOURCE  
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lapi.c 
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lauxlib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lbaselib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lcode.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/ldblib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/ldebug.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/ldo.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/ldump.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lfunc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lgc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/linit.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/liolib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/llex.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lmathlib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lmem.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/loadlib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lobject.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lopcodes.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/loslib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lparser.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lstate.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lstring.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lstrlib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/ltable.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/ltablib.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/ltm.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lundump.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lvm.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lzio.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/print.c
      ${CMAKE_CURRENT_SOURCE_DIR}/lua-engine.cpp 
   )
endif()

if ( ${ZLIB_FOUND} )

  message( STATUS "Using System zlib ${ZLIB_VERSION_STRING}" )
  #add_definitions( ${ZLIB_CFLAGS} )
  include_directories( ${ZLIB_INCLUDE_DIRS} )
else()

   message( STATUS "Using Internal zlib" )

   set( ZLIB_SOURCE 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/adler32.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/compress.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/crc32.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/deflate.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/gzio.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/infblock.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/infcodes.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/inffast.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/inflate.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/inftrees.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/infutil.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/trees.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/uncompr.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/unzip.c 
       ${CMAKE_CURRENT_SOURCE_DIR}/drivers/win/zlib/zutil.c 
   	)
endif()

include_directories( ${CMAKE_SOURCE_DIR}/src )
include_directories( ${CMAKE_SOURCE_DIR}/src/drivers )

if(APPLE)
  add_definitions( -DPSS_STYLE=1 )
else(APPLE)
  if(UNIX)
    add_definitions( -DPSS_STYLE=1 )
  endif(UNIX)
endif(APPLE)

set(SRC_CORE
	${CMAKE_CURRENT_SOURCE_DIR}/asm.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/cart.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/cheat.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/conddebug.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/debug.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/drawing.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/fceu.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/fds.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/file.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/emufile.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/filter.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/ines.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/movie.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/netplay.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/nsf.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/oldmovie.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/palette.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/ppu.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/sound.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/state.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/unif.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/video.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/vsuni.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/wave.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/x6502.cpp
	${LUA_ENGINE_SOURCE}
	${ZLIB_SOURCE}
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/01-222.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/09-034a.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/103.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/106.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/108.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/112.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/116.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/117.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/120.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/121.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/12in1.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/151.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/156.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/158B.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/15.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/164.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/168.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/170.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/175.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/176.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/177.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/178.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/183.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/185.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/186.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/187.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/189.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/18.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/190.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/193.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/199.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/206.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/208.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/222.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/225.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/228.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/230.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/232.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/234.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/235.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/244.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/246.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/252.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/253.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/28.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/32.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/33.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/34.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/36.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/3d-block.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/40.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/411120-c.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/41.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/42.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/43.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/46.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/50.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/51.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/57.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/603-5052.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/62.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/65.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/67.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/68.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/69.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/71.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/72.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/77.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/79.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/80013-B.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/80.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/8157.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/8237.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/82.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/830118C.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/88.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/8in1.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/90.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/91.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/96.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/99.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/a9746.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ac-08.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/addrlatch.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ax5705.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bandai.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bb.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bmc13in1jy110.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bmc42in1r.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bmc64in1nr.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bmc70in1.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/BMW8544.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bonza.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bs-5.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/bs4xxxr.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/cheapocabra.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/cityfighter.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/coolboy.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/dance2000.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/datalatch.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/dream.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/__dummy_mapper.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/edu2000.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/eh8813a.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/emu2413.c
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/et-100.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/et-4320.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/F-15.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/famicombox.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ffe.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/fk23c.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/fns.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ghostbusters63in1.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/gs-2004.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/gs-2013.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/h2288.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/hp10xx_hp20xx.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/hp898f.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/inlnsf.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/karaoke.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/kof97.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7010.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7012.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7013.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7016.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7017.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7030.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7031.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7032.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7037.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/ks7057.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/le05.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/lh32.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/lh53.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/malee.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/mihunche.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/mmc1.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/mmc2and4.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/mmc3.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/mmc5.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/n106.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/n625092.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/novel.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/onebus.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/pec-586.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/rt-01.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/sa-9602b.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/sachen.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/sb-2000.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/sc-127.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/sheroes.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/sl1632.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/subor.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/super24.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/supervision.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/t-227-1.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/t-262.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/tengen.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/tf-1201.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/transformer.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/unrom512.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/vrc1.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/vrc2and4.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/vrc3.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/vrc5.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/vrc6.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/vrc7.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/vrc7p.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/boards/yoko.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/arkanoid.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/bworld.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/cursor.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/fkb.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/fns.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/ftrainer.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/hypershot.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/mahjong.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/mouse.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/oekakids.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/pec586kb.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/powerpad.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/quiz.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/shadow.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/snesmouse.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/suborkb.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/toprider.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/virtualboy.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/lcdcompzapper.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/input/zapper.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/utils/backward.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/utils/xstring.cpp
  	${CMAKE_CURRENT_SOURCE_DIR}/utils/crc32.cpp     
  	${CMAKE_CURRENT_SOURCE_DIR}/utils/endian.cpp  
  	${CMAKE_CURRENT_SOURCE_DIR}/utils/general.cpp  
  	${CMAKE_CURRENT_SOURCE_DIR}/utils/guid.cpp    
  	${CMAKE_CURRENT_SOURCE_DIR}/utils/md5.cpp  
  	${CMAKE_CURRENT_SOURCE_DIR}/utils/memory.cpp
)


set(SRC_DRIVERS_COMMON
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/args.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/cheat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/configSys.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/hq2x.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/hq3x.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/scale2x.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/scale3x.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/scalebit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/vidblit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/os_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/common/nes_ntsc.c
)

set(SRC_DRIVERS_SDL
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/main.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ConsoleWindow.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ConsoleViewerGL.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ConsoleViewerSDL.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/InputConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/GamePadConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/HotKeyConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TimingConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/FrameTimingStats.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/PaletteConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/PaletteEditor.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ColorMenu.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/GuiConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/HelpPages.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/MoviePlay.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/MovieRecord.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/MovieOptions.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/LuaControl.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/CheatsConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/GameGenie.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/HexEditor.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/MsgLogViewer.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/CodeDataLogger.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/SymbolicDebug.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ConsoleDebugger.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ConsoleUtilities.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ConsoleVideoConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ConsoleSoundConf.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/iNesHeaderEditor.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/SplashScreen.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TraceLogger.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/AboutWindow.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/fceuWrapper.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/ppuViewer.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/NameTableViewer.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/RamWatch.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/RamSearch.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/config.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/input.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/nes_shm.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/keyscan.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/sdl-sound.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/sdl-video.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/sdl-joystick.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/sdl-throttle.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/unix-netplay.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/AviRecord.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/AviRiffViewer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/avi/avi-utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/avi/fileio.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/avi/gwavi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/TasEditorWindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/taseditor_project.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/taseditor_config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/taseditor_lua.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/markers_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/greenzone.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/selection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/playback.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/recorder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/history.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/splicer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/inputlog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/laglog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/branches.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/bookmarks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/bookmark.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/snapshot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/Qt/TasEditor/markers.cpp
)

set(SOURCES ${SRC_CORE} ${SRC_DRIVERS_COMMON} ${SRC_DRIVERS_SDL})

# Put build timestamp into BUILD_TS environment variable and from there into
# the FCEUX_BUILD_TIMESTAMP preprocessor definition.
# Note: with CMake >= 3.8.0, this will respect SOURCE_DATE_EPOCH. For more info,
#       see <https://reproducible-builds.org/docs/source-date-epoch/>.
string(TIMESTAMP BUILD_TS "%H:%M:%S  %b %d %Y")
add_definitions( -DFCEUX_BUILD_TIMESTAMP=\"${BUILD_TS}\" )

if (WIN32)
add_custom_command( 
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fceux_git_info.cpp  
	COMMAND  ${CMAKE_SOURCE_DIR}/scripts/genGitHdr.bat  ${CMAKE_CURRENT_BINARY_DIR} 
	VERBATIM )
else()
add_custom_command( 
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fceux_git_info.cpp  
	COMMAND  ${CMAKE_SOURCE_DIR}/scripts/genGitHdr.sh  ${CMAKE_CURRENT_BINARY_DIR} 
	VERBATIM )
endif()

set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/fceux_git_info.cpp PROPERTY SKIP_AUTOGEN ON)

if (APPLE)

set(MACOSX_BUNDLE_ICON_FILE fceux.icns)
set(APP_ICON  ${CMAKE_SOURCE_DIR}/fceux.icns )
set_source_files_properties( ${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources" )

add_executable(  ${APP_NAME}  MACOSX_BUNDLE ${APP_ICON} ${SOURCES} ../resources.qrc 
	${CMAKE_CURRENT_BINARY_DIR}/fceux_git_info.cpp)
elseif (WIN32)

add_executable(  ${APP_NAME}  ${SOURCES} ../resources.qrc 
	${CMAKE_CURRENT_BINARY_DIR}/fceux_git_info.cpp
	${APP_ICON_RESOURCES_WINDOWS} )
else()
add_executable(  ${APP_NAME}  ${SOURCES} ../resources.qrc 
	${CMAKE_CURRENT_BINARY_DIR}/fceux_git_info.cpp)
endif()

target_link_libraries( ${APP_NAME}  
   ${${Qt}Widgets_LIBRARIES}
   ${${Qt}Help_LIBRARIES}
   ${${Qt}OpenGL_LIBRARIES}
   ${${Qt}OpenGLWidgets_LIBRARIES}
   ${OPENGL_LDFLAGS}  
 	${SDL2_LDFLAGS}
	${MINIZIP_LDFLAGS} ${ZLIB_LIBRARIES}
	${LUA_LDFLAGS} ${X264_LDFLAGS} ${X265_LDFLAGS} ${LIBAV_LDFLAGS}
 	${SYS_LIBS}
)

if (WIN32)
	#   target_link_libraries( ${APP_NAME} wsock32 ws2_32 )

   set_target_properties(${APP_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if (APPLE)

install( TARGETS  ${APP_NAME}  
	BUNDLE  DESTINATION  . COMPONENT Runtime 
	RUNTIME  DESTINATION  bin COMPONENT Runtime )

# Use \$ to defer expansion until install script is called; CPack will call it with its own CMAKE_INSTALL_PREFIX
set(APP \${CMAKE_INSTALL_PREFIX}/${APP_NAME}.app)

install( DIRECTORY ${CMAKE_SOURCE_DIR}/output/.
	DESTINATION ${APP_NAME}.app/Contents/Resources COMPONENT Extra )

set(CPACK_PACKAGE_ICON ${CMAKE_SOURCE_DIR}/fceux.icns )
set(CPACK_GENERATOR "DragNDrop")
include(CPACK)

# macdeployqt tool that comes with Qt: https://doc.qt.io/qt-5/macos-deployment.html#macdeploy
# Compared to fixup_bundle, correctly finds and installs Qt-specific resources as well as non-Qt dependencies
find_program(MACDEPLOYQT macdeployqt)
if(NOT MACDEPLOYQT)
	message(FATAL_ERROR "Could not find macdeployqt executable")
endif()

find_package(Perl REQUIRED)

install( CODE "
	message(STATUS \"Deploying and fixing up dependencies in app: ${APP}\")
	execute_process(COMMAND \"${MACDEPLOYQT}\" \"${APP}\" -verbose=1)
	execute_process(COMMAND \"${PERL_EXECUTABLE}\"
	                        \"${CMAKE_SOURCE_DIR}/scripts/macOSX_BundleFix.pl\"
	                        \"${CMAKE_BINARY_DIR}/_CPack_Packages\")
		"
	COMPONENT Runtime
	)

else(APPLE)

install( TARGETS  ${APP_NAME}  
	RUNTIME  DESTINATION  bin )

install( FILES     ${CMAKE_CURRENT_SOURCE_DIR}/auxlib.lua                DESTINATION share/fceux/luaScripts  )
install( DIRECTORY ${CMAKE_SOURCE_DIR}/output/.                          DESTINATION share/fceux  )
install( FILES     ${CMAKE_SOURCE_DIR}/fceux1.png                        DESTINATION share/pixmaps  )
install( FILES     ${CMAKE_SOURCE_DIR}/fceux.desktop                     DESTINATION share/applications  )
install( FILES     ${CMAKE_SOURCE_DIR}/documentation/fceux.6             DESTINATION ${CMAKE_INSTALL_MANDIR}/man6  )
install( FILES     ${CMAKE_SOURCE_DIR}/documentation/fceux-net-server.6  DESTINATION ${CMAKE_INSTALL_MANDIR}/man6  )

endif(APPLE)
