
set(YUMA_TEST_SRC
        ${YUMA_TEST_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/test.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/test.h
        )

add_library(yuma_test STATIC ${YUMA_TEST_SRC})
target_link_libraries(yuma_test boost_thread boost_log boost_filesystem boost_system pmemobj pmemlog yuma)

function (add_yuma_test file_name)
    add_executable(${file_name} ${file_name}.cc)

    target_link_libraries(${file_name} yuma_test pthread gtest ${Boost_LOG_LIBRARY} boost_system)
    target_link_libraries(${file_name} ${ARCH_LIBS})
    add_test(NAME ${file_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${file_name})
endfunction()


add_subdirectory(memory_manager)
