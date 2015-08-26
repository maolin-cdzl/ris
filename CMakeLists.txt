CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(ris)

set(CMAKE_C_FLAGS "-std=c99 -fPIC -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC -fvisibility=hidden ${CMAKE_CXX_FLAGS}")

add_library(snapshot STATIC
	src/snapshot/snapshot.cxx
	src/snapshot/snapshotservice.cxx
	src/snapshot/snapshotserviceworker.cxx
	src/snapshot/snapshotclient.cxx
	include/ris/snapshot/snapshotable.h
	include/ris/snapshot/snapshot.h
	include/ris/snapshot/snapshotservice.h
	include/ris/snapshot/snapshotserviceworker.h
	include/ris/snapshot/snapshotbuilder.h
)

target_include_directories(snapshot
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(region SHARED
	src/zmqhelper.c
	src/ritypes.cxx
	src/region/regionactor.cxx
	src/region/regiontable.cxx
	src/region/publisher.cxx
	src/region/regiondll.cxx
	include/ris/region/publisher.h
	include/ris/region/regionactor.h
	include/ris/region/regiontable.h
	include/ris/region/regiondll.h
)

target_include_directories(region
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(region PRIVATE LIBREGION_BUILDING)

target_link_libraries(region PUBLIC
	snapshot
	czmq
	zmq
	config++
	glog
)


