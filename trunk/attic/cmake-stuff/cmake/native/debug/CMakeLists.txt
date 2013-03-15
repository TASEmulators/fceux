project("fceux: native debug")
set(CMAKE_BUILD_TYPE debug)
set(FCEUX_EXE_NAME fceuxDBG)
include(${CMAKE_SOURCE_DIR}/cmake/native/fceux_native.cmake)
