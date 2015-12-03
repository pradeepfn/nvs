include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(TEST_SOURCES
    ../src/px_log.h
    ../src/px_log.c
    ../src/px_util.h
    ../src/px_util.c
    logtest.c)

add_executable(log_test ${TEST_SOURCES})