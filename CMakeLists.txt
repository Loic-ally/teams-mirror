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

find_library(MYTEAMS_IMPORTED_LIB
    NAMES myteams
    PATHS "${MYTEAMS_LIB_DIR}"
    NO_DEFAULT_PATH
)

if(MYTEAMS_IMPORTED_LIB)
    message(STATUS "myteams lib found: ${MYTEAMS_IMPORTED_LIB}")
    add_library(myteams SHARED IMPORTED GLOBAL)
    set_target_properties(myteams PROPERTIES
        IMPORTED_LOCATION "${MYTEAMS_IMPORTED_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${MYTEAMS_LIB_DIR}"
    )
    add_library(myteams::myteams ALIAS myteams)
else()
    message(STATUS "myteams lib not found, building without it")
endif()

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

if(MYTEAMS_IMPORTED_LIB)
    target_link_libraries(myteams_server PRIVATE myteams::myteams)
    target_link_libraries(myteams_cli PRIVATE myteams::myteams)
else()
    target_link_options(myteams_server PRIVATE -Wl,--unresolved-symbols=ignore-all)
    target_link_options(myteams_cli PRIVATE -Wl,--unresolved-symbols=ignore-all)
endif()
