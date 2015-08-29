CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(ris)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(CMakeProtobuf)


set(CMAKE_C_FLAGS "-std=c99 -fPIC -g -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC -g -fvisibility=hidden ${CMAKE_CXX_FLAGS}")


# configure risproto
set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto)

set(RIS_PROTO 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/regionapi.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/regionpub.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/snapshot.proto 
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
	include/ris/snapshot/snapshotable.h
	include/ris/snapshot/snapshotservice.h
	include/ris/snapshot/snapshotserviceworker.h
	include/ris/snapshot/snapshotbuilder.h
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
)


# region

add_library(region STATIC
	src/ritypes.cxx
	src/region/regionactor.cxx
	src/region/regiontable.cxx
	src/region/publisher.cxx
	src/region/regionctx.cxx
	include/ris/region/publisher.h
	include/ris/region/regionactor.h
	include/ris/region/regiontable.h
	include/ris/region/regionctx.h
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
	zmqx
	czmq
	zmq
	config++
	glog
	gflags
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

# tracker
add_library(tracker STATIC
	src/tracker/subscriber.cxx
	include/ris/tracker/subscriber.h
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
	zmqx
	czmq
	zmq
	config++
	glog
	gflags
)



# broker
add_executable(ribroker src/broker/ribroker.cxx)
target_link_libraries(ribroker PRIVATE zmq gflags)

add_subdirectory(zmqx)

add_subdirectory(test)
