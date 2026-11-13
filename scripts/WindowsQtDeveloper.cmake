# Opt-in developer layout; leave CI packaging and existing builds unchanged.
set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${APP_NAME})
set_target_properties(${APP_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${APP_NAME}>")

if(MSVC)
	target_compile_options(${APP_NAME} PRIVATE /MP)
endif()

set(_ide_sources ${SOURCES} "${CMAKE_SOURCE_DIR}/resources.qrc" ${APP_ICON_RESOURCES_WINDOWS})
if(RCHEEVOS_SOURCE)
	list(REMOVE_ITEM _ide_sources ${RCHEEVOS_SOURCE})
	source_group(TREE "${RCHEEVOS_DIR}" PREFIX "rcheevos" FILES ${RCHEEVOS_SOURCE})
endif()
source_group(TREE "${CMAKE_SOURCE_DIR}" PREFIX "" FILES ${_ide_sources})
source_group("Generated" FILES "${CMAKE_CURRENT_BINARY_DIR}/fceux_git_info.cpp")

get_target_property(_qt_qmake ${Qt}::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin "${_qt_qmake}" DIRECTORY)
set(_windeployqt "${_qt_bin}/windeployqt.exe")
if(NOT EXISTS "${_windeployqt}")
	message(FATAL_ERROR "Developer setup requires windeployqt from the selected Qt installation")
endif()

file(GLOB _archive_dlls CONFIGURE_DEPENDS "${LIBARCHIVE_INSTALL_PREFIX}/bin/*.dll")
if(NOT _archive_dlls)
	message(FATAL_ERROR "Developer setup requires libarchive runtime DLLs")
endif()
set(_runtime_files
	"${SDL_INSTALL_PREFIX}/lib/x64/SDL2.dll"
	"${CMAKE_SOURCE_DIR}/src/drivers/win/lua/x64/lua51.dll"
	"${CMAKE_SOURCE_DIR}/src/drivers/win/lua/x64/lua5.1.dll"
	"${CMAKE_SOURCE_DIR}/src/auxlib.lua"
	${_archive_dlls})
if(USE_LIBAV)
	file(GLOB _ffmpeg_dlls CONFIGURE_DEPENDS "${FFMPEG_INSTALL_PREFIX}/bin/*.dll")
	if(NOT _ffmpeg_dlls)
		message(FATAL_ERROR "Developer setup requires FFmpeg runtime DLLs")
	endif()
	list(APPEND _runtime_files ${_ffmpeg_dlls})
endif()

add_custom_command(TARGET ${APP_NAME} POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different
		${_runtime_files} "$<TARGET_FILE_DIR:${APP_NAME}>"
	COMMAND "${_windeployqt}"
		"$<IF:$<CONFIG:Debug>,--debug,--release>" --no-compiler-runtime
		"$<TARGET_FILE:${APP_NAME}>"
	COMMENT "Deploying Qt and emulator runtime dependencies"
	VERBATIM)

# The Qt frontend looks for these directories one level above the executable.
foreach(_data_dir palettes luaScripts tools)
	add_custom_command(TARGET ${APP_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_directory
			"${CMAKE_SOURCE_DIR}/output/${_data_dir}"
			"$<TARGET_FILE_DIR:${APP_NAME}>/../${_data_dir}"
		VERBATIM)
endforeach()
add_custom_command(TARGET ${APP_NAME} POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${APP_NAME}>/../doc"
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different
		"${CMAKE_SOURCE_DIR}/output/fceux.chm"
		"${CMAKE_SOURCE_DIR}/output/taseditor.chm"
		"$<TARGET_FILE_DIR:${APP_NAME}>/../doc"
	VERBATIM)
