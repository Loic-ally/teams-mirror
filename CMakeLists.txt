cmake_minimum_required(VERSION 3.16)

project(myteams LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
	add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

set(MYTEAMS_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/myteams")
set(MYTEAMS_LIB "${MYTEAMS_LIB_DIR}/libmyteams.so")

if(NOT EXISTS "${MYTEAMS_LIB}")
	message(FATAL_ERROR "Missing shared library: ${MYTEAMS_LIB}")
endif()

add_library(myteams SHARED IMPORTED GLOBAL)
add_library(myteams::myteams ALIAS myteams)
set_target_properties(myteams PROPERTIES
	IMPORTED_LOCATION "${MYTEAMS_LIB}"
	INTERFACE_INCLUDE_DIRECTORIES "${MYTEAMS_LIB_DIR}"
)

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
set_target_properties(myteams_server PROPERTIES
	BUILD_RPATH "$ORIGIN/libs/myteams"
	INSTALL_RPATH "$ORIGIN/libs/myteams"
)

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
set_target_properties(myteams_cli PROPERTIES
	BUILD_RPATH "$ORIGIN/libs/myteams"
	INSTALL_RPATH "$ORIGIN/libs/myteams"
)
