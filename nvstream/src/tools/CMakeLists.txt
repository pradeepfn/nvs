
set(NVS_HFORMATTER_SRC
        heapFormatter.cc
        ../common/log.cc
        ../sheap/layout.h
        )

find_package(Boost 1.55 COMPONENTS log filesystem thread system    thread
   filesystem
   chrono
   system
   date_time
   atomic
   log_setup
   log
REQUIRED)

message(STATUS "Boost libraries is 	${Boost_LIBRARIES} \n")
add_executable(nvsformat ${NVS_HFORMATTER_SRC})
target_link_libraries(nvsformat
        pthread
        boost_filesystem
        boost_system
	${Boost_LIBRARIES} 
        boost_log
        pmemobj
        pmemlog
        yuma
        dl	${Boost_LIBRARIES} 
)
