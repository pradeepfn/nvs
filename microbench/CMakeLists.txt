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

set(SEQUENTIAL_W_SOURCES src/seq_writer.cc)
set(SEQUENTIAL_DELTAW_SOURCES src/seq_dwriter.cc)
set(SEQUENTIAL_R_SOURCES src/seq_reader.cc)
set(PSEQUENTIAL_R_SOURCES src/pseq_reader.cc)
#set(PARALLEL_SOURCES src/parallel.c)

add_executable(seqwriter
        ${SEQUENTIAL_W_SOURCES})

add_executable(seqdwriter
        ${SEQUENTIAL_DELTAW_SOURCES})  
        
add_executable(seqreader
        ${SEQUENTIAL_R_SOURCES})

add_executable(pseqreader
        ${PSEQUENTIAL_R_SOURCES})

target_link_libraries(seqdwriter
        ${PROJECT_SOURCE_DIR}/../nvstream/build/src/libyuma.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemlog.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemobj.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmem.a
        pthread
        boost_thread boost_log boost_filesystem boost_system rt dl)


target_link_libraries(seqwriter
        ${PROJECT_SOURCE_DIR}/../nvstream/build/src/libyuma.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemlog.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemobj.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmem.a
        pthread
        boost_thread boost_log boost_filesystem boost_system rt dl)

target_link_libraries(seqreader
        ${PROJECT_SOURCE_DIR}/../nvstream/build/src/libyuma.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemlog.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemobj.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmem.a
        pthread
        boost_thread boost_log boost_filesystem boost_system  rt dl)


target_link_libraries(pseqreader
        ${PROJECT_SOURCE_DIR}/../nvstream/build/src/libyuma.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemlog.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmemobj.a
        ${PROJECT_SOURCE_DIR}/../nvstream/third-party/nvml/src/debug/libpmem.a
        pthread
        boost_thread boost_log boost_filesystem boost_system  rt dl)

#add_executable(parallel
#        ${PARALLEL_SOURCES})

#target_link_libraries(parallel pthread)
