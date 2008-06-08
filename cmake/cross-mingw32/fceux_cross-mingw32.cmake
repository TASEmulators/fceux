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
