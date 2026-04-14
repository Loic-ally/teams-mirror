cmake_minimum_required(VERSION 3.16)

project(myteams LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
	add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

set(MYTEAMS_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/myteams")
set(MYTEAMS_LIB_SO "${MYTEAMS_LIB_DIR}/libmyteams.so")
set(MYTEAMS_LIB_MACOS_SO "${MYTEAMS_LIB_DIR}/libmyteams_macos.so")
set(MYTEAMS_LIB_DYLIB "${MYTEAMS_LIB_DIR}/libmyteams.dylib")
set(MYTEAMS_APPLE_RUNTIME_LIB_NAME "libmyteams_macos.so")

set(MYTEAMS_IMPORTED_LIB "")
if(APPLE)
	if(EXISTS "${MYTEAMS_LIB_MACOS_SO}")
		set(MYTEAMS_IMPORTED_LIB "${MYTEAMS_LIB_MACOS_SO}")
	elseif(EXISTS "${MYTEAMS_LIB_DYLIB}")
		set(MYTEAMS_IMPORTED_LIB "${MYTEAMS_LIB_DYLIB}")
	elseif(EXISTS "${MYTEAMS_LIB_SO}")
		message(WARNING "Linux library libmyteams.so detected on macOS. Falling back to local logging stubs.")
	endif()
else()
	if(EXISTS "${MYTEAMS_LIB_SO}")
		set(MYTEAMS_IMPORTED_LIB "${MYTEAMS_LIB_SO}")
	endif()
endif()

if(MYTEAMS_IMPORTED_LIB)
	add_library(myteams SHARED IMPORTED GLOBAL)
	set_target_properties(myteams PROPERTIES
		IMPORTED_LOCATION "${MYTEAMS_IMPORTED_LIB}"
		INTERFACE_INCLUDE_DIRECTORIES "${MYTEAMS_LIB_DIR}"
	)
else()
	if(NOT APPLE)
		message(FATAL_ERROR "Missing shared library: ${MYTEAMS_LIB_SO}")
	endif()
	add_library(myteams INTERFACE)
	target_include_directories(myteams INTERFACE "${MYTEAMS_LIB_DIR}")
endif()

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
if(APPLE AND MYTEAMS_IMPORTED_LIB)
	add_custom_command(TARGET myteams_server POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${MYTEAMS_IMPORTED_LIB}"
			"${MYTEAMS_LIB_DIR}/${MYTEAMS_APPLE_RUNTIME_LIB_NAME}"
	)
endif()

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
if(APPLE AND MYTEAMS_IMPORTED_LIB)
	add_custom_command(TARGET myteams_cli POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${MYTEAMS_IMPORTED_LIB}"
			"${MYTEAMS_LIB_DIR}/${MYTEAMS_APPLE_RUNTIME_LIB_NAME}"
	)
endif()
