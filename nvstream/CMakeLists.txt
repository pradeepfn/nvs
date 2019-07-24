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
set(CMAKE_CXX_FLAGS "-fpic -g -std=c++11 -Wno-write-strings")

include_directories("${PROJECT_BINARY_DIR}")

OPTION(FILE_STORE "enable file based checkpoint" OFF)
OPTION(NULL_STORE "enable no-checkpoint" OFF)
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
 add_definitions(-D_NULL_STORE)
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

# Some boilerplate to setup nice output directories
set(CMAKE_INSTALL_BINDIR bin CACHE STRING "Installation runtime subdirectory")
set(CMAKE_INSTALL_LIBDIR lib CACHE STRING "Installation library subdirectory")
set(CMAKE_INSTALL_INCLUDEDIR include
  CACHE STRING "Installation include subdirectory")
if(WIN32 AND NOT CYGWIN)
  set(CMAKE_INSTALL_CMAKEDIR CMake
    CACHE STRING "Installation CMake subdirectory")
else()
  set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/ffs
    CACHE STRING "Installation CMake subdirectory")
endif()
mark_as_advanced(CMAKE_INSTALL_BINDIR)
mark_as_advanced(CMAKE_INSTALL_LIBDIR)
mark_as_advanced(CMAKE_INSTALL_INCLUDEDIR)
mark_as_advanced(CMAKE_INSTALL_CMAKEDIR)

list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
  ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
  ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif()

list(INSERT CMAKE_PREFIX_PATH 0 ${CMAKE_INSTALL_PREFIX})

# Default to a RelWithDebInfo build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE RelWithDebugInfo)
endif()

include(CMakeDependentOption)


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





