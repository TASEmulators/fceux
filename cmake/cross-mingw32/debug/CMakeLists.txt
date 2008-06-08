project("fceux: cross-mingw32 debug")
set(CMAKE_BUILD_TYPE debug)
set(FCEUX_EXE_NAME fceuxDBG.exe)
include(${CMAKE_SOURCE_DIR}/cmake/cross-mingw32/fceux_cross-mingw32.cmake)
