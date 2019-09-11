cmake_minimum_required(VERSION 2.8)
PROJECT("Alignment")

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake ${CMAKE_MODULE_PATH})

include_directories(sdk/cmake)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/sdk/bin)

option(BUILD_SERVER "Enable building server" OFF)
option(BUILD_CLIENT "Enable building client" OFF)

if (BUILD_SERVER)
    add_subdirectory(server)
endif ()
if (BUILD_CLIENT)
    add_subdirectory(client)
endif ()


#add_executable(passerver server)
