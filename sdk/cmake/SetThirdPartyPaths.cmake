###############################################################################
#
# Copyright (C) 2005-2014 Unified Automation GmbH. All Rights Reserved.
# Web: http://www.unified-automation.com
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# This software is licensed under the proprietary Software License Agreement
# (SLA) of Unified Automation GmbH. You are not allowed to publish, distribute,
# sublicense, and/or sell the Source Code of this Software.
# Get full License Agreement at: license@www.unified-automation.com
#
# Try to set the third party paths
#
# If any of these variables is set, it will not be set by this module:
#   UA_THIRDPARTY_DIR     - the folder containing all third-party libs
#   THIRD_PARTY_PATH      - OS specific folder (e.g. win32, win64...)
#   COMPILER_DIR          - compiler specific folder (e.g. vs2005sp1, mingw...)
#
# Variables that will be defined:
#   UA_THIRDPARTY_DIR     - the folder containing all third-party libs
#   THIRD_PARTY_PATH      - OS specific folder (e.g. win32, win64...)
#   COMPILER_DIR          - compiler specific folder (e.g. vs2005sp1, mingw...)
###############################################################################

if (${CMAKE_SYSTEM_NAME} MATCHES "^(Windows|WindowsCE|WindowsIoT)$")
    # we assume that this file is inside /cmake or /uasdk_cmake and the
    # third party folder is in /third-party
    if (NOT UA_THIRDPARTY_DIR)
        if (NOT CMAKE_CURRENT_LIST_DIR)
            get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
        endif ()

        if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/../third-party)
            set(UA_THIRDPARTY_DIR "${CMAKE_CURRENT_LIST_DIR}/../third-party")
        else ()
            message(FATAL_ERROR "Auto detection of UA_THIRDPARTY_DIR failed, please set it manually!")
        endif ()
        get_filename_component(UA_THIRDPARTY_DIR ${UA_THIRDPARTY_DIR} ABSOLUTE)
    endif ()

    # detect win32/win64/wince
    if (NOT THIRD_PARTY_PATH)
        if (WINCE)
            if (CMAKE_SYSTEM_VERSION MATCHES "6.*")
                set(THIRD_PARTY_PATH "wince6")
            elseif (CMAKE_SYSTEM_VERSION MATCHES "7.*")
                set(THIRD_PARTY_PATH "wince7")
            else ()
                message(FATAL_ERROR "Selected Windows CE Version (${CMAKE_SYSTEM_VERSION}) is not supported. "
                                    "If matching third-party libraries can be provided, please adapt "
                                    "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
            endif ()
        elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsIoT")
            set(THIRD_PARTY_PATH "winiot")
        elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(THIRD_PARTY_PATH "win64")
        else ()
            set(THIRD_PARTY_PATH "win32")
        endif ()
    endif ()

    # detect compiler
    if (NOT COMPILER_DIR)
        if (DEFINED MSVC_VERSION)
            if (MSVC60 OR ${MSVC_VERSION} EQUAL 1200)
                set(COMPILER_DIR vs6sp6)
            elseif (MSVC80 OR ${MSVC_VERSION} EQUAL 1400)
                set(COMPILER_DIR vs2005sp1)
            elseif (MSVC90 OR ${MSVC_VERSION} EQUAL 1500)
                set(COMPILER_DIR vs2008sp1)
            elseif (MSVC10 OR ${MSVC_VERSION} EQUAL 1600)
                set(COMPILER_DIR vs2010sp1)
            elseif (MSVC11 OR ${MSVC_VERSION} EQUAL 1700)
                set(COMPILER_DIR vs2012sp4)
            elseif (MSVC12 OR ${MSVC_VERSION} EQUAL 1800)
                set(COMPILER_DIR vs2013sp3)
            elseif (MSVC14 OR ${MSVC_VERSION} EQUAL 1900)
                set(COMPILER_DIR vs2015)
            else ()
                message(FATAL_ERROR "The used Visual Studio version was not recognized. "
                                    "If matching third-party libraries can be provided, please adapt "
                                    "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
            endif ()
        elseif (MINGW)
            set(COMPILER_DIR mingw)
        else ()
            message(FATAL_ERROR "The used compiler was not recognized. "
                                "If matching third-party libraries can be provided, please adapt "
                                "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
        endif ()
    endif ()
endif ()
