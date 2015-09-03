CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(ris)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(CMakeProtobuf)

set(CMAKE_C_FLAGS "-std=c99 -fPIC -Wall -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC -Wall -fvisibility=hidden ${CMAKE_CXX_FLAGS}")


# configure risproto
set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto)

set(RIS_PROTO 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/regionapi.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/regionpub.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/snapshot.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/trackerapi.proto 
)

build_pb_cxx_sources(${CMAKE_CURRENT_BINARY_DIR}/include/ris ${CMAKE_CURRENT_BINARY_DIR}/src RIS_PROTO_SRCS RIS_PROTO_HDRS ${RIS_PROTO})
add_library(risproto STATIC
	${RIS_PROTO_SRCS}
	${RIS_PROTO_HDRS}
)
target_include_directories(risproto
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ris
)

target_link_libraries(risproto PUBLIC 
	protobuf
)

# configure snapshot
add_library(snapshot STATIC
	src/snapshot/snapshotservice.cxx
	src/snapshot/snapshotserviceworker.cxx
	src/snapshot/snapshotclient.cxx
	include/snapshot/snapshotable.h
	include/snapshot/snapshotservice.h
	include/snapshot/snapshotserviceworker.h
	include/snapshot/snapshotbuilder.h
)

target_include_directories(snapshot
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC $<TARGET_PROPERTY:risproto,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(snapshot LINK_PUBLIC 
	zmqx
	risproto
	zmq
	czmq
	glog
	gflags
)

# target ris

add_library(ris STATIC
	src/ritypes.cxx
	include/ris/ritypes.h
	include/ris/riobserver.h
)

target_include_directories(ris
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ris LINK_PUBLIC 
	snapshot
)

# region

add_library(region STATIC
	src/region/regionactor.cxx
	src/region/regiontable.cxx
	src/region/publisher.cxx
	src/region/regionctx.cxx
	include/region/publisher.h
	include/region/regionactor.h
	include/region/regiontable.h
	include/region/regionctx.h
)

target_include_directories(region
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ris
)

target_link_libraries(region LINK_PUBLIC
	risproto
	snapshot
	ris
	zmqx
	config++
)


# regionapi

add_library(regionapi SHARED
	regionapi/regionapi.h
	regionapi/regionapi.cxx
)

target_include_directories(regionapi
	PRIVATE $<TARGET_PROPERTY:region,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(regionapi PRIVATE REGIONAPI_BUILDING)

target_link_libraries(regionapi LINK_PUBLIC
	region
)

add_library(regionapi_static STATIC 
	regionapi/regionapi.h
	regionapi/regionapi.cxx
)

target_include_directories(regionapi_static
	PRIVATE $<TARGET_PROPERTY:region,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(regionapi_static PRIVATE REGIONAPI_BUILDING)

target_link_libraries(regionapi_static LINK_PUBLIC
	region
)

# tracker
add_library(tracker STATIC
	src/tracker/subscriber.cxx
	src/tracker/subcacher.cxx
	src/tracker/trackertable.cxx
	src/tracker/fromregionfactory.cxx
	src/tracker/trackeractor.cxx
	src/tracker/trackerctx.cxx
	include/tracker/subscriber.h
	include/tracker/subcacher.h
	include/tracker/trackertable.h
	include/tracker/fromregionfactory.h
	include/tracker/trackeractor.h
	include/tracker/trackerctx.h
)

target_include_directories(tracker
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ris
)

target_link_libraries(tracker LINK_PUBLIC
	risproto
	snapshot
	ris
	zmqx
	config++
)


# trackerapi
add_library(trackerapi SHARED
	trackerapi/trackerapi.h
	trackerapi/trackerapi.cxx
)

target_include_directories(trackerapi
	PRIVATE $<TARGET_PROPERTY:tracker,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(trackerapi PRIVATE TRACKERAPI_BUILDING)

target_link_libraries(trackerapi LINK_PUBLIC
	tracker
)

add_library(trackerapi_static STATIC
	trackerapi/trackerapi.h
	trackerapi/trackerapi.cxx
)

target_include_directories(trackerapi_static
	PRIVATE $<TARGET_PROPERTY:tracker,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(trackerapi_static PRIVATE TRACKERAPI_BUILDING)

target_link_libraries(trackerapi_static LINK_PUBLIC
	tracker
)


# broker
add_executable(ribroker src/broker/ribroker.cxx)
target_link_libraries(ribroker PRIVATE zmq gflags)

# test project
include_directories(googletest/googletest/include)
include_directories(googletest/googlemock/include)
add_subdirectory(googletest/googlemock)
add_subdirectory(zmqx)
add_subdirectory(test)
