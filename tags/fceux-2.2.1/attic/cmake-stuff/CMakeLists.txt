cmake_minimum_required(VERSION 2.6)
project(fceux)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
add_subdirectory(cmake/native)
if(UNIX)
  add_subdirectory(cmake/cross-mingw32)
endif(UNIX)

add_custom_command(
  OUTPUT ${EXECUTABLE_OUTPUT_PATH}/fceux.chm
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/drivers/win/help/fceux.chm ${EXECUTABLE_OUTPUT_PATH}/fceux.chm
  DEPENDS ${CMAKE_SOURCE_DIR}/src/drivers/win/help/fceux.chm
  VERBATIM
)
add_custom_target(
  InstallHelpFile
  ALL
  DEPENDS ${EXECUTABLE_OUTPUT_PATH}/fceux.chm
)
