cmake_minimum_required(VERSION 2.8)
project(microbench)

#versin number
set (MICROBENCH_VERSION_MAJOR 1)
set (MICROBENCH_VERSION_MINOR 3)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wno-write-strings")

include_directories(${PROJECT_SOURCE_DIR}/../nvstream/include)
link_directories(${PROJECT_SOURCE_DIR}/../nvstream/build/src)
link_directories(${PROJECT_SOURCE_DIR}/../third-party/nvml/src/debug)
#
# enable boost
#
find_package(Boost 1.55 COMPONENTS log REQUIRED)

#add_definitions(-DNDEBUG)
#set(CMAKE_C_FLAGS "-O2 -Wall")

set(SEQUENTIAL_SOURCES src/seq_writer.cc)
#set(PARALLEL_SOURCES src/parallel.c)

add_executable(sequential
        ${SEQUENTIAL_SOURCES})

target_link_libraries(sequential
        ${PROJECT_SOURCE_DIR}/../nvstream/build/src/libyuma.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmem.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemlog.a
        boost_thread boost_log boost_filesystem boost_system)

#add_executable(parallel
#        ${PARALLEL_SOURCES})

#target_link_libraries(parallel pthread)
