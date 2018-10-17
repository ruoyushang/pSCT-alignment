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
# Configures the uastack submodule.
###############################################################################

# uastack platform
if (${CMAKE_SYSTEM_NAME} MATCHES "^(Windows|WindowsCE|WindowsIoT)$")
    set(UASTACK_PLATFORM "win32_v6")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(UASTACK_PLATFORM "linux")
elseif (vxWorks)
    set(UASTACK_PLATFORM "vxworks")
elseif (QNXNTO)
    set(UASTACK_PLATFORM "qnx")
elseif (EUROS)
    set(UASTACK_PLATFORM "euros")
else ()
    message(FATAL_ERROR "Unknown stack platform!")
endif ()
set(UASTACK_PLATFORM ${UASTACK_PLATFORM} CACHE STRING "Ua Stack platform chosen by the user at CMake configure time")
unset(UASTACK_PLATFORM)
set(UASTACK_PLATFORM_VALUES "win32_v6;win32;win32_cp;rtos;codesys;euros;intime;linux;qnx;vxworks;apple")
set_property(CACHE UASTACK_PLATFORM PROPERTY STRINGS ${UASTACK_PLATFORM_VALUES})
#message(STATUS "Selected stack platform ${UASTACK_PLATFORM}")

if (NOT CMAKE_CURRENT_LIST_DIR)
    get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
endif ()

if (DEFINED uastack_SOURCE_DIR)
    set(_BASE_DIR                   ${CMAKE_CURRENT_LIST_DIR}/../src)
    set(UASTACK_INCLUDE             ${_BASE_DIR}/uastack/core
                                    ${_BASE_DIR}/uastack/stackcore
                                    ${_BASE_DIR}/uastack/proxystub/serverstub
                                    ${_BASE_DIR}/uastack/proxystub/clientproxy
                                    ${_BASE_DIR}/uastack/transport/tcp
                                    ${_BASE_DIR}/uastack/transport/https
                                    ${_BASE_DIR}/uastack/platforms/${UASTACK_PLATFORM}
                                    ${uastack_BINARY_DIR})
    set(UASTACK_LIBRARY             uastack)
else ()
    # Find the SDK by probing:
    # - CMake variable UASDK_BASE_DIR
    # - environment variable UASDK_BASE_DIR
    # - path relative to folder containing this file
    # - path relative to CMAKE_INSTALL_PREFIX
    if (UASDK_BASE_DIR AND NOT IS_DIRECTORY ${UASDK_BASE_DIR}/include/uastack)
        message(FATAL_ERROR "CMake variable UASDK_BASE_DIR does not point to folder containing UaStack include headers")
    else ()
        file(TO_CMAKE_PATH "$ENV{UASDK_BASE_DIR}" _TMP_BASE_DIR)
        if (_TMP_BASE_DIR)
            if (IS_DIRECTORY ${_TMP_BASE_DIR}/include/uastack)
                set(_BASE_DIR ${_TMP_BASE_DIR})
            else ()
                message(FATAL_ERROR "Environment variable UASDK_BASE_DIR does not point to folder containing UaStack include headers")
            endif ()
        elseif (IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../include/uastack)
            set(_BASE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
        elseif (NOT IS_ABSOLUTE ${CMAKE_INSTALL_PREFIX})
            get_filename_component(TMP_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
            if (IS_DIRECTORY ${TMP_INSTALL_PREFIX}/include/uastack)
                set(_BASE_DIR ${TMP_INSTALL_PREFIX})
            endif ()
        elseif (IS_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include/uastack)
            set(_BASE_DIR ${CMAKE_INSTALL_PREFIX})
        endif ()

        if (NOT _BASE_DIR)
            message(FATAL_ERROR "Could not find folder containing UaStack include headers")
        else ()
            set(UASDK_BASE_DIR ${_BASE_DIR} CACHE PATH "Directory containing the UA SDK")
            mark_as_advanced(UASDK_BASE_DIR)
        endif ()
    endif ()

    set(UASTACK_INCLUDE             ${UASDK_BASE_DIR}/include/uastack)

    set(UA_LIB_DIR                  ${UASDK_BASE_DIR}/lib)

    set(UASTACK_LIBRARY             debug "uastackd"            optimized "uastack")

    macro (get_option_from_file _FILENAME _DEFINE_NAME _OPTION_NAME)
        file(STRINGS "${_FILENAME}" TMP_${_OPTION_NAME}
             REGEX "^#[\t ]*define[\t ]+${_DEFINE_NAME}[\t ]+!?OPCUA_CONFIG_(ON|OFF)[\t ]*")
        if (NOT TMP_${_OPTION_NAME})
            message(FATAL_ERROR "Define ${_DEFINE_NAME} missing in config file ${_FILENAME}")
        endif ()
        if (TMP_${_OPTION_NAME} MATCHES "([^!]OPCUA_CONFIG_ON|!OPCUA_CONFIG_OFF)")
            set(${_OPTION_NAME} ON CACHE INTERNAL "")
        else ()
            set(${_OPTION_NAME} OFF CACHE INTERNAL "")
        endif ()
    endmacro ()

    # Use config headers to detect the build configuration
    get_option_from_file("${UASTACK_INCLUDE}/opcua_p_config.h" OPCUA_HAVE_OPENSSL UASTACK_WITH_OPENSSL)
    get_option_from_file("${UASTACK_INCLUDE}/opcua_p_config.h" OPCUA_P_SOCKETMANAGER_SUPPORT_SSL UASTACK_WITH_TLS)
    get_option_from_file("${UASTACK_INCLUDE}/opcua_p_config.h" OPCUA_SUPPORT_PKI_WIN32 UASTACK_WITH_PKI_WIN32)

    get_option_from_file("${UASTACK_INCLUDE}/opcua_config.h" OPCUA_MULTITHREADED UASTACK_USE_THREADS)
    get_option_from_file("${UASTACK_INCLUDE}/opcua_config.h" OPCUA_USE_SYNCHRONISATION UASTACK_USE_SYNCHRONIZATION)
    get_option_from_file("${UASTACK_INCLUDE}/opcua_config.h" OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)
    get_option_from_file("${UASTACK_INCLUDE}/opcua_config.h" OPCUA_BUILD_SHARED_STACK BUILD_SHARED_STACK)
endif ()

# set SYSTEM_LIBS depending on configuration
if (NOT BUILD_SHARED_STACK)
    if (UASTACK_WITH_OPENSSL)
        set(SYSTEM_LIBS ${SYSTEM_LIBS} ${OPENSSL_CRYPTO_LIBRARY})
        if (UASTACK_WITH_TLS)
            set(SYSTEM_LIBS ${SYSTEM_LIBS} ${OPENSSL_SSL_LIBRARY})
        endif ()
    endif ()
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        list(APPEND SYSTEM_LIBS ws2_32 rpcrt4)
        if (UASTACK_WITH_PKI_WIN32)
            list(APPEND SYSTEM_LIBS crypt32)
        endif ()
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsCE")
        list(APPEND SYSTEM_LIBS ${WCELIBCEX_LIBRARIES} ws2 mmtimer)
    elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        list(APPEND SYSTEM_LIBS rt)
        if (UASTACK_USE_SYNCHRONIZATION)
            list(APPEND SYSTEM_LIBS pthread)
        endif ()
    endif ()
endif ()

# deprecated compat variables
set(UA_STACK_LIBRARY ${UASTACK_LIBRARY})

# sanity check
if (DEFINED BUILD_SHARED_STACK AND NOT BUILD_SHARED_STACK AND BUILD_SHARED_LIBS)
    message(FATAL_ERROR "It is not possible to build shared libs together with a static UaStack. "
                        "Set BUILD_SHARED_STACK to ON or BUILD_SHARED_LIBS to OFF to fix this error.")
endif ()
