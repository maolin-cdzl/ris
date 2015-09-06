CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(testcase)

# test region
add_executable(test_region
	test_region.cxx
)

target_include_directories(test_region
	PRIVATE $<TARGET_PROPERTY:region_static,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(test_region PRIVATE CONFI_FILE="${CMAKE_CURRENT_SOURCE_DIR}/region.conf")

target_link_libraries(test_region
	LINK_PRIVATE region_static
)

# test tracker
add_executable(test_tracker
	test_tracker.cxx
)

target_include_directories(test_tracker
	PRIVATE $<TARGET_PROPERTY:tracker_static,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(test_tracker PRIVATE CONFI_FILE="${CMAKE_CURRENT_SOURCE_DIR}/tracker.conf")

target_link_libraries(test_tracker
	LINK_PRIVATE tracker_static
)


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
)

target_include_directories(test_ris
	PRIVATE $<TARGET_PROPERTY:tracker_static,INTERFACE_INCLUDE_DIRECTORIES>
	PRIVATE $<TARGET_PROPERTY:region_static,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(test_ris
	LINK_PRIVATE tracker_static region_static gmock gtest uuid
)
