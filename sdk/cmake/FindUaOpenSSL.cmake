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
# Try to find the OpenSSL encryption library
#
# Variables that change behaviour:
#   UA_OPENSSL_BASE_PATH  - can be set to point to a folder containing the
#                           folders inc32, out32dll and out32dll.dbg
#   UA_THIRDPARTY_DIR     - the folder containing all third-party libs
#   THIRD_PARTY_PATH      - OS specific folder (e.g. win32, win64...)
#   COMPILER_DIR          - compiler specific folder (e.g. vs2005sp1, mingw...)
#
# Read-Only variables the will be defined:
#   OPENSSL_FOUND - system has the OpenSSL library
#   OPENSSL_INCLUDE_DIR - the OpenSSL include directory
#   OPENSSL_LIBRARIES - The libraries needed to use OpenSSL
#   OPENSSL_VERSION - This is set to $major.$minor.$revision$path (eg. 0.9.8s)
###############################################################################
include(FindPackageMessage)
include(FindPackageHandleStandardArgs)

if (WIN32 AND (${CMAKE_VERSION} VERSION_LESS 2.8.3 OR ${CMAKE_VERSION} VERSION_EQUAL 2.8.10))
    # The FindOpenSSL module provided by CMake version < 2.8.3 and 2.8.10 does not
    # find our OpenSSL libraries for Visual Studio, hence the CMake version
    # requirement:
    message(WARNING "The OpenSSL libraries delivered with this SDK will NOT be "
                    "found when using this version of CMake. Update to a CMake "
                    "version of at least 2.8.3 and different to 2.8.10 to be "
                    "able to use OpenSSL.")
endif ()

if (OPENSSL_LIBRARIES OR OPENSSL_INCLUDE_DIR OR OPENSSL_SSL_LIBRARY OR OPENSSL_CRYPTO_LIBRARY)
    #message(STATUS "OPENSSL_LIBRARIES: " ${OPENSSL_LIBRARIES})
    #message(STATUS "OPENSSL_INCLUDE_DIR: " ${OPENSSL_INCLUDE_DIR})
    #message(STATUS "OPENSSL_SSL_LIBRARY: " ${OPENSSL_SSL_LIBRARY})
    #message(STATUS "OPENSSL_CRYPTO_LIBRARY: " ${OPENSSL_CRYPTO_LIBRARY})

    # on vxWorks it is not necessary to link against OpenSSL, so only OPENSSL_INCLUDE_DIR is set
    if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL vxWorks)
        if (NOT (DEFINED OPENSSL_LIBRARIES))
            message(FATAL_ERROR "OPENSSL_LIBRARIES not set!")
        endif ()
        if (NOT (DEFINED OPENSSL_INCLUDE_DIR))
            message(FATAL_ERROR "OPENSSL_INCLUDE_DIR not set!")
        endif ()
        if (NOT (DEFINED OPENSSL_SSL_LIBRARY))
            message(FATAL_ERROR "OPENSSL_SSL_LIBRARY not set!")
        endif ()
        if (NOT (DEFINED OPENSSL_CRYPTO_LIBRARY))
            message(FATAL_ERROR "OPENSSL_CRYPTO_LIBRARY not set!")
        endif ()
    endif ()
    # if this was already provided by a toolchain or user
    # we avoid searching and just use it
    set(OPENSSL_FOUND TRUE)
else ()
    if (${CMAKE_SYSTEM_NAME} MATCHES "^(Windows|WindowsCE|WindowsIoT)$")
        if (NOT UA_OPENSSL_BASE_PATH)
            include(SetThirdPartyPaths)
            set(UA_OPENSSL_BASE_PATH "${UA_THIRDPARTY_DIR}/${THIRD_PARTY_PATH}/${COMPILER_DIR}/openssl")
            if (NOT EXISTS "${UA_OPENSSL_BASE_PATH}")
                message(FATAL_ERROR "Cannot find OpenSSL library in '${UA_OPENSSL_BASE_PATH}'. "
                                    "If matching OpenSSL libraries can be provided, please adapt "
                                    "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
            endif ()
        endif ()

        if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
            set(SSL_PATH out32dll)
            set(SSL_PATH_DBG out32dll.dbg)
        elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsCE")
            if ("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "X86")
                # X86
                set(SSL_PATH out32dll_x86)
                set(SSL_PATH_DBG out32dll_x86.dbg)
            elseif ("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARMV4I")
                # ARMV4I
                set(SSL_PATH out32dll_ARM)
                set(SSL_PATH_DBG out32dll_ARM.dbg)
            else ()
                message(FATAL_ERROR "Unsupported Windows CE architecture (${MSVC_C_ARCHITECTURE_ID}). "
                                    "If matching OpenSSL libraries can be provided, please adapt "
                                    "'${CMAKE_CURRENT_LIST_FILE}' accordingly for finding and using them.")
            endif ()
        elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsIoT")
            set(SSL_PATH out32dll_${IOT_TARGET_PLATFORM})
            set(SSL_PATH_DBG out32dll_${IOT_TARGET_PLATFORM}.dbg)
        endif ()

        if (EXISTS ${UA_OPENSSL_BASE_PATH}/${SSL_PATH} AND EXISTS ${UA_OPENSSL_BASE_PATH}/${SSL_PATH_DBG})
            set(OPENSSL_ROOT_DIR ${UA_OPENSSL_BASE_PATH}/inc32 ${UA_OPENSSL_BASE_PATH}/${SSL_PATH} ${UA_OPENSSL_BASE_PATH}/${SSL_PATH_DBG})
        endif ()
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "QNX")
        # Backwards compatibility
        if (LIBRARY_ARCH)
            set(CMAKE_LIBRARY_ARCHITECTURE ${LIBRARY_ARCH})
        endif ()
        set(OPENSSL_ROOT_DIR /${CMAKE_LIBRARY_ARCHITECTURE}/usr/lib)
        # Following hack is currently needed, as OPENSSL_ROOT_DIR is not handled correctly for non-Windows systems
        # by FindOpenSSL.cmake in some CMake versions. If that gets fixed we should only use OPENSSL_ROOT_DIR instead.
        set(_OPENSSL_ROOT_HINTS_AND_PATHS PATHS /${CMAKE_LIBRARY_ARCHITECTURE}/usr/lib)
    endif ()

    # For MINGW, we set CMAKE_FIND_ROOT_PATH_MODE_... to BOTH, as OpenSSL cannot be found
    # inside CMAKE_FIND_ROOT_PATH but in OPENSSL_ROOT_DIR. This folder would not be used if
    # CMAKE_FIND_ROOT_PATH_MODE_... was set to ONLY.
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND MINGW AND CMAKE_CROSSCOMPILING)
        set(_CMAKE_FIND_ROOT_PATH_MODE_LIBRARY_BAK ${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY})
        set(_CMAKE_FIND_ROOT_PATH_MODE_INCLUDE_BAK ${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE})
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    endif ()

    find_package(OpenSSL)

    # reset MINGW exceptions
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND MINGW AND CMAKE_CROSSCOMPILING)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ${_CMAKE_FIND_ROOT_PATH_MODE_LIBRARY_BAK})
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ${_CMAKE_FIND_ROOT_PATH_MODE_INCLUDE_BAK})
    endif ()

    if (OPENSSL_FOUND)
        if (WIN32 AND NOT CYGWIN)
            if (MSVC)
                if (SSL_EAY_LIBRARY AND LIB_EAY_LIBRARY)
                    set(OPENSSL_SSL_LIBRARY ${SSL_EAY_LIBRARY})
                    set(OPENSSL_CRYPTO_LIBRARY ${LIB_EAY_LIBRARY})
                else ()
                    # workaround for older CMake versions where SSL_EAY_LIBRARY and LIB_EAY_LIBRARY are not set
                    if (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
                        set(OPENSSL_SSL_LIBRARY optimized ${SSL_EAY_RELEASE} debug ${SSL_EAY_DEBUG})
                        set(OPENSSL_CRYPTO_LIBRARY optimized ${LIB_EAY_RELEASE} debug ${LIB_EAY_DEBUG})
                    else()
                        set(OPENSSL_SSL_LIBRARY ${SSL_EAY_RELEASE})
                        set(OPENSSL_CRYPTO_LIBRARY ${LIB_EAY_RELEASE})
                    endif()
                endif ()
            else ()
                set(OPENSSL_SSL_LIBRARY ${SSL_EAY})
                set(OPENSSL_CRYPTO_LIBRARY ${LIB_EAY})
            endif ()
        else ()
            if (NOT OPENSSL_SSL_LIBRARY AND OPENSSL_SSL_LIBRARIES)
                set(OPENSSL_SSL_LIBRARY ${OPENSSL_SSL_LIBRARIES})
            endif ()
            if (NOT OPENSSL_CRYPTO_LIBRARY AND OPENSSL_CRYPTO_LIBRARIES)
                set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARIES})
            endif ()
        endif ()

        set(OPENSSL_SSL_LIBRARY ${OPENSSL_SSL_LIBRARY} CACHE STRING "")
        set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARY} CACHE STRING "")
        set(OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES} CACHE STRING "")

        if (WIN32 AND OPENSSL_LIBRARIES MATCHES "OpenSSL")
            message(WARNING "FindOpenSSL seems to have found another installation of OpenSSL instead "
                            "of the libraries provided by Unified Automation. If you want to use the "
                            "Unified Automation libraries instead, please temporarily rename the folder"
                            "containing the installed OpenSSL and run CMake again.")
        endif ()
    endif ()

    if (OPENSSL_VERSION)
        find_package_handle_standard_args(OpenSSL
                                          REQUIRED_VARS OPENSSL_LIBRARIES OPENSSL_INCLUDE_DIR
                                          VERSION_VAR OPENSSL_VERSION
                                          FAIL_MESSAGE "Could NOT find OpenSSL, try to set the path to OpenSSL root folder in the system variable OPENSSL_ROOT_DIR")
    else ()
        find_package_handle_standard_args(OpenSSL
                                          "Could NOT find OpenSSL, try to set the path to OpenSSL root folder in the system variable OPENSSL_ROOT_DIR"
                                          OPENSSL_LIBRARIES
                                          OPENSSL_INCLUDE_DIR)
    endif ()

    mark_as_advanced(OPENSSL_INCLUDE_DIR OPENSSL_LIBRARIES)

    if (NOT OPENSSL_FOUND)
        # If any of these vars is set but OPENSSL_FOUND is FALSE, we would enter
        # the if () at the top of this file and OPENSSL_FOUND would be set to TRUE,
        # so we have to unset all of them.
        unset(OPENSSL_INCLUDE_DIR)
        unset(OPENSSL_LIBRARIES)
        unset(OPENSSL_SSL_LIBRARY)
        unset(OPENSSL_CRYPTO_LIBRARY)
        unset(OPENSSL_VERSION)
        unset(OPENSSL_INCLUDE_DIR CACHE)
        unset(OPENSSL_LIBRARIES CACHE)
        unset(OPENSSL_SSL_LIBRARY CACHE)
        unset(OPENSSL_CRYPTO_LIBRARY CACHE)
        unset(OPENSSL_VERSION CACHE)
    endif ()
endif ()

macro (install_openssl_ssl_library)
    if (OPENSSL_FOUND AND WIN32 AND NOT CYGWIN)
        list(LENGTH OPENSSL_SSL_LIBRARY _CURR_LENGTH)
        math(EXPR _CURR_LENGTH "${_CURR_LENGTH} - 1")
        if (_CURR_LENGTH GREATER 0)
            foreach (_CURR_IDX RANGE 0 ${_CURR_LENGTH} 2)
                list(GET OPENSSL_SSL_LIBRARY ${_CURR_IDX} _CURR_ELEM)
                if (_CURR_ELEM STREQUAL "debug")
                    set(_CURR_CONFIGS Debug)
                elseif (_CURR_ELEM STREQUAL "optimized")
                    set(_CURR_CONFIGS Release MinSizeRel RelWithDebInfo)
                endif ()
                math(EXPR _CURR_IDX "${_CURR_IDX} + 1")
                list(GET OPENSSL_SSL_LIBRARY ${_CURR_IDX} _CURR_ELEM)
                string(REGEX REPLACE "(\\.a|\\.lib)" ".dll" _CURR_ELEM ${_CURR_ELEM})
                install(FILES ${_CURR_ELEM} DESTINATION bin CONFIGURATIONS ${_CURR_CONFIGS})
                if ("${_CURR_CONFIGS}" STREQUAL Debug)
                    string(REPLACE ".dll" ".pdb" _CURR_ELEM ${_CURR_ELEM})
                    install(FILES ${_CURR_ELEM} DESTINATION bin CONFIGURATIONS ${_CURR_CONFIGS} OPTIONAL)
                endif ()
            endforeach ()
        elseif (_CURR_LENGTH EQUAL 0)
            # one element -> no difference between debug and optimized
            list(GET OPENSSL_SSL_LIBRARY 0 _CURR_ELEM)
            string(REGEX REPLACE "(\\.a|\\.lib)" ".dll" _CURR_ELEM ${_CURR_ELEM})
            install(FILES ${_CURR_ELEM} DESTINATION bin)
        endif ()
    endif ()
endmacro ()

macro (install_openssl_crypto_library)
    if (OPENSSL_FOUND AND WIN32 AND NOT CYGWIN)
        list(LENGTH OPENSSL_CRYPTO_LIBRARY _CURR_LENGTH)
        math(EXPR _CURR_LENGTH "${_CURR_LENGTH} - 1")
        if (_CURR_LENGTH GREATER 0)
            foreach (_CURR_IDX RANGE 0 ${_CURR_LENGTH} 2)
                list(GET OPENSSL_CRYPTO_LIBRARY ${_CURR_IDX} _CURR_ELEM)
                if (_CURR_ELEM STREQUAL "debug")
                    set(_CURR_CONFIGS Debug)
                elseif (_CURR_ELEM STREQUAL "optimized")
                    set(_CURR_CONFIGS Release MinSizeRel RelWithDebInfo)
                endif ()
                math(EXPR _CURR_IDX "${_CURR_IDX} + 1")
                list(GET OPENSSL_CRYPTO_LIBRARY ${_CURR_IDX} _CURR_ELEM)
                string(REGEX REPLACE "(\\.a|\\.lib)" ".dll" _CURR_ELEM ${_CURR_ELEM})
                install(FILES ${_CURR_ELEM} DESTINATION bin CONFIGURATIONS ${_CURR_CONFIGS})
                if ("${_CURR_CONFIGS}" STREQUAL Debug)
                    string(REPLACE ".dll" ".pdb" _CURR_ELEM ${_CURR_ELEM})
                    install(FILES ${_CURR_ELEM} DESTINATION bin CONFIGURATIONS ${_CURR_CONFIGS} OPTIONAL)
                endif ()
            endforeach ()
        elseif (_CURR_LENGTH EQUAL 0)
            # one element -> no difference between debug and optimized
            list(GET OPENSSL_CRYPTO_LIBRARY 0 _CURR_ELEM)
            string(REGEX REPLACE "(\\.a|\\.lib)" ".dll" _CURR_ELEM ${_CURR_ELEM})
            install(FILES ${_CURR_ELEM} DESTINATION bin)
        endif ()
    endif ()
endmacro ()

macro (install_openssl_libraries)
    install_openssl_ssl_library()
    install_openssl_crypto_library()
endmacro ()
