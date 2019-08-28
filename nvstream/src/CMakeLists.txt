
add_subdirectory(common)
add_subdirectory(kvstore)
add_subdirectory(sheap)
add_subdirectory(tools)
#we populate YUMA_SRC variable within sub-directories

# IDE support
set(MY_HEADERS
        ../include/nvs/log_id.h
        ../include/nvs/global_ptr.h
        ../include/nvs/memory_manager.h
        ../include/nvs/store.h
        ../include/nvs/log.h
        ../include/nvs/errorCode.h
        ../include/nvs/store_manager.h
        kvstore/logentry.h
        kvstore/file_store.h)

add_library(yuma STATIC ${YUMA_SRC} ${MY_HEADERS})
add_library(nvsshared SHARED ${YUMA_SRC} ${MY_HEADERS})
target_link_libraries(yuma boost_filesystem boost_thread boost_log boost_system)
