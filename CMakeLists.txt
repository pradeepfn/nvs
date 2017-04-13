cmake_minimum_required(VERSION 2.8)
project(phoenix)

#versin number
set (PHOENIX_VERSION_MAJOR 1)
set (PHOENIX_VERSION_MINOR 3)
#set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_BUILD_TYPE Debug)

# configure a header file to pass some of the CMake settings
# to the source code
#configure_file (
#	"${PROJECT_SOURCE_DIR}/src/px_config.h.in"
#	"${PROJECT_BINARY_DIR}/px_config.h"
#	)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")


set(HEADERS
	include/phoenix.h)



add_definitions(-DNDEBUG)
add_definitions(-DDEDUP)
add_definitions(-DTIMING)
add_definitions(-DSTATS)


include_directories("include")

set(CMAKE_CC_FLAGS "-Wall -g")

set(SOURCE_FILES
	src/phoenix.cc
	src/px_types.h
	src/px_debug.h
	src/px_debug.cc
	src/px_log.h
	src/px_log.cc
	src/px_util.h
	src/px_util.cc
	src/uthash.h
	src/px_io.h
	src/px_timer.h
	src/px_constants.h)

set(PHXFORMAT_SOURCE_FILES
	src/phoenix.cc
	src/px_types.h
	src/format.cc
	src/px_debug.h
	src/px_debug.cc
	src/px_log.cc
	src/px_util.h
	src/px_util.cc)

set(MPIFORMAT_SOURCE_FILES
	src/phoenix.cc
	src/px_types.h
	src/mpi_format.cc
	src/px_debug.h
	src/px_debug.cc
	src/px_log.cc
	src/px_util.h
	src/px_io.h
	src/px_timer.h
	src/px_util.cc)



SET(MY_OWN_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to
install directories")
SET(CMAKE_INSTALL_PREFIX "${MY_OWN_INSTALL_PREFIX}" CACHE INTERNAL "Prefix
prepended to install directories" FORCE)

add_library(phoenix STATIC ${SOURCE_FILES})
install(TARGETS phoenix DESTINATION lib)

#add_executable(phxformat ${PHXFORMAT_SOURCE_FILES})
#target_link_libraries(phxformat pthread)
#target_link_libraries(phxformat ssl)
#install(TARGETS phxformat DESTINATION bin)

#adding mpi version of phxformat
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
add_executable(mpiformat ${MPIFORMAT_SOURCE_FILES})
target_link_libraries(mpiformat ${MPI_LIBRARIES})
target_link_libraries(mpiformat pthread)
target_link_libraries(mpiformat ssl)
target_link_libraries(mpiformat rt)
install(TARGETS mpiformat DESTINATION bin)

# adding basic testing functionality
add_subdirectory(test)
enable_testing()
add_test(NAME Basic-functionality COMMAND BasicTest) 

#get verbose output during test failures
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
