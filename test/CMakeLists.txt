CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(testcase)

# test

add_executable(test_ris
	snapshotgenerator.h
	mock_defines.h
	test_helper.h
	main.cxx
	snapshotgenerator.cxx
	test_helper.cxx
	test_snapshot.cxx
	test_regiontable.cxx
	test_pubsub.cxx
	test_ris.cxx
)

target_include_directories(test_ris
	PRIVATE $<TARGET_PROPERTY:trackercli_static,INTERFACE_INCLUDE_DIRECTORIES>
	PRIVATE $<TARGET_PROPERTY:tracker_static,INTERFACE_INCLUDE_DIRECTORIES>
	PRIVATE $<TARGET_PROPERTY:region_static,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(test_ris
	LINK_PRIVATE tracker_static trackercli_static region_static gmock gtest uuid
)
