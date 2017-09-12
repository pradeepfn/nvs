
set(YUMA_SRC
        ${YUMA_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/runtimeManager.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/store.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/key.h
        ${CMAKE_CURRENT_SOURCE_DIR}/key.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/version.h
        ${CMAKE_CURRENT_SOURCE_DIR}/version.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/serializationTypes.h
        PARENT_SCOPE
        )