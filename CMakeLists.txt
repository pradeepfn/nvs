cmake_minimum_required(VERSION 2.8)
project(phoenix)

#versin number
set (PHOENIX_VERSION_MAJOR 1)
set (PHOENIX_VERSION_MINOR 3)
#set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_BUILD_TYPE Debug)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/px_config.h.in"
  "${PROJECT_BINARY_DIR}/px_config.h"
  )
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")
include_directories("/usr/lib/mpich/include")


set(HEADERS 
    include/phoenix.h
    include/timecount.h
    include/px_remote.h)


include_directories("include")

find_package(MPI REQUIRED)

set(MPI_C_COMPILER mpicc)
add_definitions(-DNDEBUG)
add_definitions(-DTIMING)
#add_definitions(-DPX_DIGEST)


include_directories(${MPI_INCLUDE_PATH})
include_directories("/home1/03528/pradeepf/armci_install/include")

set(CMAKE_C_FLAGS "-Wall -g -O0")

set(SOURCE_FILES
    src/px_types.h
    src/px_checkpoint.c
    src/px_constants.h
    src/px_debug.c
    src/px_debug.h
    src/px_log.c
    src/px_log.h
    src/px_dlog.h
    src/px_dlog.c
    src/px_read.c
    src/px_read.h
    src/px_remote.c
    src/px_remote.h
    src/px_util.c
    src/px_mpiutil.c
    src/px_util.h
    src/rmtchkpt.c
    src/rmtchkpt.h
    src/timecount.c
    src/uthash.h
    src/px_allocate.c
    src/px_allocate.h
    src/px_sampler.c
    src/px_sampler.h
    src/px_threadpool.h
    src/px_threadpool.c
    src/px_earlycopy.h
    src/px_timer.h)

SET(MY_OWN_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to
install directories")
SET(CMAKE_INSTALL_PREFIX "${MY_OWN_INSTALL_PREFIX}" CACHE INTERNAL "Prefix
prepended to install directories" FORCE)

add_library(phoenix STATIC ${SOURCE_FILES})
install(TARGETS phoenix DESTINATION lib)


 add_subdirectory(tests)

 enable_testing()
 add_test(NAME log_test COMMAND log_test)
 add_test(NAME log_test_commit COMMAND log_test_commit)
