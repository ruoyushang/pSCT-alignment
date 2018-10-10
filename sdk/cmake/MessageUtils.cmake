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
# Utilities for formatting and indenting messages
#
# STRING_INDENTATION can be set to modify the indentation depth of these
# utility functions.
###############################################################################

if (NOT STRING_INDENTATION)
    set(STRING_INDENTATION 50)
endif ()

### indent_string STR LEN
# helper function for indenting the string output; appends spaces until
# the string is LEN characters long
function (indent_string STR LEN)
    string(LENGTH ${STR} CURLEN)
    while (${CURLEN} LESS ${LEN})
        set(STR "${STR} ")
        string(LENGTH ${STR} CURLEN)
    endwhile ()
    set(MSG ${STR} PARENT_SCOPE)
endfunction ()

### list_package FOUND NAME [ VERSION ]
# helper function for listing found packages
# FOUND     a variable telling if the package was found (e.g. OPENSSL_FOUND)
# NAME      the name of the package (e.g. "UaOpenSSL")
# VERSION   optional: the version of the package (e.g. OPENSSL_VERSION)
function (list_package FOUND NAME)
    set(MSG "${NAME}")
    indent_string(${MSG} ${STRING_INDENTATION})
    if (${FOUND})
        if (${ARGC} GREATER 2)
            message(STATUS "${MSG}found (${${ARGV2}})")
        else ()
            message(STATUS "${MSG}found")
        endif ()
    else ()
        message(STATUS "${MSG}not found")
    endif ()
    unset(MSG)
endfunction ()

### display_message [ STR_LEFT [ STR_RIGHT ] ]
# helper function for displaying a message
# STR_LEFT  optional: the left side part of the message
# STR_RIGHT optional: the right side part of the message, intended by STRING_INDENTATION spaces
function (display_message)
    if (${ARGC} GREATER 0)
        set(MSG "${ARGV0}")
        if (${ARGC} GREATER 1)
            indent_string(${MSG} ${STRING_INDENTATION})

            # if STR_RIGHT is a list, concatenate them with semicolon
            set(CUR_ARGC 1)
            while (${CUR_ARGC} LESS ${ARGC})
                if (${CUR_ARGC} GREATER 1)
                    set(MSG "${MSG};")
                endif ()
                set(MSG "${MSG}${ARGV${CUR_ARGC}}")
                math(EXPR CUR_ARGC "${CUR_ARGC} + 1")
            endwhile ()
        endif ()
        message(STATUS "${MSG}")
        unset(MSG)
    else ()
        message(STATUS "")
    endif ()
endfunction ()

### display_variable VARIABLE_NAME
# helper function for displaying a variable and its content
# VARIABLE_NAME the name of the variable
function (display_variable VARIABLE_NAME)
    if (DEFINED ${VARIABLE_NAME})
        display_message(${VARIABLE_NAME} ${${VARIABLE_NAME}})
    else ()
        display_message(${VARIABLE_NAME} "Not defined")
    endif ()
endfunction ()

# display_separator [ SEPARATOR_NAME ]
# helper function for displaying a separator
# SEPARATOR_NAME optional: the message to display in the separator
function (display_separator)
    math(EXPR TMP_INDENT "${STRING_INDENTATION} / 2")
    set(MSG "#")
    string(LENGTH ${MSG} CURLEN)
    while (${CURLEN} LESS ${TMP_INDENT})
        set(MSG "${MSG}#")
        string(LENGTH ${MSG} CURLEN)
    endwhile ()

    if (${ARGC} GREATER 0)
        set(MSG "${MSG} ${ARGV0} ")
    endif ()

    math(EXPR TMP_INDENT "${STRING_INDENTATION} * 2")
    string(LENGTH ${MSG} CURLEN)
    while (${CURLEN} LESS ${TMP_INDENT})
        set(MSG "${MSG}#")
        string(LENGTH ${MSG} CURLEN)
    endwhile ()
    message(STATUS "${MSG}")
endfunction ()

# display_subseparator [ SEPARATOR_NAME ]
# helper function for displaying a second level separator
# SEPARATOR_NAME optional: the message to display in the separator
function (display_subseparator)
    set(MSG "###")
    if (${ARGC} GREATER 0)
        set(MSG "${MSG} ${ARGV0} ")
    endif ()
    message(STATUS "${MSG}")
endfunction ()

# display_project_header [ SUPPRESS_BLANK_LINE ]
# helper function for displaying a project header
# SUPPRESS_BLANK_LINE optional: don't prepend a blank line
function (display_project_header)
    if (${ARGC} EQUAL 0 OR NOT ${ARGV0})
        display_message()
    endif ()
    display_separator("Creating Makefile of ${PROJECT_NAME}")
endfunction ()
