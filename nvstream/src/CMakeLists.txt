cmake_minimum_required(VERSION 3.10)
FIND_PACKAGE( Boost 1.40 COMPONENTS thread filesystem log  REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

INCLUDE_DIRECTORIES( ../include . )

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
LINK_DIRECTORIES(${Boost_LIBRARY_DIR})
target_link_libraries(yuma boost_filesystem boost_thread boost_log boost_system  ${Boost_LIBRARIES} pmemobj pmemlog)

install(FILES ${MY_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nvs)
install(TARGETS yuma nvsshared
  EXPORT nvstream-targets
  RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"          COMPONENT bin
  LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"          COMPONENT shlib
  ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"          COMPONENT lib
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/nvs" COMPONENT dev)

