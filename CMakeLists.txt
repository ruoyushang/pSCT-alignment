cmake_minimum_required(VERSION 2.8.4)
PROJECT("Alignment")

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake ${CMAKE_MODULE_PATH})

include_directories(sdk/cmake)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/sdk/bin)
add_subdirectory(server)
add_subdirectory(client)

#add_executable(passerver server)
