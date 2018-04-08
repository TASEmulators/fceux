project("fceux: cross-mingw32 release")
set(CMAKE_BUILD_TYPE release)
set(FCEUX_EXE_NAME fceuxREL.exe)
include(${CMAKE_SOURCE_DIR}/cmake/cross-mingw32/fceux_cross-mingw32.cmake)
