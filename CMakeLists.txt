cmake_minimum_required(VERSION 3.10)

# Project name and version
project(orpheus VERSION 1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable debugging symbols
set(CMAKE_BUILD_TYPE Debug)

# Find libmpdclient
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMPDCLIENT REQUIRED libmpdclient)
if(NOT LIBMPDCLIENT_FOUND)
    message(FATAL_ERROR "libmpdclient not found. Please install libmpdclient-dev (Debian/Ubuntu) or libmpdclient-devel (Fedora).")
endif()

# Find ncurses
find_package(Curses REQUIRED)
if(NOT CURSES_FOUND)
    message(FATAL_ERROR "ncurses not found. Please install libncurses5-dev (Debian/Ubuntu) or ncurses-devel (Fedora).")
endif()

# Find Lua 5.3
find_package(Lua 5.3 REQUIRED)
if(NOT LUA_FOUND)
    message(FATAL_ERROR "Lua 5.3 not found. Please install liblua5.3-dev (Debian/Ubuntu) or lua-devel (Fedora).")
endif()

# Add executable
add_executable(orpheus src/main.c)

# Include directories
target_include_directories(orpheus
    PRIVATE
        ${LIBMPDCLIENT_INCLUDE_DIRS}
        ${CURSES_INCLUDE_DIR}
        ${LUA_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(orpheus
    PRIVATE
        ${LIBMPDCLIENT_LIBRARIES}
        ${CURSES_LIBRARIES}
        ${LUA_LIBRARIES}
)

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
