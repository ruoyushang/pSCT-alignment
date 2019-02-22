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
###############################################################################

# install_if_newer SRC_FILE DST_FILE
function (install_if_newer SRC_FILE DST_FILE)
    file(TO_CMAKE_PATH ${SRC_FILE} SRC_FILE)
    file(TO_CMAKE_PATH ${DST_FILE} DST_FILE)
    get_filename_component(DST_FILE_ABS "${DST_FILE}" ABSOLUTE)
    install(CODE
        "
        if (\"${SRC_FILE}\" IS_NEWER_THAN \"${DST_FILE}\")
            if (NOT ( ${CMAKE_VERSION} VERSION_LESS \"3.1\" ) AND NOT ( ${CMAKE_INSTALL_MESSAGE} MATCHES \"NEVER\" ) )
                message(STATUS \"Installing: ${DST_FILE_ABS}\")
            endif ()
            configure_file(\"${SRC_FILE}\" \"${DST_FILE}\" COPYONLY)
        else ()
            if (NOT ( ${CMAKE_VERSION} VERSION_LESS \"3.1\" ) AND NOT ( ${CMAKE_INSTALL_MESSAGE} MATCHES \"(NEVER|LAZY)\" ) )
                message(STATUS \"Up-to-date: ${DST_FILE_ABS}\")
            endif ()
        endif ()
        ")
endfunction ()
