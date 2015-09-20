CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(ris)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(CMakeProtobuf)

set(CMAKE_C_FLAGS "-std=c99 -fPIC -Wall -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC -Wall -fvisibility=hidden ${CMAKE_CXX_FLAGS}")


# configure risproto
set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto)

set(RIS_PROTO 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/bus.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/pub.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/snapshot.proto 
	${CMAKE_CURRENT_SOURCE_DIR}/proto/regionapi.proto 
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
	src/snapshot/snapshotclientworker.cxx
	src/snapshot/snapshotfeature.cxx
	include/snapshot/snapshotable.h
	include/snapshot/snapshotclient.h
	include/snapshot/snapshotclientworker.h
	include/snapshot/snapshotservice.h
	include/snapshot/snapshotserviceworker.h
	include/snapshot/snapshotbuilder.h
	include/snapshot/snapshotfeature.h
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
	include/ris/regionlrureader.h
)

target_include_directories(ris
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ris LINK_PUBLIC 
	snapshot
)

################### Target Region

# region objects
add_library(region_obj OBJECT
	src/region/regionactor.cxx
	src/region/regiontable.cxx
	src/region/publisher.cxx
	src/region/regionctx.cxx
	src/region/busprocesser.cxx
	include/region/publisher.h
	include/region/regionactor.h
	include/region/regiontable.h
	include/region/regionctx.h
	include/region/busprocesser.h
)

target_include_directories(region_obj
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ris
)

# static library
add_library(region_static STATIC
	src/region/regionapi.cxx
	include/region/regionapi.h
	$<TARGET_OBJECTS:region_obj>
)

target_include_directories(region_static
	PUBLIC $<TARGET_PROPERTY:region_obj,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(region_static LINK_PUBLIC
	risproto
	snapshot
	ris
	zmqx
	config++
)

# shared library
add_library(region STATIC
	src/region/regionapi.cxx
	include/region/regionapi.h
	$<TARGET_OBJECTS:region_obj>
)

target_compile_definitions(region PRIVATE REGIONAPI_BUILDING)
target_include_directories(region
	PUBLIC $<TARGET_PROPERTY:region_obj,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(region LINK_PUBLIC
	risproto
	snapshot
	ris
	zmqx
	config++
)


### Target Tracker

# tracker objects
add_library(tracker_obj OBJECT
	src/tracker/pubdata.cxx
	src/tracker/pubtracker.cxx
	src/tracker/regionfactory.cxx
	src/tracker/regionproduct.cxx
	src/tracker/subscriber.cxx
	src/tracker/regionsubcacher.cxx
	src/tracker/trackertable.cxx
	src/tracker/trackeractor.cxx
	src/tracker/trackerctx.cxx
	include/tracker/pubdata.h
	include/tracker/pubtracker.h
	include/tracker/regionfactory.h
	include/tracker/regionproduct.h
	include/tracker/subscriber.h
	include/tracker/regionsubcacher.h
	include/tracker/trackertable.h
	include/tracker/trackeractor.h
	include/tracker/trackerctx.h
)

target_include_directories(tracker_obj
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/ris
)


# tracker static library
add_library(tracker_static STATIC
	$<TARGET_OBJECTS:tracker_obj>
	src/tracker/trackerapi.cxx
	include/tracker/trackerapi.h
)

target_include_directories(tracker_static
	PUBLIC $<TARGET_PROPERTY:tracker_obj,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(tracker_static LINK_PUBLIC
	risproto
	snapshot
	ris
	zmqx
	config++
)

# tracker
add_library(tracker SHARED 
	$<TARGET_OBJECTS:tracker_obj>
	src/tracker/trackerapi.cxx
	include/tracker/trackerapi.h
)

target_include_directories(tracker
	PUBLIC $<TARGET_PROPERTY:tracker_obj,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(tracker PRIVATE TRACKERAPI_BUILDING)

target_link_libraries(tracker LINK_PUBLIC
	risproto
	snapshot
	ris
	zmqx
	config++
)


# ritracker
add_executable(ritracker
	src/tracker/ritracker.cxx
)

target_include_directories(ritracker 
	PRIVATE $<TARGET_PROPERTY:tracker_static,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(ritracker LINK_PRIVATE tracker_static)

# trackercli
add_library(trackercli SHARED
	trackercli/trackersession.h
	trackercli/trackersession.cxx
)

target_include_directories(trackercli
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/trackercli
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PRIVATE $<TARGET_PROPERTY:risproto,INTERFACE_INCLUDE_DIRECTORIES>
)


target_link_libraries(trackercli LINK_PUBLIC
	risproto
	zmqx
	zmq
	czmq
)

add_library(trackercli_static STATIC
	trackercli/trackersession.h
	trackercli/trackersession.cxx
)

target_include_directories(trackercli_static
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/trackercli
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PRIVATE $<TARGET_PROPERTY:risproto,INTERFACE_INCLUDE_DIRECTORIES>
)


target_link_libraries(trackercli_static LINK_PUBLIC
	risproto
	zmqx
	zmq
	czmq
)

## regioncli
add_library(regioncli SHARED
	regioncli/regionsession.h
	regioncli/regionsession.cxx
)

target_include_directories(regioncli
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/regioncli
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PRIVATE $<TARGET_PROPERTY:risproto,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(regioncli LINK_PUBLIC
	risproto
	zmqx
	zmq
	czmq
)

# regioncli static
add_library(regioncli_static STATIC
	regioncli/regionsession.h
	regioncli/regionsession.cxx
)

target_include_directories(regioncli_static
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/regioncli
	PUBLIC $<TARGET_PROPERTY:zmqx,INTERFACE_INCLUDE_DIRECTORIES>
	PRIVATE $<TARGET_PROPERTY:risproto,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(regioncli_static LINK_PUBLIC
	risproto
	zmqx
	zmq
	czmq
)

# broker
add_executable(ribroker 
	src/broker/ribroker.cxx
	src/broker/ribroker_main.cxx
)
target_link_libraries(ribroker PRIVATE zmq gflags glog)

add_library(ribrokeractor STATIC
	src/broker/ribroker.cxx
	src/broker/ribrokeractor.cxx
	include/broker/ribrokeractor.h
)

target_include_directories(ribrokeractor 
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ribrokeractor PUBLIC
	zmq
	gflags
	glog
)

# test project
include_directories(googletest/googletest/include)
include_directories(googletest/googlemock/include)
add_subdirectory(googletest/googlemock)
add_subdirectory(zmqx)
add_subdirectory(test)
