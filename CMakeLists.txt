CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(ris)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(CMakeProtobuf)


set(CMAKE_C_FLAGS "-std=c99 -fPIC -g -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC -g -fvisibility=hidden ${CMAKE_CXX_FLAGS}")

set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto)

set(SNAPSHOT_PROTO 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/snapshot.proto 
)
build_pb_cxx_sources(${CMAKE_CURRENT_BINARY_DIR}/include/ris ${CMAKE_CURRENT_BINARY_DIR}/src SNAPSHOT_PROTO_SRCS SNAPSHOT_PROTO_HDRS ${SNAPSHOT_PROTO})

add_library(snapshot STATIC
	src/snapshot/snapshot.cxx
	src/snapshot/snapshotservice.cxx
	src/snapshot/snapshotserviceworker.cxx
	src/snapshot/snapshotclient.cxx
	${SNAPSHOT_PROTO_SRCS}
	include/ris/snapshot/snapshotable.h
	include/ris/snapshot/snapshot.h
	include/ris/snapshot/snapshotservice.h
	include/ris/snapshot/snapshotserviceworker.h
	include/ris/snapshot/snapshotbuilder.h
	${SNAPSHOT_PROTO_HDRS}
)

target_include_directories(snapshot
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ris
)


# regionactor

set(REGION_ACTOR_PROTO 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/regionapi.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/regionpub.proto 
)

build_pb_cxx_sources(${CMAKE_CURRENT_BINARY_DIR}/include/ris ${CMAKE_CURRENT_BINARY_DIR}/src REGION_ACTOR_PROTO_SRCS REGION_ACTOR_PROTO_HDRS ${REGION_ACTOR_PROTO})

add_library(regionactor STATIC
	src/ritypes.cxx
	src/region/regionactor.cxx
	src/region/regiontable.cxx
	src/region/publisher.cxx
	${REGION_ACTOR_PROTO_SRCS}
	include/ris/region/publisher.h
	include/ris/region/regionactor.h
	include/ris/region/regiontable.h
	${REGION_ACTOR_PROTO_HDRS}
)

target_include_directories(regionactor
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ris
)



add_library(regionapi SHARED
	regionapi/regionapi.h
	regionapi/regionapi.cxx
)

target_include_directories(regionapi
	PRIVATE $<TARGET_PROPERTY:regionactor,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(regionapi PRIVATE REGIONAPI_BUILDING)

target_link_libraries(regionapi PUBLIC
	regionactor
	snapshot
	zmqx
	czmq
	zmq
	config++
	glog
	gflags
)


add_executable(ribroker src/broker/ribroker.cxx)
target_link_libraries(ribroker PRIVATE zmq gflags)

add_subdirectory(zmqx)

add_subdirectory(test)
