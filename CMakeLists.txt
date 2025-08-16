cmake_minimum_required(VERSION 3.10)
# Project name and version
project(orpheus VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable debugging symbols
set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MPDCLIENT REQUIRED libmpdclient)
pkg_check_modules(NCURSES REQUIRED ncurses)
pkg_check_modules(LUA REQUIRED lua)  # Adjust to your Lua version if needed, e.g., lua5.4
pkg_check_modules(JPG REQUIRED libjpeg)  # Use libjpeg instead of libjpeg-turbo

include_directories(${CMAKE_SOURCE_DIR}/include ${MPDCLIENT_INCLUDE_DIRS} ${NCURSES_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS} ${JPG_INCLUDE_DIRS})

set(SOURCES
    src/main.c
    src/mpd_connections.c
    src/ui.c
    src/ascii_art.c
    src/lua_config.c
)

add_executable(orpheus ${SOURCES})

target_link_libraries(orpheus PRIVATE ${MPDCLIENT_LIBRARIES} ${NCURSES_LIBRARIES} ${LUA_LIBRARIES} ${JPG_LIBRARIES})

install(TARGETS orpheus DESTINATION bin)

cmake_minimum_required(VERSION 3.10)


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