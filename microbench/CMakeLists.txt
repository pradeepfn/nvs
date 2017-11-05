cmake_minimum_required(VERSION 2.8)
project(microbench)

#versin number
set (MICROBENCH_VERSION_MAJOR 1)
set (MICROBENCH_VERSION_MINOR 3)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS "-g -std=c++11 -Wno-write-strings")

include_directories(${PROJECT_SOURCE_DIR}/../nvstream/include)
link_directories(${PROJECT_SOURCE_DIR}/../nvstream/build/src)


#add_definitions(-DNDEBUG)
#set(CMAKE_C_FLAGS "-O2 -Wall")

set(SEQUENTIAL_SOURCES src/seq_writer.cc)
#set(PARALLEL_SOURCES src/parallel.c)



add_executable(sequential
        ${SEQUENTIAL_SOURCES})
target_link_libraries(sequential yuma)
#add_executable(parallel
#        ${PARALLEL_SOURCES})

#target_link_libraries(parallel pthread)
