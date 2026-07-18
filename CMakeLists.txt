cmake_minimum_required(VERSION 3.10)
project(orpheus VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(ENABLE_TESTS "Enable unit tests" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers for debugging" OFF)
option(ENABLE_FFMPEG "Enable FFmpeg for AAC/ALAC/M4A support" OFF)

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/ncurses")
    set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/ncurses/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endif()

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED IMPORTED_TARGET ncursesw)
if(NOT NCURSES_FOUND)
    pkg_check_modules(NCURSES REQUIRED IMPORTED_TARGET ncurses)
endif()
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)
pkg_check_modules(LUA REQUIRED IMPORTED_TARGET lua)

# Optional FFmpeg for AAC/ALAC/M4A
#if(ENABLE_FFMPEG)
#    pkg_check_modules(AVCODEC REQUIRED libavcodec)
#    pkg_check_modules(AVFORMAT REQUIRED libavformat)
#    pkg_check_modules(AVUTIL REQUIRED libavutil)
#    add_compile_definitions(ORPHEUS_FFMPEG)
#endif()

# Source files
set(SOURCES
    src/main.cpp
    src/ui/ui.cpp
    src/ui/art.cpp
    src/ui/visualizer.cpp
    src/player/player.cpp
    src/util/log.cpp
    src/util/util.cpp
    src/util/miniaudio.c
)

add_executable(orpheus ${SOURCES})

# Include directories
target_include_directories(orpheus PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(orpheus PRIVATE
    PkgConfig::NCURSES
    PkgConfig::LUA
    PkgConfig::TAGLIB
)

if(APPLE)
    target_link_libraries(orpheus PRIVATE
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
    )
    target_link_libraries(orpheus PRIVATE pthread m)
endif()

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
    target_compile_options(orpheus PRIVATE --coverage)
    target_link_options(orpheus PRIVATE --coverage)
endif()

if(ENABLE_SANITIZERS)
    target_compile_options(orpheus PRIVATE -fsanitize=address -fsanitize=undefined)
    target_link_options(orpheus PRIVATE -fsanitize=address -fsanitize=undefined)
endif()
