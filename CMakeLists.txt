cmake_minimum_required(VERSION 3.18)
project(storm-engine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(conan)
include(StormSetup)

option(STORM_ENABLE_CRASH_REPORTS "Enable automatic crash reports" OFF)
option(STORM_ENABLE_STEAM "Enable Steam integration" OFF)
option(STORM_ENABLE_SAFE_MODE "Enable additional runtime checks" OFF)
option(STORM_USE_CONAN_SDL "Use sdl from conan" ON)
if (NOT WIN32)
    option(STORM_MESA_NINE "Use Gallium Nine from Mesa for native D3D9 API" OFF)
endif()

### Set up output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)

set(STORM_WATERMARK_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/watermark.hpp CACHE FILEPATH "Include file containing build revision, etc." FORCE)

if (WIN32)
    set(SDL2_LIBRARIES "sdl")
else()
    include(linux)
endif()

### Set up third-party dependencies
set(ENV{CONAN_REVISIONS_ENABLED} 1)
conan_add_remote(NAME piratesahoy
    URL https://gitlab.com/api/v4/projects/54068744/packages/conan
    VERIFY_SSL True
)
normalize_booleans(STORM_ENABLE_CRASH_REPORTS STORM_ENABLE_STEAM STORM_USE_CONAN_SDL)
conan_cmake_run(CONANFILE conanfile.py
    BASIC_SETUP CMAKE_TARGETS
    BUILD missing
    OPTIONS
        output_directory=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        watermark_file=${STORM_WATERMARK_FILE}
        crash_reports=${STORM_ENABLE_CRASH_REPORTS}
        steam=${STORM_ENABLE_STEAM}
        conan_sdl=${STORM_USE_CONAN_SDL}
)

### Define library ALIASes for use without CONAN_PKG:: prefix
foreach (conan_target ${CONAN_TARGETS})
    string(REPLACE "CONAN_PKG::" "" unprefixed_target ${conan_target})
	add_library(${unprefixed_target} ALIAS ${conan_target})
endforeach()
###

### Set up build scripts
set(SRC_DIRS "src" CACHE STRING "File locations for public header files" FORCE)
set(PUBLIC_INCLUDE_DIRS "include" CACHE STRING "File locations for source files" FORCE)
set(TESTSUITE_DIRS "testsuite" CACHE STRING "File locations for tests" FORCE)
set(RESOURCE_DIRS "rsrc" CACHE STRING "File locations for tests" FORCE)

set(SOURCE_FILE_EXTENSIONS "*.cpp" CACHE STRING "File extensions for source files" FORCE)
set(HEADER_FILE_EXTENSIONS "*.h" "*.hpp" CACHE STRING "File extensions for header files" FORCE)
###

### Copy Shader Effects
file(GLOB_RECURSE glob_techniques LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src" "src/techniques/**/*.fx")
set(technique_files "")
foreach(effect_file ${glob_techniques})
#    message("${CMAKE_CURRENT_SOURCE_DIR}/src/${effect_file} -> ${CMAKE_CURRENT_BINARY_DIR}/bin/resource/${effect_file}")
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bin/resource/${effect_file}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/src/${effect_file}
            ${CMAKE_CURRENT_BINARY_DIR}/bin/resource/${effect_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/${effect_file}
    )
    list(APPEND technique_files ${CMAKE_CURRENT_BINARY_DIR}/bin/resource/${effect_file})
endforeach()
add_custom_target(engine_techniques DEPENDS ${technique_files})
###

add_subdirectory(src)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT engine)
