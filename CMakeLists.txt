cmake_minimum_required(VERSION 3.2)
project(phoenix)

#versin number
set (PHOENIX_VERSION_MAJOR 1)
set (PHOENIX_VERSION_MINOR 3)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/px_config.h.in"
  "${PROJECT_BINARY_DIR}/px_config.h"
  )
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")

set(HEADERS 
    include/phoenix.h
    include/timecount.h
    include/px_remote.h)

include_directories("include")

find_package(MPI REQUIRED)

include_directories(${MPI_INCLUDE_PATH})
include_directories("/home1/03528/pradeepf/armci_install/include")

set(CMAKE_C_FLAGS "-Wall -g")

set(SOURCE_FILES
    src/px_checkpoint.c
    src/px_checkpoint.h
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
    src/px_util.h
    src/rmtchkpt.c
    src/rmtchkpt.h
    src/timecount.c
    src/uthash.h)

SET(MY_OWN_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to
install directories")
SET(CMAKE_INSTALL_PREFIX "${MY_OWN_INSTALL_PREFIX}" CACHE INTERNAL "Prefix
prepended to install directories" FORCE)

add_library(phoenix STATIC ${SOURCE_FILES})
install(TARGETS phoenix DESTINATION lib)
