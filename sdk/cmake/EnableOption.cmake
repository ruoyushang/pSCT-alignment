# This macro enables an option even if it is not yet existing
# Copyright (C) 2014 Hannes Mezger

macro (enable_option option_name)
    get_property(enable_option_help_string CACHE "${option_name}" PROPERTY HELPSTRING)
    if (NOT enable_option_help_string)
        # set a dummy value if the option does not exist yet, otherwise
        # the following set fails as enable_option_help_string is empty
        set(enable_option_help_string "dummy value")
    endif ()
    set(${option_name} ON CACHE BOOL ${enable_option_help_string} FORCE)
endmacro ()
