project("fceux: native release")
set(CMAKE_BUILD_TYPE release)
set(FCEUX_EXE_NAME fceuxREL)
include(${CMAKE_SOURCE_DIR}/cmake/native/fceux_native.cmake)
