
cmake_minimum_required(VERSION 3.16)

project(fceux)

# CTest is only needed for the opt-in GUI test server (option defined in src/).
if (FCEU_TEST_SERVER)
	include(CTest)
endif()

add_subdirectory( src )

