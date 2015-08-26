CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(testcase)

add_executable(test_region
	test_region.cxx
)

target_include_directories(test_region
	PRIVATE $<TARGET_PROPERTY:region,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(test_region PRIVATE CONFI_FILE="${CMAKE_CURRENT_SOURCE_DIR}/region.conf")

target_link_libraries(test_region
	LINK_PRIVATE region
)
