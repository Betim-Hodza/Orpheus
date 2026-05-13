cmake_minimum_required(VERSION 3.10)
project(orpheus VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(ENABLE_TESTS "Enable unit tests" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers for debugging" OFF)
option(ENABLE_FFMPEG "Enable FFmpeg for AAC/ALAC/M4A support" OFF)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED ncurses)
pkg_check_modules(LUA REQUIRED lua)
pkg_check_modules(TAGLIB REQUIRED taglib)

# Optional FFmpeg for AAC/ALAC/M4A
if(ENABLE_FFMPEG)
    pkg_check_modules(AVCODEC REQUIRED libavcodec)
    pkg_check_modules(AVFORMAT REQUIRED libavformat)
    pkg_check_modules(AVUTIL REQUIRED libavutil)
    add_compile_definitions(ORPHEUS_FFMPEG)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/include
    ${NCURSES_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
)

# Source files — new directory structure
set(SOURCES
    src/main.cpp
    src/ui/img_to_ascii.cpp
    src/ui/ui.cpp
    src/util/log.cpp
    src/util/lua_config.cpp
    src/util/mpd_connections.cpp
    src/util/util.cpp
)

add_executable(orpheus ${SOURCES})

# miniaudio: header-only, implementation defined in player.cpp only
set_source_files_properties(src/player/player.cpp PROPERTIES
    COMPILE_DEFINITIONS "MINIAUDIO_IMPLEMENTATION"
)
# STB: same deal, defined in img_to_ascii.cpp only
set_source_files_properties(src/ui/img_to_ascii.cpp PROPERTIES
    COMPILE_DEFINITIONS "STB_IMAGE_IMPLEMENTATION"
)

# Link libraries
target_link_libraries(orpheus PRIVATE
    ${NCURSES_LIBRARIES}
    ${LUA_LIBRARIES}
    ${TAGLIB_LIBRARIES}
    ncurses
)
if(ENABLE_FFMPEG)
    target_include_directories(orpheus PRIVATE
        ${AVCODEC_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
    )
    target_link_libraries(orpheus PRIVATE
        ${AVCODEC_LIBRARIES}
        ${AVFORMAT_LIBRARIES}
        ${AVUTIL_LIBRARIES}
    )
endif()

target_compile_options(orpheus PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(orpheus PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS orpheus DESTINATION bin)

if(ENABLE_TESTS)
    enable_testing()
endif()

if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

if(ENABLE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined")
endif()
