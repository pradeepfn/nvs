include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(TEST_SOURCES
    ../src/px_log.h
    ../src/px_log.c
    ../src/px_util.h
    ../src/px_util.c
    ../src/px_debug.c
    ../src/px_debug.h
    ../src/px_types.h)

add_executable(log_test logtest.c ${TEST_SOURCES})
target_link_libraries(log_test ssl crypto)
add_executable(log_test_commit logtest_commit.c ${TEST_SOURCES})
target_link_libraries(log_test_commit ssl crypto)