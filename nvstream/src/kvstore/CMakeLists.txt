
set(YUMA_SRC
        ${YUMA_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/delta_store.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/delta_store.h
        ${CMAKE_CURRENT_SOURCE_DIR}/nvs_store.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/nvs_store.h
        ${CMAKE_CURRENT_SOURCE_DIR}/object.h
        ${CMAKE_CURRENT_SOURCE_DIR}/object.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/constants.h
        ${CMAKE_CURRENT_SOURCE_DIR}/file_store.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/store_manager.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/timing_store.h
        PARENT_SCOPE
        )