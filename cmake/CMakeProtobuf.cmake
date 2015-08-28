
function(build_pb_join VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

function(build_pb_c_sources HEADER_DIR SOURCE_DIR SRCS HDRS)
	if(NOT ARGN)
		message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
		return()
	endif()

	set(OUTPUT_HEADER_DIR ${HEADER_DIR})
	set(OUTPUT_SOURCE_DIR ${SOURCE_DIR})
	file(MAKE_DIRECTORY ${OUTPUT_HEADER_DIR})
	file(MAKE_DIRECTORY ${OUTPUT_SOURCE_DIR})

	if(PROTOBUF_GENERATE_CPP_APPEND_PATH)
		# Create an include path for each file specified
		foreach(FIL ${ARGN})
			get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
			get_filename_component(ABS_PATH ${ABS_FIL} PATH)
			list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
			if(${_contains_already} EQUAL -1)
				list(APPEND _protobuf_include_path -I ${ABS_PATH})
			endif()
		endforeach()
	else()
		set(_protobuf_include_path -I ${OUTPUT_HEADER_DIR})
	endif()

	if(DEFINED PROTOBUF_IMPORT_DIRS)
		foreach(DIR ${PROTOBUF_IMPORT_DIRS})
			get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
			list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
			if(${_contains_already} EQUAL -1)
				list(APPEND _protobuf_include_path -I ${ABS_PATH})
			endif()
		endforeach()
	endif()

	set(${SRCS})
	set(${HDRS})
	foreach(FIL ${ARGN})
		get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
		get_filename_component(FIL_WE ${FIL} NAME_WE)

		list(APPEND ${SRCS} "${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb-c.c")
		list(APPEND ${HDRS} "${OUTPUT_HEADER_DIR}/${FIL_WE}.pb-c.h")

		add_custom_command(
			OUTPUT "/tmp/${FIL_WE}.pb-c.c"
			"/tmp/${FIL_WE}.pb-c.h"
			COMMAND  protoc-c
			ARGS --c_out /tmp ${_protobuf_include_path} ${ABS_FIL}
			DEPENDS ${ABS_FIL}
			COMMENT "Running C protocol buffer compiler on ${FIL}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			VERBATIM )
		add_custom_command(
			OUTPUT "${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb-c.c"
			COMMAND ${CMAKE_COMMAND} -E copy /tmp/${FIL_WE}.pb-c.c ${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb-c.c
			DEPENDS "/tmp/${FIL_WE}.pb-c.c"
			)

		add_custom_command(
			OUTPUT "${OUTPUT_HEADER_DIR}/${FIL_WE}.pb-c.h"
			COMMAND ${CMAKE_COMMAND} -E copy /tmp/${FIL_WE}.pb-c.h ${OUTPUT_HEADER_DIR}/${FIL_WE}.pb-c.h
			DEPENDS "/tmp/${FIL_WE}.pb-c.h"
			)
	endforeach()

	set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
	set(${SRCS} ${${SRCS}} PARENT_SCOPE)
	set(${HDRS} ${${HDRS}} PARENT_SCOPE)

	build_pb_join("${${SRCS}}" " " STRSRC)
	build_pb_join("${${HDRS}}" " " STRHDR)

	#add_custom_target(clean-proto 
	#	COMMAND ${CMAKE_COMMAND} -E remove -f ${SRCS} ${HDRS})
endfunction()

function(build_pb_cm_sources HEADER_DIR SOURCE_DIR SRCS HDRS)
	if(NOT ARGN)
		message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
		return()
	endif()

	set(OUTPUT_HEADER_DIR ${HEADER_DIR})
	set(OUTPUT_SOURCE_DIR ${SOURCE_DIR})
	file(MAKE_DIRECTORY ${OUTPUT_HEADER_DIR})
	file(MAKE_DIRECTORY ${OUTPUT_SOURCE_DIR})

	if(PROTOBUF_GENERATE_CPP_APPEND_PATH)
		# Create an include path for each file specified
		foreach(FIL ${ARGN})
			get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
			get_filename_component(ABS_PATH ${ABS_FIL} PATH)
			list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
			if(${_contains_already} EQUAL -1)
				list(APPEND _protobuf_include_path -I ${ABS_PATH})
			endif()
		endforeach()
	else()
		set(_protobuf_include_path -I ${OUTPUT_HEADER_DIR})
	endif()

	if(DEFINED PROTOBUF_IMPORT_DIRS)
		foreach(DIR ${PROTOBUF_IMPORT_DIRS})
			get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
			list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
			if(${_contains_already} EQUAL -1)
				list(APPEND _protobuf_include_path -I ${ABS_PATH})
			endif()
		endforeach()
	endif()

	set(${SRCS})
	set(${HDRS})
	foreach(FIL ${ARGN})
		get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
		get_filename_component(FIL_WE ${FIL} NAME_WE)

		list(APPEND ${SRCS} "${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb-c.c")
		list(APPEND ${HDRS} "${OUTPUT_HEADER_DIR}/${FIL_WE}.pb-c.h")

		add_custom_command(
			OUTPUT "/tmp/${FIL_WE}.pb-c.c"
			"/tmp/${FIL_WE}.pb-c.h"
			COMMAND  protoc-cm
			ARGS --c_out /tmp ${_protobuf_include_path} ${ABS_FIL}
			DEPENDS ${ABS_FIL}
			COMMENT "Running C protocol buffer compiler(without global vars) on ${FIL}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			VERBATIM )
		add_custom_command(
			OUTPUT "${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb-c.c"
			COMMAND ${CMAKE_COMMAND} -E copy /tmp/${FIL_WE}.pb-c.c ${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb-c.c
			DEPENDS "/tmp/${FIL_WE}.pb-c.c"
			)

		add_custom_command(
			OUTPUT "${OUTPUT_HEADER_DIR}/${FIL_WE}.pb-c.h"
			COMMAND ${CMAKE_COMMAND} -E copy /tmp/${FIL_WE}.pb-c.h ${OUTPUT_HEADER_DIR}/${FIL_WE}.pb-c.h
			DEPENDS "/tmp/${FIL_WE}.pb-c.h"
			)
	endforeach()

	set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
	set(${SRCS} ${${SRCS}} PARENT_SCOPE)
	set(${HDRS} ${${HDRS}} PARENT_SCOPE)

	build_pb_join("${${SRCS}}" " " STRSRC)
	build_pb_join("${${HDRS}}" " " STRHDR)

	#add_custom_target(clean-proto 
	#	COMMAND ${CMAKE_COMMAND} -E remove -f ${SRCS} ${HDRS})
endfunction()


function(build_pb_cxx_sources HEADER_DIR SOURCE_DIR SRCS HDRS)
	if(NOT ARGN)
		message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
		return()
	endif()

	set(OUTPUT_HEADER_DIR ${HEADER_DIR})
	set(OUTPUT_SOURCE_DIR ${SOURCE_DIR})
	file(MAKE_DIRECTORY ${OUTPUT_HEADER_DIR})
	file(MAKE_DIRECTORY ${OUTPUT_SOURCE_DIR})

	if(PROTOBUF_GENERATE_CPP_APPEND_PATH)
		# Create an include path for each file specified
		foreach(FIL ${ARGN})
			get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
			get_filename_component(ABS_PATH ${ABS_FIL} PATH)
			list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
			if(${_contains_already} EQUAL -1)
				list(APPEND _protobuf_include_path -I ${ABS_PATH})
			endif()
		endforeach()
	else()
		set(_protobuf_include_path -I ${OUTPUT_HEADER_DIR})
	endif()

	if(DEFINED PROTOBUF_IMPORT_DIRS)
		foreach(DIR ${PROTOBUF_IMPORT_DIRS})
			get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
			list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
			if(${_contains_already} EQUAL -1)
				list(APPEND _protobuf_include_path -I ${ABS_PATH})
			endif()
		endforeach()
	endif()

	set(${SRCS})
	set(${HDRS})
	foreach(FIL ${ARGN})
		get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
		get_filename_component(FIL_WE ${FIL} NAME_WE)

		list(APPEND ${SRCS} "${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb.cc")
		list(APPEND ${HDRS} "${OUTPUT_HEADER_DIR}/${FIL_WE}.pb.h")

		add_custom_command(
			OUTPUT "/tmp/${FIL_WE}.pb.cc"
			"/tmp/${FIL_WE}.pb.h"
			COMMAND  protoc
			ARGS --cpp_out /tmp ${_protobuf_include_path} ${ABS_FIL}
			DEPENDS ${ABS_FIL}
			COMMENT "Running C protocol buffer compiler on ${FIL}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			VERBATIM )
		add_custom_command(
			OUTPUT "${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb.cc"
			COMMAND ${CMAKE_COMMAND} -E copy /tmp/${FIL_WE}.pb.cc ${OUTPUT_SOURCE_DIR}/${FIL_WE}.pb.cc
			DEPENDS "/tmp/${FIL_WE}.pb.cc"
			)

		add_custom_command(
			OUTPUT "${OUTPUT_HEADER_DIR}/${FIL_WE}.pb.h"
			COMMAND ${CMAKE_COMMAND} -E copy /tmp/${FIL_WE}.pb.h ${OUTPUT_HEADER_DIR}/${FIL_WE}.pb.h
			DEPENDS "/tmp/${FIL_WE}.pb.h"
			)
	endforeach()

	set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
	set(${SRCS} ${${SRCS}} PARENT_SCOPE)
	set(${HDRS} ${${HDRS}} PARENT_SCOPE)

	build_pb_join("${${SRCS}}" " " STRSRC)
	build_pb_join("${${HDRS}}" " " STRHDR)

	#add_custom_target(clean-proto 
	#	COMMAND ${CMAKE_COMMAND} -E remove -f ${SRCS} ${HDRS})
endfunction()
