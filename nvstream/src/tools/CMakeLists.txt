
set(NVS_HFORMATTER_SRC
        heapFormatter.cc
        ../common/log.cc
        ../sheap/layout.h
        )

find_package(Boost 1.55 COMPONENTS log REQUIRED)

add_executable(nvsformat ${NVS_HFORMATTER_SRC})
target_link_libraries(nvsformat
        pthread
        boost_log
        boost_filesystem
        boost_system
        pmemobj
        pmemlog
        yuma
        dl)