cmake_minimum_required(VERSION 2.8)
project(mmapbench)

#versin number
set (MMAPBENCH_VERSION_MAJOR 1)
set (MMAPBENCH_VERSION_MINOR 3)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DNDEBUG)
add_definitions(-D__YUMA)
#add_definitions(-D__YUMA_STREAM)
set(CMAKE_C_FLAGS "-O2 -Wall")

set(SOURCES
        mmapbench.c)

add_executable(bench ${SOURCES})
target_link_libraries(bench pthread)
