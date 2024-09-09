cmake_minimum_required(VERSION 3.16)
include(CMakeDependentOption)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# parameters
option(WEB_BUILD "Build for WebAssembly" OFF)
option(DEBUG_CHECKS "Enable debug checks and assertions" ON)
option(ENABLE_SCOPED_PROFILER "Enable profiler that records time of execution of scopes across threads" OFF)
option(MEMLEAK_DETECTION "Enable memory leaks detection code (slow)" OFF)
option(MEMLEAK_REDEFINE_NEW "Redefine default new (provide more detailed info but can conflict with some libs)" OFF)
option(CONCURRENT_ACCESS_DETECTION "Enable concurrent access detection" ON)
option(DEDICATED_SERVER "Are we building dedicated server" OFF)
option(BUILD_UNIT_TESTS "Build unit-tests with normal build" ON)
option(ENABLE_UNITY_BUILD "Should the sources be built with unity build" OFF)
option(FAKE_NETWORK "Should faked network be used instead of real networking" OFF)
CMAKE_DEPENDENT_OPTION(BUILD_AUTO_TESTS "Build AutoTests project with normal build" ON "NOT DEDICATED_SERVER" OFF)
CMAKE_DEPENDENT_OPTION(DISABLE_SDL "Are we building version without graphics, e.g. for dedicated server" OFF "NOT DEDICATED_SERVER" ON)
CMAKE_DEPENDENT_OPTION(IMGUI_ENABLED "Enable imgui in the build" ON "NOT DISABLE_SDL" OFF)

if(DEDICATED_SERVER)
	set(APP_NAME DedicatedServer)
else()
	set(APP_NAME GameMain)
endif()

set(EXECUTABLE_NAME Game)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# constants
set(GAME_BASE_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${GAME_BASE_DIR}/cmake")
set(GAME_BIN_DIR ${GAME_BASE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GAME_BIN_DIR})
set(GAME_SRC_DIR ${GAME_BASE_DIR}/src)
set(ENGINE_SRC_DIR ${GAME_SRC_DIR}/Engine)
set(EXTERNALS_DIR ${GAME_BASE_DIR}/external)
set(BIN_PLATFORM "x64")

set(BUILD_SHARED_LIBS FALSE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)

file(STRINGS "${GAME_BASE_DIR}/VersionInfo" EXECUTABLE_VERSION)
file(STRINGS "${GAME_BASE_DIR}/BuildNum" BUILD_NUMBER)
project(${APP_NAME} VERSION ${EXECUTABLE_VERSION})

file(MAKE_DIRECTORY ${GAME_BIN_DIR})

# definitions
if(DEBUG_CHECKS)
	add_definitions(-DDEBUG_CHECKS -DRACCOON_ECS_DEBUG_CHECKS_ENABLED)
endif(DEBUG_CHECKS)

if(ENABLE_SCOPED_PROFILER)
	add_definitions(-DENABLE_SCOPED_PROFILER)
endif(ENABLE_SCOPED_PROFILER)

if(MEMLEAK_DETECTION)
	add_definitions(-DDETECT_MEMORY_LEAKS)
	if(MEMLEAK_REDEFINE_NEW)
		add_definitions(-DREDEFINE_NEW)
	endif()
endif()

if(DEDICATED_SERVER)
	add_definitions(-DDEDICATED_SERVER)
endif()

if(DISABLE_SDL)
	add_definitions(-DDISABLE_SDL)
endif()

if(IMGUI_ENABLED)
	add_definitions(-DIMGUI_ENABLED)
endif()

if(CONCURRENT_ACCESS_DETECTION)
	add_definitions(-DCONCURRENT_ACCESS_DETECTION)
endif(CONCURRENT_ACCESS_DETECTION)

if(FAKE_NETWORK)
	add_definitions(-DFAKE_NETWORK)
endif()

if(WEB_BUILD)
	add_definitions(-DWEB_BUILD)
endif()

if(BUILD_AUTO_TESTS)
	add_definitions(-DBUILD_AUTO_TESTS)
endif()

add_definitions(-DGLEW_NO_GLU)
add_definitions(-DGLM_FORCE_RADIANS)
add_definitions(-DRACCOON_ECS_COPYABLE_COMPONENTS)

if (NOT WIN32)
	add_definitions(-DLUA_USE_POSIX)
endif()

# compiler parameters
if(MSVC)
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
	set(PROJECT_CXX_FLAGS /W4 /std:c++latest /wd4996)
	set(EXTERNALS_CXX_FLAGS /W0 /std:c++latest /wd4996 /wd4458)
	set(EXTERNALS_C_FLAGS /W0)

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -DDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
else()
	set(PROJECT_CXX_FLAGS -std=c++2a -Wall -Wextra -pedantic -Werror)
	set(EXTERNALS_CXX_FLAGS -std=c++2a)
	set(EXTERNALS_C_FLAGS "")

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# requirements
find_package(Python3 COMPONENTS Interpreter)

# code generation
execute_process(
	COMMAND ${Python3_EXECUTABLE} "${GAME_BASE_DIR}/tools/build/code_generate.py"
	WORKING_DIRECTORY ${GAME_BASE_DIR}
	COMMAND_ERROR_IS_FATAL ANY
)

include_directories(
	${EXTERNALS_DIR}/include
	${EXTERNALS_DIR}/sources
	${EXTERNALS_DIR}/sources/soa-sort/include
	${GAME_SRC_DIR}
	${ENGINE_SRC_DIR}
)

link_directories(
	${EXTERNALS_DIR}/lib/${BIN_PLATFORM}
)

function(add_folder_as_library folder_path name)
	file(GLOB_RECURSE TEMP_SRC RELATIVE "" FOLLOW_SYMLINKS "${folder_path}/*")
	add_library(${name} ${TEMP_SRC})
	target_precompile_headers(${name} PRIVATE "${ENGINE_SRC_DIR}/EngineCommon/precomp.h")
	set_target_properties(${name} PROPERTIES UNITY_BUILD ${ENABLE_UNITY_BUILD})
	target_compile_options(${name} PRIVATE ${PROJECT_CXX_FLAGS})
endfunction()

macro(mark_option_internal _var)
	set(${_var} ${${_var}} CACHE INTERNAL "hidden" FORCE)
endmacro(mark_option_internal _var)

# All the things that are going to be included anywhere and not related
# to any specific game
add_folder_as_library(${ENGINE_SRC_DIR}/EngineCommon EngineCommon)
# Data types that can be stored in engine components
add_folder_as_library(${ENGINE_SRC_DIR}/EngineData EngineData)
# Data types that can be stored in game-specific ECS components
add_folder_as_library(${GAME_SRC_DIR}/GameData GameData)
# Algorithms and types that don't have to be stored in ECS components (shared between games)
add_folder_as_library(${ENGINE_SRC_DIR}/EngineUtils EngineUtils)
# Algorithms and types that don't have to be stored in ECS components (specific to one game)
add_folder_as_library(${GAME_SRC_DIR}/GameUtils GameUtils)
# Hardware Access Layer (shared between games, can depend only on Engine common/data/utils)
add_folder_as_library(${ENGINE_SRC_DIR}/HAL Hal)
# ECS systems and glue code (engine specific)
add_folder_as_library(${ENGINE_SRC_DIR}/EngineLogic EngineLogic)
# ECS systems and glue code (specific to one game)
add_folder_as_library(${GAME_SRC_DIR}/GameLogic GameLogic)
if(BUILD_AUTO_TESTS)
	# Automatic tests that run predefined scenarios and validate against checklists
	add_folder_as_library(${GAME_SRC_DIR}/AutoTests AutoTests)
endif()

if(WEB_BUILD)
	file(GLOB_RECURSE APP_SRC RELATIVE "" FOLLOW_SYMLINKS "${GAME_SRC_DIR}/WebAssembly/*")
else()
	file(GLOB_RECURSE APP_SRC RELATIVE "" FOLLOW_SYMLINKS "${GAME_SRC_DIR}/GameMain/*")
endif()
add_executable(${APP_NAME} "${APP_SRC}")
target_compile_options(${APP_NAME} PRIVATE ${PROJECT_CXX_FLAGS})

# configure VS specific project parameters
if(MSVC)
	set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${GAME_BIN_DIR}")
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${APP_NAME})
endif()

if(NOT DISABLE_SDL)
	if(WEB_BUILD)
		# For web build we need to use SDL2, SDL2_image and SDL2_mixer ports for Emscripten
		set(USE_FLAGS "-sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='[\"png\"]' -sUSE_SDL_MIXER=2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
	else()
		# SDL2
		add_subdirectory(${EXTERNALS_DIR}/sources/SDL EXCLUDE_FROM_ALL)
		include_directories(${EXTERNALS_DIR}/sources/SDL/include)

		#SDL_image
		add_subdirectory(${EXTERNALS_DIR}/sources/SDL_image EXCLUDE_FROM_ALL)
		include_directories(${EXTERNALS_DIR}/sources/SDL_image)

		#SDL_mixer
		add_subdirectory(${EXTERNALS_DIR}/sources/SDL_mixer EXCLUDE_FROM_ALL)
		include_directories(${EXTERNALS_DIR}/sources/SDL_mixer/include)
	endif()
endif()

if(WEB_BUILD)
	set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS "--preload-file resources -s ASSERTIONS=1 -s GL_ASSERTIONS=1 -s LEGACY_GL_EMULATION=1")
endif()

# Lua
add_library(Lua EXCLUDE_FROM_ALL
	${EXTERNALS_DIR}/sources/lua/src/lapi.c
	${EXTERNALS_DIR}/sources/lua/src/lauxlib.c
	${EXTERNALS_DIR}/sources/lua/src/lbaselib.c
	${EXTERNALS_DIR}/sources/lua/src/lcode.c
	${EXTERNALS_DIR}/sources/lua/src/lcorolib.c
	${EXTERNALS_DIR}/sources/lua/src/lctype.c
	${EXTERNALS_DIR}/sources/lua/src/ldblib.c
	${EXTERNALS_DIR}/sources/lua/src/ldebug.c
	${EXTERNALS_DIR}/sources/lua/src/ldo.c
	${EXTERNALS_DIR}/sources/lua/src/ldump.c
	${EXTERNALS_DIR}/sources/lua/src/lfunc.c
	${EXTERNALS_DIR}/sources/lua/src/lgc.c
	${EXTERNALS_DIR}/sources/lua/src/linit.c
	${EXTERNALS_DIR}/sources/lua/src/liolib.c
	${EXTERNALS_DIR}/sources/lua/src/llex.c
	${EXTERNALS_DIR}/sources/lua/src/lmathlib.c
	${EXTERNALS_DIR}/sources/lua/src/lmem.c
	${EXTERNALS_DIR}/sources/lua/src/loadlib.c
	${EXTERNALS_DIR}/sources/lua/src/lobject.c
	${EXTERNALS_DIR}/sources/lua/src/lopcodes.c
	${EXTERNALS_DIR}/sources/lua/src/loslib.c
	${EXTERNALS_DIR}/sources/lua/src/lparser.c
	${EXTERNALS_DIR}/sources/lua/src/lstate.c
	${EXTERNALS_DIR}/sources/lua/src/lstring.c
	${EXTERNALS_DIR}/sources/lua/src/lstrlib.c
	${EXTERNALS_DIR}/sources/lua/src/ltable.c
	${EXTERNALS_DIR}/sources/lua/src/ltablib.c
	${EXTERNALS_DIR}/sources/lua/src/ltm.c
	${EXTERNALS_DIR}/sources/lua/src/lundump.c
	${EXTERNALS_DIR}/sources/lua/src/lutf8lib.c
	${EXTERNALS_DIR}/sources/lua/src/lvm.c
	${EXTERNALS_DIR}/sources/lua/src/lzio.c
)
add_dependencies(${APP_NAME} Lua)
include_directories(${EXTERNALS_DIR}/sources/lua/src)
target_compile_options(Lua PRIVATE ${EXTERNALS_C_FLAGS})

# ImGui
if(IMGUI_ENABLED)
	add_library(Imgui EXCLUDE_FROM_ALL
		${EXTERNALS_DIR}/sources/imgui/imgui.cpp
		${EXTERNALS_DIR}/sources/imgui/imgui_demo.cpp
		${EXTERNALS_DIR}/sources/imgui/imgui_draw.cpp
		${EXTERNALS_DIR}/sources/imgui/imgui_widgets.cpp
		${EXTERNALS_DIR}/sources/imgui/examples/imgui_impl_opengl2.cpp
		${EXTERNALS_DIR}/sources/imgui/examples/imgui_impl_sdl.cpp
	)
	add_dependencies(${APP_NAME} Imgui)
	include_directories(${EXTERNALS_DIR}/sources/imgui)
	target_compile_options(Imgui PRIVATE ${EXTERNALS_CXX_FLAGS})
endif()

# GameNetworkingSockets
if(NOT FAKE_NETWORK)
	include_directories(${EXTERNALS_DIR}/sources/GameNetworkingSockets/include)
	if(NOT WIN32)
		set(BUILD_STATIC_LIB OFF)
		add_subdirectory(${EXTERNALS_DIR}/sources/GameNetworkingSockets)
		# hide options of GameNetworkingSockets from project Cmake GUI
		mark_option_internal(BUILD_STATIC_LIB)
		mark_option_internal(BUILD_SHARED_LIB)
		mark_option_internal(BUILD_EXAMPLES)
		mark_option_internal(BUILD_TESTS)
		mark_option_internal(BUILD_TOOLS)
		mark_option_internal(LTO)
		mark_option_internal(ENABLE_ICE)
		mark_option_internal(USE_STEAMWEBRTC)
		mark_option_internal(Protobuf_USE_STATIC_LIBS)
		mark_option_internal(MSVC_CRT_STATIC)
		mark_option_internal(USE_CRYPTO)
		mark_option_internal(SANITIZE_ADDRESS)
		mark_option_internal(SANITIZE_MEMORY)
		mark_option_internal(SANITIZE_LINK_STATIC)
		mark_option_internal(SANITIZE_THREAD)
		mark_option_internal(SANITIZE_UNDEFINED)
	endif()
endif()

set(GENERAL_PROJECT_LIBS
	GameLogic
	EngineLogic
	Hal
	GameUtils
	EngineUtils
	GameData
	EngineData
	EngineCommon

	Lua
)

if (BUILD_AUTO_TESTS)
	set(GENERAL_PROJECT_LIBS
		AutoTests
		${GENERAL_PROJECT_LIBS}
	)
endif()

if(NOT FAKE_NETWORK)
	set(GENERAL_PROJECT_LIBS
		${GENERAL_PROJECT_LIBS}
		GameNetworkingSockets
	)
endif()

if(NOT DISABLE_SDL)
	if(NOT WEB_BUILD)
		set(GENERAL_PROJECT_LIBS
			${GENERAL_PROJECT_LIBS}
			SDL2::SDL2-static
			SDL2_image
			SDL2_mixer
			SDL2main
		)
	else()
		set(GENERAL_PROJECT_LIBS
			${GENERAL_PROJECT_LIBS}
			SDL2
			SDL2_image_png
			SDL2_mixer
		)
	endif()
endif()

if(IMGUI_ENABLED)
	set(GENERAL_PROJECT_LIBS
		${GENERAL_PROJECT_LIBS}
		Imgui
	)
endif()

if(NOT DISABLE_SDL)
	if(WIN32)
		set(GENERAL_PROJECT_LIBS
			${GENERAL_PROJECT_LIBS}
			opengl32
		)
	else()
		set(GENERAL_PROJECT_LIBS
			${GENERAL_PROJECT_LIBS}
			${SDL2_LIBRARY}
			${SDL2_IMAGE_LIBRARY}
			GL
		)
	endif()
endif()

if(WIN32)
	set(GENERAL_PROJECT_LIBS
		${GENERAL_PROJECT_LIBS}
		ws2_32
	)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(GENERAL_PROJECT_LIBS
		${GENERAL_PROJECT_LIBS}
		stdc++
	)
else()
	set(GENERAL_PROJECT_LIBS
		${GENERAL_PROJECT_LIBS}
		stdc++fs
	)
endif()

target_link_libraries(${APP_NAME} ${GENERAL_PROJECT_LIBS})

set_target_properties(${APP_NAME} PROPERTIES
	VERSION "${EXECUTABLE_VERSION}.${BUILD_NUMBER}"
	SOVERSION ${BUILD_NUMBER}
	EXPORT_NAME ${EXECUTABLE_NAME}
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${ads_PlatformDir}/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${ads_PlatformDir}/lib"
	RUNTIME_OUTPUT_DIRECTORY "${GAME_BIN_DIR}"
)

# Unit tests
if(BUILD_UNIT_TESTS)
	set(UNITTESTS_NAME UnitTests)
	file(GLOB_RECURSE UNITTESTS_SRC RELATIVE "" FOLLOW_SYMLINKS "${GAME_SRC_DIR}/UnitTests/*")
	set(UNITTESTS_SRC
		${UNITTESTS_SRC}
		${EXTERNALS_DIR}/sources/googletest/src/gtest-all.cc
	)
	include_directories(${EXTERNALS_DIR}/sources/googletest)
	add_executable(${UNITTESTS_NAME} ${UNITTESTS_SRC})
	target_compile_definitions(${UNITTESTS_NAME} PRIVATE -DRACCOON_ECS_TOOLMODE)
	target_compile_options(${UNITTESTS_NAME} PRIVATE ${PROJECT_CXX_FLAGS})
	if(WIN32)
		set_target_properties(${UNITTESTS_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${GAME_BIN_DIR}")
	endif()
	target_link_libraries(${UNITTESTS_NAME} ${GENERAL_PROJECT_LIBS})
endif()

# symlink resources dir
if(NOT EXISTS "${GAME_BIN_DIR}/resources" AND NOT WEB_BUILD)
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/resources" "${GAME_BIN_DIR}/resources"
		COMMAND_ERROR_IS_FATAL ANY
	)
endif()

if(WIN32)
	# copy dlls
	file(GLOB WIN32_DLLS
		"${EXTERNALS_DIR}/dll/${BIN_PLATFORM}/*"
	)
	file(COPY ${WIN32_DLLS} DESTINATION ${GAME_BIN_DIR})
endif()

# pre-compile additional code generation
add_custom_target(
	AdditionalCodeGeneration
	COMMAND ${Python3_EXECUTABLE} ${GAME_BASE_DIR}/tools/build/precompile_code_generate.py ${GAME_BASE_DIR}
	COMMENT "C++ pre-compile additional code generation"
)
add_dependencies(EngineCommon AdditionalCodeGeneration)
