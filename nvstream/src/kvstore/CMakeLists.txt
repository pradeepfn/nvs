
set(YUMA_SRC
        ${YUMA_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/nvs_store.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/nvs_store.h
        ${CMAKE_CURRENT_SOURCE_DIR}/key.h
        ${CMAKE_CURRENT_SOURCE_DIR}/key.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/object.h
        ${CMAKE_CURRENT_SOURCE_DIR}/object.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/version.h
        ${CMAKE_CURRENT_SOURCE_DIR}/version.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/serializationTypes.h
        ${CMAKE_CURRENT_SOURCE_DIR}/constants.h
        ${CMAKE_CURRENT_SOURCE_DIR}/file_store.cc
        PARENT_SCOPE
        )