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
# Try to find the WCELIBCEX library
#
# Variables that change behaviour:
#   WCELIBCEX_BASE_PATH   - can be set to point to a folder containing the
#                           folders inc32, out32_x86 and out32.dbg_x86
#   UA_THIRDPARTY_DIR     - the folder containing all third-party libs
#   THIRD_PARTY_PATH      - OS specific folder (e.g. wince6, wince7...)
#   COMPILER_DIR          - compiler specific folder (e.g. vs2008sp1...)
#
# Variables the will be defined:
#   WCELIBCEX_INCLUDE_DIR - the WCELIBCEX include directory
#   WCELIBCEX_LIBRARIES - The libraries needed to use WCELIBCEX
###############################################################################

if (CMAKE_SYSTEM_NAME STREQUAL "WindowsCE")
    if (WCELIBCEX_LIBRARIES OR WCELIBCEX_INCLUDE_DIR)
        #message(STATUS "WCELIBCEX_LIBRARIES:   " ${WCELIBCEX_LIBRARIES})
        #message(STATUS "WCELIBCEX_INCLUDE_DIR: " ${WCELIBCEX_INCLUDE_DIR})

        if (NOT (DEFINED WCELIBCEX_LIBRARIES))
            message(FATAL_ERROR "WCELIBCEX_LIBRARIES not set!")
        endif ()
        if (NOT (DEFINED WCELIBCEX_INCLUDE_DIR))
            message(FATAL_ERROR "WCELIBCEX_INCLUDE_DIR not set!")
        endif ()
    else ()
        if (NOT WCELIBCEX_BASE_PATH)
            include(SetThirdPartyPaths)
            set(WCELIBCEX_BASE_PATH ${UA_THIRDPARTY_DIR}/${THIRD_PARTY_PATH}/${COMPILER_DIR}/wcelibcex)
            if (NOT EXISTS "${WCELIBCEX_BASE_PATH}")
                message(FATAL_ERROR "Cannot find wcelibcex library in '${WCELIBCEX_BASE_PATH}'. "
                                    "If matching wcelibcex libraries can be provided, please adapt "
                                    "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
            endif ()
        endif ()

        if ("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "X86")
            set(COMPAT_PATH out32_x86)
            set(COMPAT_PATH_DBG out32.dbg_x86)
        elseif ("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARMV4I")
            set(COMPAT_PATH out32_ARMV4I)
            set(COMPAT_PATH_DBG out32.dbg_ARMV4I)
        else ()
            message(FATAL_ERROR "Unsupported Windows CE architecture (${MSVC_C_ARCHITECTURE_ID}). "
                                "If matching wcelibcex libraries can be provided, please adapt "
                                "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
        endif ()

        set(WCELIBCEX_INCLUDE_DIR ${WCELIBCEX_BASE_PATH}/include CACHE STRING "")
        set(WCELIBCEX_LIBRARIES optimized ${WCELIBCEX_BASE_PATH}/${COMPAT_PATH}/wcelibcex.lib
                                debug ${WCELIBCEX_BASE_PATH}/${COMPAT_PATH_DBG}/wcelibcex.lib
                                CACHE STRING "")

        mark_as_advanced(WCELIBCEX_INCLUDE_DIR WCELIBCEX_LIBRARIES)
    endif ()

    set(WINDOWS_CE 1)

    include_directories(${WCELIBCEX_INCLUDE_DIR})

    add_definitions(-DNO_SYS_TYPES_H -D_INC_STDIO -D_USE_32BIT_TIME_T -D_GUID_CREATE_NOT_AVAILABLE)

    # Set the LINK_FLAGS to a blank line to avoid linker errors in WinCE
    set(WCELIBCEX_LINK_FLAGS LINK_FLAGS "/ENTRY:\"\"")
    # Compat define for projects that used the old naming
    set(WCELINKERFLAGS ${WCELIBCEX_LINK_FLAGS})

    set(SYSTEM_LIBS ${SYSTEM_LIBS} ${WCELIBCEX_LIBRARIES} ws2 mmtimer)
endif ()
