# Set debugging configuration in Visual Studio projects
# (C) 2015 ascolab GmbH
#
# This file provides:
#
# vs_set_debug_config TARGET <target name>
#                     [SET_ALL_BUILD]
#                     [INSTALL_DIR_EXECUTABLE | BINARY_DIR_EXECUTABLE]
#                     [FORCE]
#                     [WORKING_DIR <working dir>]
#                     [ARGS <additional arguments>]
#                     [ENVIRONMENT <path1> [<path2>]]
#
# - TARGET: specifies an existing target
# - SET_ALL_BUILD: set this configuration for the ALL_BUILD project, too
# - INSTALL_DIR_EXECUTABLE: use the installed executable inside CMAKE_INSTALL_PREFIX
# - BINARY_DIR_EXECUTABLE: use the generated executable inside CMAKE_BINARY_DIR
# - FORCE: force creation of the .user files, even if they already exist
# - WORKING_DIR: the working directory to use when executing the target application
# - ARGS: arguments to pass to the application
# - ENVIRONMENT: additional folders to prepend to the PATH environment variable
#
# Relative paths passed to WORKING_DIR and ENVIRONMENT are considered
# being relative to CMAKE_BINARY_DIR.
#
################################################################################

if (NOT MSVC_IDE)
    function (VS_SET_DEBUG_CONFIG)
    endfunction ()
    return()
elseif (${CMAKE_VERSION} VERSION_LESS 2.8.7)
    # string(SHA1 ...) is supported starting with 2.8.7
    message(WARNING "CMake version of at least 2.8.7 is needed for setting the debug "
                    "configuration of the generated Visual Studio projects. This is an "
                    "optional feature and not necessary for development. Update CMake "
                    "to version 2.8.7 or above to use this feature.")
    function (VS_SET_DEBUG_CONFIG)
    endfunction ()
    return()
endif ()

include(CMakeParseArguments)

set(_VS_TEMPLATE_DIR ${CMAKE_CURRENT_LIST_DIR}/VSSetDebugConfigTemplates)

function (VS_SET_DEBUG_CONFIG)
    # parse arguments
    cmake_parse_arguments(_VS "SET_ALL_BUILD;INSTALL_DIR_EXECUTABLE;BINARY_DIR_EXECUTABLE;FORCE" "TARGET;WORKING_DIR;ARGS" "ENVIRONMENT" ${ARGN})

    # check for valid arguments
    if (_VS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Invalid arguments: ${_VS_UNPARSED_ARGUMENTS}")
        return()
    elseif (NOT _VS_TARGET)
        message(FATAL_ERROR "TARGET not set")
        return()
    elseif (NOT TARGET ${_VS_TARGET})
        message(FATAL_ERROR "'${_VS_TARGET}' is not a valid target")
        return()
    elseif (NOT _VS_INSTALL_DIR_EXECUTABLE AND NOT _VS_BINARY_DIR_EXECUTABLE)
        message(FATAL_ERROR "One of INSTALL_DIR_EXECUTABLE or BINARY_DIR_EXECUTABLE has to be set")
        return()
    elseif (_VS_INSTALL_DIR_EXECUTABLE AND _VS_BINARY_DIR_EXECUTABLE)
        message(FATAL_ERROR "Only one of INSTALL_DIR_EXECUTABLE or BINARY_DIR_EXECUTABLE can be set")
        return()
    endif ()

    # get architecture
    if (CMAKE_CL_64)
        # 64 bit
        set(_VS_ARCH "x64")
    else ()
        # 32 bit
        set(_VS_ARCH "Win32")
    endif ()

    # get hostname
    site_name(_VS_HOSTNAME)
    mark_as_advanced(_VS_HOSTNAME)

    # get used Visual Studio version
    if (MSVC_VERSION EQUAL 1310)
        # (Visual Studio 2003)
        set(_VS_VERSION "7,10")
    elseif (MSVC_VERSION EQUAL 1400)
        # (Visual Studio 2005)
        set(_VS_VERSION "8,00")
    elseif (MSVC_VERSION EQUAL 1500)
        # (Visual Studio 2008)
        set(_VS_VERSION "9,00")
    elseif (MSVC_VERSION EQUAL 1600)
        # (Visual Studio 2010)
        set(_VS_VERSION "10,00")
    elseif (MSVC_VERSION EQUAL 1700)
        # (Visual Studio 2012)
        set(_VS_VERSION "11,00")
    elseif (MSVC_VERSION EQUAL 1800)
        # (Visual Studio 2013)
        set(_VS_VERSION "12,00")
    elseif (MSVC_VERSION EQUAL 1900)
        # (Visual Studio 2015)
        set(_VS_VERSION "14,00")
    else ()
        message(WARNING "Unsupported version of Visual Studio detected (${MSVC_VERSION}), skip setting debug config")
        return()
    endif ()

    # use PROJECT_BINARY_DIR if there was no explicit project() command (e.g. in Unittests)
    if (${_VS_TARGET}_BINARY_DIR)
        set(_VS_USED_BINARY_DIR ${${_VS_TARGET}_BINARY_DIR})
    else ()
        set(_VS_USED_BINARY_DIR ${PROJECT_BINARY_DIR})
    endif ()

    # use templates depending on Visual Studio version
    if (MSVC_VERSION GREATER 1500)
        # VS 2010 and later
        file(READ "${_VS_TEMPLATE_DIR}/vcxproj_per_config.in" _VS_PERCONFIG_IN)
        set(_VS_INFILE "${_VS_TEMPLATE_DIR}/vcxproj_user.in")
        set(_VS_OUTFILE "${_VS_USED_BINARY_DIR}/${_VS_TARGET}.vcxproj.user")
        set(_VS_ENV_SEPARATOR "\n")
    else ()
        # VS 2008 and earlier
        file(READ "${_VS_TEMPLATE_DIR}/vcproj_per_config.in" _VS_PERCONFIG_IN)
        set(_VS_INFILE "${_VS_TEMPLATE_DIR}/vcproj_user.in")
        set(_VS_OUTFILE "${_VS_USED_BINARY_DIR}/${_VS_TARGET}.vcproj.${_VS_HOSTNAME}.$ENV{USERNAME}.user")
        set(_VS_ENV_SEPARATOR "&#x0A;")
    endif ()

    # set PATH variable from passed ENVIRONMENT
    if (_VS_ENVIRONMENT)
        set(_VS_ENVIRONMENT_STRING "PATH=")
    endif ()
    foreach (_VS_ENVIRONMENT_PART IN LISTS _VS_ENVIRONMENT)
        if (NOT IS_ABSOLUTE ${_VS_ENVIRONMENT_PART})
            get_filename_component(_VS_ENVIRONMENT_PART "${CMAKE_BINARY_DIR}/${_VS_ENVIRONMENT_PART}" ABSOLUTE)
        endif ()
        file(TO_NATIVE_PATH ${_VS_ENVIRONMENT_PART} _VS_ENVIRONMENT_PART)
        set(_VS_ENVIRONMENT_STRING "${_VS_ENVIRONMENT_STRING}${_VS_ENVIRONMENT_PART};")
    endforeach ()
    if (_VS_ENVIRONMENT_STRING)
        set(_VS_ENVIRONMENT_STRING "${_VS_ENVIRONMENT_STRING}%PATH%")
    endif ()
    # add $(LocalDebuggerEnvironment) for Visual Studio >= 2010
    if (MSVC_VERSION GREATER 1500)
        set(_VS_ENVIRONMENT_STRING "${_VS_ENVIRONMENT_STRING}${_VS_ENV_SEPARATOR}$(LocalDebuggerEnvironment)")
    endif ()

    foreach (_VS_CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
        # check for config-specific OUTPUT_NAME
        get_target_property(_VS_FILENAME ${_VS_TARGET} OUTPUT_NAME_${_VS_CONFIG})
        if (NOT _VS_FILENAME)
            # check for generic OUTPUT_NAME
            get_target_property(_VS_FILENAME ${_VS_TARGET} OUTPUT_NAME)
        endif ()
        if (NOT _VS_FILENAME)
            # use target name
            set(_VS_FILENAME ${_VS_TARGET})
        endif ()

        # check for config-specific POSTFIX
        get_target_property(_VS_POSTFIX ${_VS_TARGET} ${_VS_CONFIG}_POSTFIX)
        if (NOT _VS_POSTFIX AND ${_VS_CONFIG} STREQUAL "Debug")
            # check for special DEBUG_POSTFIX
            get_target_property(_VS_POSTFIX ${_VS_TARGET} DEBUG_POSTFIX)
        endif ()
        if (_VS_POSTFIX)
            set(_VS_FILENAME ${_VS_FILENAME}${_VS_POSTFIX})
        endif ()

        # check which executable to use
        if (_VS_INSTALL_DIR_EXECUTABLE)
            set(_VS_COMMAND "${CMAKE_INSTALL_PREFIX}/bin/${_VS_FILENAME}.exe")
        elseif (_VS_BINARY_DIR_EXECUTABLE)
            set(_VS_COMMAND "${_VS_USED_BINARY_DIR}/${_VS_CONFIG}/${_VS_FILENAME}.exe")
        endif ()

        # ensure absolute paths
        if (NOT IS_ABSOLUTE ${_VS_COMMAND})
            get_filename_component(_VS_COMMAND "${CMAKE_BINARY_DIR}/${_VS_COMMAND}" ABSOLUTE)
        endif ()
        file(TO_NATIVE_PATH ${_VS_COMMAND} _VS_COMMAND)
        if (NOT IS_ABSOLUTE ${_VS_WORKING_DIR})
            get_filename_component(_VS_WORKING_DIR "${CMAKE_BINARY_DIR}/${_VS_WORKING_DIR}" ABSOLUTE)
        endif ()
        file(TO_NATIVE_PATH ${_VS_WORKING_DIR} _VS_WORKING_DIR)

        # generate and concatenate per-config blocks
        if (_VS_PERCONFIG)
            set(_VS_PERCONFIG "${_VS_PERCONFIG}\n")
        endif ()
        string(CONFIGURE ${_VS_PERCONFIG_IN} _VS_PERCONFIG_OUT)
        set(_VS_PERCONFIG "${_VS_PERCONFIG}${_VS_PERCONFIG_OUT}")
    endforeach ()

    # create the .user file
    file(READ ${_VS_INFILE} _VS_INFILE_STRING)
    string(CONFIGURE ${_VS_INFILE_STRING} _VS_OUTFILE_STRING)
    _vs_write_outfile()

    # set the same configuration for the ALL_BUILD project
    if (_VS_SET_ALL_BUILD)
        if (MSVC_VERSION GREATER 1500)
            set(_VS_OUTFILE "${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user")
        else ()
            set(_VS_OUTFILE "${CMAKE_BINARY_DIR}/ALL_BUILD.vcproj.${_VS_HOSTNAME}.$ENV{USERNAME}.user")
        endif ()
        _vs_write_outfile()
    endif ()
endfunction ()

macro (_VS_WRITE_OUTFILE)
    if (NOT EXISTS ${_VS_OUTFILE} OR _VS_FORCE)
        file(WRITE "${_VS_OUTFILE}.tmp" ${_VS_OUTFILE_STRING})
        file(WRITE "${_VS_OUTFILE}" ${_VS_OUTFILE_STRING})
    else ()
        if (NOT EXISTS "${_VS_OUTFILE}.tmp")
            file(WRITE "${_VS_OUTFILE}.tmp" ${_VS_OUTFILE_STRING})
        endif ()

        string(SHA1 _VS_OUTFILE_STRING_SHA1 ${_VS_OUTFILE_STRING})
        file(READ "${_VS_OUTFILE}.tmp" _VS_TMPFILE_SHA1)
        string(SHA1 _VS_TMPFILE_SHA1 ${_VS_TMPFILE_SHA1})

        if (NOT (${_VS_OUTFILE_STRING_SHA1} STREQUAL ${_VS_TMPFILE_SHA1}))
            file(WRITE "${_VS_OUTFILE}.tmp" ${_VS_OUTFILE_STRING})
            file(WRITE "${_VS_OUTFILE}" ${_VS_OUTFILE_STRING})
        endif ()
    endif ()
endmacro ()
