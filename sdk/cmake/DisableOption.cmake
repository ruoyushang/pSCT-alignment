# This macro can disable BOOL options without losing the doc string
# Copyright (C) 2014 Gerhard Gappmeier

macro (disable_option option_name)
    get_property(disable_option_help_string CACHE "${option_name}" PROPERTY HELPSTRING)
    if (NOT disable_option_help_string)
        # set a dummy value if the option does not exist yet, otherwise
        # the following set fails as disable_option_help_string is empty
        set(disable_option_help_string "dummy value")
    endif ()
    set(${option_name} OFF CACHE BOOL ${disable_option_help_string} FORCE)
endmacro ()

