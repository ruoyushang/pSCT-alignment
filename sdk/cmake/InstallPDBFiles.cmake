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
# This file provides functions to install Visual Studio generated PDB files
# to a given _InstallDestination.
###############################################################################

# install_pdb_files [_ExecutableName] [_DebugPostfix] [_InstallDestination]
# _ExecutableName:       default = ${PROJECT_NAME}
# _DebugPostfix:         default = ${CMAKE_DEBUG_POSTFIX}
# _InstallDestination:   default = "bin"
function (install_pdb_files)
    if (WIN32 AND MSVC)
        set(_SkipInstallingPdbFiles OFF)

        set(_ExecutableName ${PROJECT_NAME})
        if (${ARGC} GREATER 0)
            set(_ExecutableName ${ARGV0})
        else ()
            # use the default PROJECT_NAME, now check if it's a SHARED_LIBRARY or EXECUTABLE
            get_target_property(TMP_PROJECT_TYPE ${PROJECT_NAME} TYPE)
            if (TMP_PROJECT_TYPE STREQUAL STATIC_LIBRARY OR TMP_PROJECT_TYPE STREQUAL MODULE_LIBRARY)
                set(_SkipInstallingPdbFiles ON)
            endif ()
        endif ()

        set(_DebugPostfix ${CMAKE_DEBUG_POSTFIX})
        if (${ARGC} GREATER 1)
            set(_DebugPostfix ${ARGV1})
        endif ()

        set(_InstallDestination "bin")
        if (${ARGC} GREATER 2)
            set(_InstallDestination ${ARGV2})
        endif ()

        if (NOT _SkipInstallingPdbFiles)
            if (MSVC_IDE)
                # when using the IDE the PDB files are in subfolders according to the
                # selected CMAKE_BUILD_TYPE.
                install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/${_ExecutableName}${_DebugPostfix}.pdb
                    DESTINATION ${_InstallDestination}
                    CONFIGURATIONS Debug
               )
                install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/${_ExecutableName}.pdb
                    DESTINATION ${_InstallDestination}
                    CONFIGURATIONS RelWithDebInfo
               )
            else ()
                # When building using NMake Makefiles the PDB files are in the current binary dir.
                install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_ExecutableName}${_DebugPostfix}.pdb
                    DESTINATION ${_InstallDestination}
                    CONFIGURATIONS Debug
               )
                install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_ExecutableName}.pdb
                    DESTINATION ${_InstallDestination}
                    CONFIGURATIONS RelWithDebInfo
               )
            endif ()
        endif ()

        unset(_SkipInstallingPdbFiles)
        unset(_ExecutableName)
        unset(_DebugPostfix)
        unset(_InstallDestination)
    endif ()
endfunction ()

# install_plugin_pdb_files [_ExecutableName] [_DebugPostfix]
# _ExecutableName:   default = ${PROJECT_NAME}
# _DebugPostfix:     default = ${CMAKE_DEBUG_POSTFIX}
#
# Sets _InstallDestination to "bin/plugins" and installs only if BUILD_SHARED_LIBS is set
function (install_plugin_pdb_files)
    if (WIN32 AND MSVC AND BUILD_SHARED_LIBS)
        set(_ExecutableName ${PROJECT_NAME})
        if (${ARGC} GREATER 0)
            set(_ExecutableName ${ARGV0})
        endif ()

        set(_DebugPostfix ${CMAKE_DEBUG_POSTFIX})
        if (${ARGC} GREATER 1)
            set(_DebugPostfix ${ARGV1})
        endif ()

        install_pdb_files(${_ExecutableName} ${_DebugPostfix} "bin/plugins")

        unset(_ExecutableName)
        unset(_DebugPostfix)
    endif ()
endfunction ()
