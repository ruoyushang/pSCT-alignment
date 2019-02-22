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
# Try to find the LibXml2 xml processing library
# Once done this will define
#
#  LIBXML2_FOUND - System has LibXml2
#  LIBXML2_INCLUDE_DIR - The LibXml2 include directory
#  LIBXML2_LIBRARIES - The libraries needed to use LibXml2
#
# On Windows, UA_THIRDPARTY_DIR, THIRD_PARTY_PATH and COMPILER_DIR have
# to be set to the correct values or LIBXML2_BASE_PATH has to be set to
# a folder containing the folders include, out32dll and out32dll.dbg.
###############################################################################
include(FindPackageMessage)
include(FindPackageHandleStandardArgs)

if (LIBXML2_LIBRARIES AND LIBXML2_INCLUDE_DIR)
    #message(STATUS "LIBXML2_LIBRARIES: " ${LIBXML2_LIBRARIES})
    #message(STATUS "LIBXML2_INCLUDE_DIR: " ${LIBXML2_INCLUDE_DIR})
    if (NOT (LIBXML2_LIBRARIES))
        message(FATAL_ERROR "LIBXML2_LIBRARIES not set!")
    endif ()
    if (NOT (LIBXML2_INCLUDE_DIR))
        message(FATAL_ERROR "LIBXML2_INCLUDE_DIR not set!")
    endif ()
    # if this was already provided by a toolchain or user
    # we avoid searching and just use it
    set(LIBXML2_FOUND TRUE)
else ()
    if (${CMAKE_SYSTEM_NAME} MATCHES "^(Windows|WindowsCE|WindowsIoT)$")
        if (NOT LIBXML2_BASE_PATH)
            include(SetThirdPartyPaths)
            set(LIBXML2_BASE_PATH "${UA_THIRDPARTY_DIR}/${THIRD_PARTY_PATH}/${COMPILER_DIR}/libxml2")
            if (NOT EXISTS "${LIBXML2_BASE_PATH}")
                message(FATAL_ERROR "Cannot find libxml2 library in '${LIBXML2_BASE_PATH}'. "
                                    "If matching libxml2 libraries can be provided, please adapt "
                                    "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
            endif ()
        endif ()

        if (EXISTS ${LIBXML2_BASE_PATH})
            set(LIBXML2_INCLUDE_DIR ${LIBXML2_BASE_PATH}/include)

            if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
                set(LIBXML2_DEBUG_POSTFIX "d")
                set(XML_PATH out32dll)
                set(XML_PATH_DBG out32dll.dbg)
            elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsCE")
                if ("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "X86")
                    # X86
                    set(XML_PATH out32_x86)
                    set(XML_PATH_DBG out32_x86)
                elseif ("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARMV4I")
                    # ARMV4I
                    set(XML_PATH out32_ARMV4I)
                    set(XML_PATH_DBG out32_ARMV4I)
                else ()
                    message(FATAL_ERROR "Unsupported Windows CE architecture (${MSVC_C_ARCHITECTURE_ID}). "
                                        "If matching libxml2 libraries can be provided, please adapt "
                                        "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
                endif ()
            elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsIoT")
                set(LIBXML2_DEBUG_POSTFIX "d")
                set(XML_PATH out32dll_${IOT_TARGET_PLATFORM})
                set(XML_PATH_DBG out32dll_${IOT_TARGET_PLATFORM}.dbg)
            endif ()

            if (EXISTS ${LIBXML2_BASE_PATH}/${XML_PATH} AND EXISTS ${LIBXML2_BASE_PATH}/${XML_PATH_DBG})
                set(LIBXML2_LIBRARIES
                    optimized ${LIBXML2_BASE_PATH}/${XML_PATH}/libxml2.lib
                    debug ${LIBXML2_BASE_PATH}/${XML_PATH_DBG}/libxml2${LIBXML2_DEBUG_POSTFIX}.lib
                )
            endif ()
        endif ()

        if (LIBXML2_INCLUDE_DIR AND EXISTS "${LIBXML2_INCLUDE_DIR}/libxml/xmlversion.h")
            file(STRINGS "${LIBXML2_INCLUDE_DIR}/libxml/xmlversion.h" libxml2_version_str
                 REGEX "^#define[\t ]+LIBXML_DOTTED_VERSION[\t ]+\".*\"")
            string(REGEX REPLACE "^#define[\t ]+LIBXML_DOTTED_VERSION[\t ]+\"([^\"]*)\".*" "\\1"
                   LIBXML2_VERSION_STRING "${libxml2_version_str}")
            unset(libxml2_version_str)
        endif ()

        find_package_handle_standard_args(LibXml2
                                          REQUIRED_VARS LIBXML2_LIBRARIES LIBXML2_INCLUDE_DIR
                                          VERSION_VAR LIBXML2_VERSION_STRING)
        mark_as_advanced(LIBXML2_INCLUDE_DIR LIBXML2_LIBRARIES LIBXML2_XMLLINT_EXECUTABLE)
    else ()
        # use system LibXml2
        find_package(LibXml2)
    endif ()
endif ()

macro (install_libxml2_library)
    if (LIBXML2_FOUND AND WIN32 AND NOT CYGWIN)
        list(LENGTH LIBXML2_LIBRARIES _CURR_LENGTH)
        math(EXPR _CURR_LENGTH "${_CURR_LENGTH} - 1")
        foreach (_CURR_IDX RANGE 0 ${_CURR_LENGTH} 2)
            list(GET LIBXML2_LIBRARIES ${_CURR_IDX} _CURR_ELEM)
            if (_CURR_ELEM STREQUAL "debug")
                set(_CURR_CONFIGS Debug)
            elseif (_CURR_ELEM STREQUAL "optimized")
                set(_CURR_CONFIGS Release MinSizeRel RelWithDebInfo)
            endif ()
            math(EXPR _CURR_IDX "${_CURR_IDX} + 1")
            list(GET LIBXML2_LIBRARIES ${_CURR_IDX} _CURR_ELEM)
            string(REGEX REPLACE "(\\.a|\\.lib)" ".dll" _CURR_ELEM ${_CURR_ELEM})
            install(FILES ${_CURR_ELEM} DESTINATION bin CONFIGURATIONS ${_CURR_CONFIGS})
            if ("${_CURR_CONFIGS}" STREQUAL Debug)
                string(REPLACE ".dll" ".pdb" _CURR_ELEM ${_CURR_ELEM})
                install(FILES ${_CURR_ELEM} DESTINATION bin CONFIGURATIONS ${_CURR_CONFIGS} OPTIONAL)
            endif ()
        endforeach ()
    endif ()
endmacro ()
