cmake_minimum_required(VERSION 2.8)
PROJECT("Alignment")

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake ${CMAKE_MODULE_PATH})

include_directories(sdk/cmake)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/sdk/bin)

option(BUILD_SERVER "Enable building server" ON)
option(BUILD_CLIENT "Enable building client" ON)
option(BUILD_EXAMPLES "Enable building examples" OFF)

if (BUILD_SERVER)
    add_subdirectory(server)
endif ()
if (BUILD_CLIENT)
    add_subdirectory(client)
endif ()

if (BUILD_EXAMPLES)
    add_subdirectory(sdk/examples/client_cpp_sdk)
    add_subdirectory(sdk/examples/client_gettingstarted)

    add_subdirectory(sdk/examples/server_sdk_tutorial)
    add_subdirectory(sdk/examples/server_gettingstarted)
    add_subdirectory(sdk/examples/server_cpp_demo)
endif()
    #
#add_subdirectory(server)
#add_subdirectory(client)

#add_executable(passerver server)
