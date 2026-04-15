cmake_minimum_required(VERSION 3.16)

project(myteams LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
	add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_SKIP_RPATH TRUE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

set(MYTEAMS_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/myteams")

set(MYTEAMS_IMPORTED_LIB "$ENV{LD_LIBRARY_PATH}")

if(NOT MYTEAMS_IMPORTED_LIB)
	message(FATAL_ERROR "Missing shared library path in LD_LIBRARY_PATH")
endif()

if(MYTEAMS_IMPORTED_LIB MATCHES ":")
	message(FATAL_ERROR "LD_LIBRARY_PATH must contain a single library file path for this project")
endif()

if(NOT IS_ABSOLUTE "${MYTEAMS_IMPORTED_LIB}")
	get_filename_component(MYTEAMS_IMPORTED_LIB "${MYTEAMS_IMPORTED_LIB}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
endif()

if(NOT EXISTS "${MYTEAMS_IMPORTED_LIB}")
	message(FATAL_ERROR "Shared library not found at path: ${MYTEAMS_IMPORTED_LIB}")
endif()

get_filename_component(MYTEAMS_IMPORTED_LIB_DIR "${MYTEAMS_IMPORTED_LIB}" DIRECTORY)

add_library(myteams SHARED IMPORTED GLOBAL)
set_target_properties(myteams PROPERTIES
	IMPORTED_LOCATION "${MYTEAMS_IMPORTED_LIB}"
	INTERFACE_INCLUDE_DIRECTORIES "${MYTEAMS_LIB_DIR}"
)

add_library(myteams::myteams ALIAS myteams)

file(GLOB_RECURSE COMMON_UTILS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/common/utils/*.cpp"
)

file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/server/*.cpp"
)

add_executable(myteams_server ${SERVER_SOURCES} ${COMMON_UTILS_SOURCES})
target_include_directories(myteams_server PRIVATE
	"${CMAKE_SOURCE_DIR}"
	"${CMAKE_SOURCE_DIR}/server"
	"${CMAKE_SOURCE_DIR}/server/core"
	"${CMAKE_SOURCE_DIR}/common"
	"${CMAKE_SOURCE_DIR}/common/utils"
	"${MYTEAMS_LIB_DIR}"
)
target_link_libraries(myteams_server PRIVATE myteams::myteams)

file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/client/*.cpp"
)

add_executable(myteams_cli ${CLIENT_SOURCES} ${COMMON_UTILS_SOURCES})
target_include_directories(myteams_cli PRIVATE
	"${CMAKE_SOURCE_DIR}"
	"${CMAKE_SOURCE_DIR}/client"
	"${CMAKE_SOURCE_DIR}/common"
	"${CMAKE_SOURCE_DIR}/common/utils"
	"${MYTEAMS_LIB_DIR}"
)
target_link_libraries(myteams_cli PRIVATE myteams::myteams)
if(APPLE)
	add_custom_target(myteams_runtime_symlink ALL
		COMMAND ${CMAKE_COMMAND} -E remove -f
			"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libmyteams_macos.so"
		COMMAND ${CMAKE_COMMAND} -E create_symlink
			"${MYTEAMS_IMPORTED_LIB}"
			"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libmyteams_macos.so"
	)
	add_dependencies(myteams_server myteams_runtime_symlink)
	add_dependencies(myteams_cli myteams_runtime_symlink)
endif()
