cmake_minimum_required(VERSION 3.10)
project(cbmp)

find_package(LIBMPDCLIENT REQUIRED)

add_executable(cbmp main.c)

target_include_directories(cbmp ${LIBMPDCLIENT_INCLUDE_DIRS})
