cmake_minimum_required(VERSION 2.8)
project(yuma)

#
# default system include and link directories
#
link_directories(/usr/lib)
include_directories(/usr/include)
link_directories(/usr/local/lib)
include_directories(/usr/local/include)

#gull libs and include directories
include_directories(${PROJECT_SOURCE_DIR}/../gull/include)
link_directories(${PROJECT_SOURCE_DIR}/../gull/build/src)

#version number
set (YUMA_VERSION_MAJOR 1)
set (YUMA_VERSION_MINOR 3)
#set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_BUILD_TYPE Debug)

#compiler flags
set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wno-write-strings")

include_directories("${PROJECT_BINARY_DIR}")

#add_definitions(-DNCHECKPT)
add_definitions(-DNDEBUG)
add_definitions(-DDEDUP)
add_definitions(-DTIMING)
add_definitions(-DSTATS)


#include directories
include_directories(${PROJECT_SOURCE_DIR}/include) #public headers
include_directories(${PROJECT_SOURCE_DIR}/src) #private headers
include_directories(${PROJECT_SOURCE_DIR}/test) #test headers

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(include)




