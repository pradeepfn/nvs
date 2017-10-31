cmake_minimum_required(VERSION 2.8)
project(nvstream)

#
# default system include and link directories
#
link_directories(/usr/lib)
include_directories(/usr/include)
link_directories(/usr/local/lib)
include_directories(/usr/local/include)

# nvml directories
include_directories(${PROJECT_SOURCE_DIR}/../../nvml/src/include)
link_directories(${PROJECT_SOURCE_DIR}/../../nvml/src/debug)

#version number
set (NVS_VERSION_MAJOR 1)
set (NVS_VERSION_MINOR 3)
#set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_BUILD_TYPE Debug)

#compiler flags
set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wno-write-strings")

include_directories("${PROJECT_BINARY_DIR}")

#add_definitions(-DNCHECKPT)
#add_definitions(-DNDEBUG)
add_definitions(-DDEDUP)
add_definitions(-DTIMING)
add_definitions(-DSTATS)


#tests
enable_testing()


#
# enable boost
#
find_package(Boost 1.55 COMPONENTS log REQUIRED)

# for boost::log
add_definitions(-DBOOST_LOG_DYN_LINK)

#include directories
include_directories(${PROJECT_SOURCE_DIR}/include) #public headers
include_directories(${PROJECT_SOURCE_DIR}/src) #private headers
include_directories(${PROJECT_SOURCE_DIR}/test) #test headers


#
# third-party libraries
#
set(GTEST_ROOT ${PROJECT_SOURCE_DIR}/third-party/gtest)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
add_subdirectory(third-party)
include_directories(${PROJECT_SOURCE_DIR}/third-party/gtest/include)




add_subdirectory(src)
add_subdirectory(test)
#add_subdirectory(example)
add_subdirectory(include)




