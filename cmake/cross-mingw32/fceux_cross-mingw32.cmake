include(CheckIncludeFile)

# Find mingw by looking for windows.h; set HOST accordingly.
find_path(PATH_TO_WINDOWS_H windows.h
  /usr/i386-mingw32msvc/mingw/include
  /usr/i586-mingw32msvc/mingw/include
  /usr/i686-mingw32msvc/mingw/include
  /usr/i386-mingw32/mingw/include
  /usr/i586-mingw32/mingw/include
  /usr/i686-mingw32/mingw/include
)
string(REGEX MATCH "i.86-[^/]*" HOST ${PATH_TO_WINDOWS_H})

set(CMAKE_CROSSCOMPILING TRUE)
set(MINGW TRUE)
set(WIN32 TRUE)
set(APPLE FALSE)
set(UNIX FALSE)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER "${HOST}-gcc")
set(CMAKE_CXX_COMPILER "${HOST}-g++")
set(CMAKE_RC_COMPILER "${HOST}-windres")

set(CMAKE_FIND_ROOT_PATH "/usr/${HOST}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

include(${CMAKE_SOURCE_DIR}/cmake/fceux.cmake)
# I cannot fucking believe that I need to do this:
#don't: ENABLE_LANGUAGE(RC) # above
#do: (for cmake-2.6.0 at least)
set(OUTPUT_RES_RC_O CMakeFiles/${FCEUX_EXE_NAME}.dir/__/__/__/src/drivers/win/res.rc.o)
add_custom_command(
  OUTPUT ${OUTPUT_RES_RC_O}
  COMMAND ${HOST}-windres -Isrc/drivers/win -DLVS_OWNERDATA=0x1000 -o ${OUTPUT_RES_RC_O} ${CMAKE_SOURCE_DIR}/src/drivers/win/res.rc
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
  PROPERTY LINK_FLAGS ${OUTPUT_RES_RC_O}
)
