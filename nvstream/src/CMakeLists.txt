add_subdirectory(client)
add_subdirectory(common)
add_subdirectory(ns)
add_subdirectory(runtime)
add_subdirectory(sheap)

#we populate YUMA_SRC variable within sub-directories

add_library(yuma STATIC ${YUMA_SRC})
target_link_libraries(yuma boost_thread boost_log)