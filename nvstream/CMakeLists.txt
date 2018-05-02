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
include_directories(${PROJECT_SOURCE_DIR}/third-party/nvml/src/include)
link_directories(${PROJECT_SOURCE_DIR}/third-party/nvml/src/debug)

#version number
set (NVS_VERSION_MAJOR 1)
set (NVS_VERSION_MINOR 3)
#set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_BUILD_TYPE Debug)

#compiler flags
set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wno-write-strings")

include_directories("${PROJECT_BINARY_DIR}")

OPTION(FILE_STORE "enable file based checkpoint" OFF)
OPTION(NULL_STORE "enable no checkpoint" ON)
OPTION(DELTA_STORE "enable delta" OFF)
OPTION(MEMCPY "enable memcpy" OFF)
OPTION(PMFS "using PMFS" OFF)
OPTION(TMPFS "using TMPFS" OFF)



#add_definitions(-DNCHECKPT)
#add_definitions(-DNDEBUG)
add_definitions(-DDEBUG)
add_definitions(-DDEDUP)
add_definitions(-D_TIMING)
add_definitions(-DSTATS)


IF(FILE_STORE)
    add_definitions(-D_FILE_STORE)
    MESSAGE("file storage enabled")
ENDIF(FILE_STORE)

IF(PMFS)
    add_definitions(-D_PMFS)
    MESSAGE("pmfs as the file storage")
ENDIF(PMFS)

IF(TMPFS)
 add_definitions(-D_TMPFS)
 MESSAGE("tmpfs as the file system")
ENDIF(TMPFS)

IF(DELTA_STORE)
 add_definitions(-D_DELTA_STORE)
 MESSAGE("delta store enabled")
ENDIF(DELTA_STORE)

IF(MEMCPY)
 add_definitions(-D_MEMCPY)
 MESSAGE("memcpy without crash-consistency")
ENDIF(MEMCPY)

IF(NULL_STORE)
 add_definitions(-D_NULLS_TORE)
 MESSAGE("no checkpoints")
ENDIF(NULL_STORE)


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





