cmake_minimum_required(VERSION 3.10)

# Project name and version
project(orpheus VERSION 1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable debugging symbols (can be overridden)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Options for testing and code quality
option(ENABLE_TESTS "Enable unit tests" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers for debugging" OFF)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPDCLIENT REQUIRED libmpdclient)
pkg_check_modules(NCURSES REQUIRED ncurses)
pkg_check_modules(LUA REQUIRED lua)
pkg_check_modules(JPG REQUIRED libjpeg)

# Find AAlib (use libaa for pkg-config, with fallback)
pkg_check_modules(AALIB QUIET libaa)
if (NOT AALIB_FOUND)
    find_library(AALIB_LIBRARY NAMES aa libaa HINTS /usr/lib /usr/local/lib)
    find_path(AALIB_INCLUDE_DIR NAMES aalib.h HINTS /usr/include /usr/local/include)
    if (AALIB_LIBRARY AND AALIB_INCLUDE_DIR)
        set(AALIB_FOUND TRUE)
        set(AALIB_LIBRARIES ${AALIB_LIBRARY})
        set(AALIB_INCLUDE_DIRS ${AALIB_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "AAlib not found. Ensure aalib is installed (sudo pacman -S aalib).")
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${MPDCLIENT_INCLUDE_DIRS}
    ${NCURSES_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
    ${JPG_INCLUDE_DIRS}
    ${AALIB_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.c
    src/mpd_connections.c
    src/ui.c
    src/lua_config.c
		src/img_to_ascii.c
)

# Add executable
add_executable(orpheus ${SOURCES})

# Link libraries (include ncurses and slang for AAlib compatibility)
target_link_libraries(orpheus PRIVATE
    ${MPDCLIENT_LIBRARIES}
    ${NCURSES_LIBRARIES}
    ${LUA_LIBRARIES}
    ${JPG_LIBRARIES}
    ${AALIB_LIBRARIES}
    ncurses
    slang
)

# Define STB_IMAGE_IMPLEMENTATION for stb_image
target_compile_definitions(orpheus PRIVATE STB_IMAGE_IMPLEMENTATION)

# Compiler warnings and optimizations
target_compile_options(orpheus
    PRIVATE
        -Wall -Wextra -Wpedantic
)

# Set output directory for the executable
set_target_properties(orpheus
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Install target
install(TARGETS orpheus DESTINATION bin)

# Testing support for local development
if(ENABLE_TESTS)
    enable_testing()
    # Test setup will be added here when tests are implemented
endif()

# Code coverage support
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Sanitizer support for debugging
if(ENABLE_SANITIZERS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined")
endif()
