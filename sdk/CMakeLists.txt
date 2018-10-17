project(uasdk)
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/alignment/lib64)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CMakeDependentOption)
include(MessageUtils)
display_project_header("true")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
    add_subdirectory(src)
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif ()

# set CMAKE_INSTALL_MESSAGE to LAZY by default to hide 'Up to date' output when building INSTALL target
if (NOT ${CMAKE_VERSION} VERSION_LESS "3.1")
    set(CMAKE_INSTALL_MESSAGE LAZY CACHE STRING "")
    set(CMAKE_INSTALL_MESSAGE_VALUES "ALWAYS;LAZY;NEVER")
    set_property(CACHE CMAKE_INSTALL_MESSAGE PROPERTY STRINGS ${CMAKE_INSTALL_MESSAGE_VALUES})
endif ()
