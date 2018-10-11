# Compiler configuration:
# This file enables all warnings for the used compiler to increase the code
# quality. Currently GCC and Visual Studio are supported.
# In additions various GCC options are made available: address sanitizer,
# code coverage, profile guided optimization, etc.
# (C) 2014 ascolab GmbH

################################################################################
cmake_policy(PUSH)
if (POLICY CMP0054)
    # set CMP0054 to new, as CMAKE_C_COMPILER_ID might contain MSVC
    # which leads to the warning being thrown below in the Clang check
    cmake_policy(SET CMP0054 NEW)
endif ()

include(DisableOption)
include(CMakeDependentOption)

set(CMAKE_DEBUG_POSTFIX "d")

macro (DISABLE_GCC_WARNING warningname)
    string(REPLACE "${warningname}" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    string(REPLACE "${warningname}" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endmacro ()

option(ENABLE_VERBOSE_MAKEFILE "Set to ON to enable verbose build output" OFF)
if (ENABLE_VERBOSE_MAKEFILE)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()

option(ENABLE_EXPERIMENTAL_OPTIONS "Set to ON to enable experimental options" OFF)

# GCC
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    if (NOT DEFINED GCC_VERSION)
        execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
        get_directory_property(TMP_PARENT_DIRECTORY PARENT_DIRECTORY)
        if (TMP_PARENT_DIRECTORY)
            # Set GCC_VERSION in parent scope to avoid logging it with every single invocation
            # of this script. Putting it in the cache leads to it being logged only once, this
            # variant leads to it being logged once with every cmake invocation.
            set(GCC_VERSION ${GCC_VERSION} PARENT_SCOPE)
        endif ()
        message(STATUS "Detected GCC variant: ${GCC_VERSION}")
    endif ()

    set(WARNINGS "${WARNINGS} -Wall -Wextra -Wformat=2 -Wunknown-pragmas -Wstrict-aliasing")
    set(WARNINGS "${WARNINGS} -Wpointer-arith -Wcast-align -Wmissing-noreturn")
    set(WARNINGS "${WARNINGS} -Wundef -Wwrite-strings")

    # C99 allows mixed declarations but MS doesn't support it, so gcc should warn about it.
    set(WARNINGS_C_ONLY "${WARNINGS_C_ONLY} -Wdeclaration-after-statement -Waggregate-return")

    if (GCC_VERSION VERSION_GREATER 4.1 OR GCC_VERSION VERSION_EQUAL 4.1)
        set(WARNINGS "${WARNINGS} -Wpragmas")
        set(WARNINGS "${WARNINGS} -Wattributes")
        set(WARNINGS_C_ONLY "${WARNINGS_C_ONLY} -Wpointer-to-int-cast")
    endif ()
    if (GCC_VERSION VERSION_GREATER 4.2 OR GCC_VERSION VERSION_EQUAL 4.2)
        set(WARNINGS "${WARNINGS} -Woverflow")
    endif ()
    if (GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
        set(WARNINGS "${WARNINGS} -Wlogical-op")
    endif ()
    if (GCC_VERSION VERSION_GREATER 4.4 OR GCC_VERSION VERSION_EQUAL 4.4)
        set(WARNINGS "${WARNINGS} -Wbuiltin-macro-redefined")
        set(WARNINGS "${WARNINGS} -Wsync-nand")
    endif ()

    # Wshadow throws needless warnings for GCC<5.0
    # See http://stackoverflow.com/questions/2958457/gcc-wshadow-is-too-strict
    if (GCC_VERSION VERSION_GREATER 5.0 OR GCC_VERSION VERSION_EQUAL 5.0)
        set(WARNINGS "${WARNINGS} -Wshadow")
    endif ()

    # removing const is sometimes necessary for string constants
    option(ENABLE_GCC_WARN_REMOVE_CONST_CAST "Set to ON to warn if const qualifier gets removed by a cast" OFF)
    if (ENABLE_GCC_WARN_REMOVE_CONST_CAST)
        set(WARNINGS "${WARNINGS} -Wcast-qual")
    endif ()

    # enable this to enforce strict C prototypes: e.g. int foo(void);
    option(ENABLE_GCC_STRICT_PROTOTYPES "Set to ON to enforce strict C prototypes: e.g. int foo(void);" OFF)
    if (ENABLE_GCC_STRICT_PROTOTYPES)
        set(WARNINGS_C_ONLY "${WARNINGS_C_ONLY} -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition")
    endif ()

    # warn if a global function is defined without a previous declaration.
    option(ENABLE_GCC_MISSING_DECL "Set to ON to warn if a global function is defined without a previous declaration" OFF)
    if (ENABLE_GCC_MISSING_DECL)
        set(WARNINGS "${WARNINGS} -Wmissing-declarations")
    endif ()

    # warn about some violations from Scott Meyers' Effective C++ series of books
    option(ENABLE_GCC_EFFCPP "Set to ON to warn about violations of Scott Meyers' Effective C++ series of books" OFF)
    if (ENABLE_GCC_EFFCPP)
        set(WARNINGS_CXX_ONLY "${WARNINGS_CXX_ONLY} -Weffc++")
    endif ()

    # enable all warnings
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 ${WARNINGS} ${WARNINGS_C_ONLY}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++98 ${WARNINGS} ${WARNINGS_CXX_ONLY}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker -Map=output.map")

    # GCC uses the option -fomit-framepointer by default. This causes problems with evaluating callstacks.
    # This optimization really only makes sense on 32bit (x86) because of a too small number of registers.
    # You can safely turn this on on other systems, as the performance impact is minimal.
    option(ENABLE_GCC_FRAME_POINTER "Set to ON to enable frame pointers to produce valid callstacks." ON)
    if (ENABLE_GCC_FRAME_POINTER)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
    endif()

    # add some some lightweight checks to be performed to detect some buffer overflow errors
    # when employing various string and memory manipulation functions.
    # see http://man7.org/linux/man-pages/man7/feature_test_macros.7.html
    option(ENABLE_GCC_FORTIFY_SOURCE "Set to ON to compile with -D_FORTIFY_SOURCE=1" ON)
    if (ENABLE_GCC_FORTIFY_SOURCE)
        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            disable_option(ENABLE_GCC_FORTIFY_SOURCE)
        else ()
            include(CheckSymbolExists)
            check_symbol_exists("_FORTIFY_SOURCE" "" _FORTIFY_SOURCE_DEFINED)
            if (NOT _FORTIFY_SOURCE_DEFINED)
                add_definitions(-D_FORTIFY_SOURCE=1)
            endif ()
        endif ()
    endif ()

    option(ENABLE_GCC_COVERAGE "Enables GCC's coverage analysis support." OFF)
    if (ENABLE_GCC_COVERAGE)
        # optimization has to be disabled for GCOV to deliver reasonable results
        if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            message(WARNING "Warning: ENABLE_GCC_COVERAGE is only usable when building configuration 'Debug'")
        else ()
            # this builds the executables with coverage info. Don't use ccache when enabling this.
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
            # NOTE: All following options have to be evaluated _after_ this point, otherwise
            #       the according flags get set and after that the option gets disabled.
            disable_option(ENABLE_GCC_ADDRESS_SANITIZER)
            disable_option(ENABLE_GCC_PROFILING)
            disable_option(ENABLE_GCC_LTO)
            disable_option(ENABLE_GCC_STRIP_DEAD_CODE)
        endif ()
    endif ()

    option(ENABLE_GCC_PEDANTIC "Set to ON to compile with -pedantic" OFF)
    if (ENABLE_GCC_PEDANTIC)
        # note that pedantic turns off all GNU extensions, so certain useful
        # features like static_assertions, #warning, container_of macro will
        # produce warnings (these are fine in the platform layer).
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
    endif ()

    # disable certain warnings (only if necessary)
    if (GCC_VERSION VERSION_GREATER 4.6 OR GCC_VERSION VERSION_EQUAL 4.6)
        # GCC>=4.6 warns about set but unused variables. This is not a bug and a
        # problem for certain variables with debug info
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
    endif ()
    if (GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8)
        # GCC>=4.8 supports memory checks using address sanitizer
        option(ENABLE_GCC_ADDRESS_SANITIZER "Enables GCC's address sanitizer to detect memory errors." OFF)
        if (ENABLE_GCC_ADDRESS_SANITIZER)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        endif ()
    endif ()

    option(ENABLE_GCC_PROFILING "Set to ON to enable GCC profiling" OFF)
    option(ENABLE_GCC_PGO       "Set to ON to enable profiling guided optimization (PGO)" OFF)
    # Profile Guided Optimization - Using GCC you can optimize according to a profile
    # 1.) Compile with ENABLE_GCC_PROFILING=On
    # 2.) Start program and run benchmark on it
    # 3.) Compile again with ENABLE_GCC_PGO=On
    if (ENABLE_GCC_PROFILING)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-generate")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-generate")
    endif ()
    if (ENABLE_GCC_PGO)
        set(CMAKE_C_FLAGS "-fprofile-use")
        set(CMAKE_CXX_FLAGS "-fprofile-use")
    endif ()
    option(ENABLE_GCC_LTO "Enable GCC's link time optimization" OFF)
    if (ENABLE_GCC_LTO)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
    endif ()

    option(ENABLE_GCC_GPROF "Set to ON to to generate profiling info for gprof" OFF)
    if (ENABLE_GCC_GPROF)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    endif ()

    if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "vxWorks")
        option(ENABLE_GCC_STRIP_DEAD_CODE "Enable GCC's --gc-sections feature to eliminate unused code" ON)
        if (ENABLE_GCC_STRIP_DEAD_CODE)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdata-sections -ffunction-sections")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
            # CMake sets "-rdynamic" by default in Linux-GNU.cmake. This instructs the
            # linker to add all symbols, not only used ones, to the dynamic symbol table.
            # This prevents the linker from "garbage-collecting" unused functions, so we need
            # to disable this flag.
            set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
        endif ()
    endif ()

    if (CMAKE_CROSSCOMPILING)
        find_program(CCACHE_APPLICATION ccache)
        if (CCACHE_APPLICATION)
            option(ENABLE_CCACHE_RULE "Prefix all compiler calls with 'ccache ', making use of ccache also for crosscompiling" OFF)
            if (ENABLE_CCACHE_RULE)
                set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
            endif ()
        endif ()
    endif ()
endif ()

# MSVC
if ("${CMAKE_MAKE_PROGRAM}" MATCHES "(msdev|devenv|nmake|MSBuild|VCExpress|jom)")
    # set highest warning level for visual studio
    set(C_WARNING_LEVEL 4)
    set(CXX_WARNING_LEVEL 4)

    # additional C warnings
    #   4062: enumerator 'identifier' in switch of enum 'enumeration' is not handled
    #   4191: 'operator/operation' : unsafe conversion from 'type of expression' to 'type required'
    #   4242: 'identifier' : conversion from 'type1' to 'type2', possible loss of data
    #   4254: 'operator' : conversion from 'type1' to 'type2', possible loss of data
    #   4263: 'function' : member function does not override any base class virtual member function
    #   4265: 'class' : class has virtual functions, but destructor is not virtual
    #   4287: 'operator' : unsigned/negative constant mismatch
    #   4289: nonstandard extension used : 'var' : loop control variable declared in the for-loop is used outside the for-loop scope
    #   4296: An unsigned variable was used in a comparison operation with zero.
    #   4302: 'conversion': truncation from 'type1' to 'type2'
    #   4905: wide string literal cast to 'LPSTR'
    #   4906: string literal cast to 'LPWSTR'
    set(ADDITIONAL_C_WARNINGS "/w44062 /w44191 /w44242 /w44254 /w44263 /w44265 /w44287 /w44289 /w44296 /w44302 /w44905 /w44906")
    # disable specific C++ warnings
    #   4127: conditional expression is constant
    #   4200: nonstandard extension used : zero-sized array in struct/union
    #   4201: nonstandard extension used : nameless struct/union
    #   4251: 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
    #   4512: 'class' : assignment operator could not be generated
    #   4306: 'identifier' : conversion from 'type1' to 'type2' of greater size
    set(ADDITIONAL_C_WARNINGS "${ADDITIONAL_C_WARNINGS} /wd4127 /wd4200 /wd4201 /wd4251 /wd4512 /wd4306")

    # additional C++ warnings
    #   4266: 'function' : no override available for virtual member function from base 'type'; function is hidden
    set(ADDITIONAL_CXX_WARNINGS "/w44266")
    # disable specific C++ warnings
    #   4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
    set(ADDITIONAL_CXX_WARNINGS "${ADDITIONAL_CXX_WARNINGS} /wd4530")

    # option for enabling /Wall, this will yield lots of errors for system headers
    option(ENABLE_ADDITIONAL_WARNINGS "Enable additional warnings using /Wall" OFF)
    if (ENABLE_ADDITIONAL_WARNINGS)
        # 4255: 'function': no function prototype given: converting '()' to '(void)'
        # 4820: 'bytes' bytes padding added after construct 'member_name'
        set(ADDITIONAL_C_WARNINGS "${ADDITIONAL_C_WARNINGS} /Wall /wd4255 /wd4820")
    endif ()

    # apply warning level and additional warnings to C compile flags
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W${C_WARNING_LEVEL} ${ADDITIONAL_C_WARNINGS}")

    # apply warning level and additional warnings to C++ compile flags
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W${CXX_WARNING_LEVEL} ${ADDITIONAL_C_WARNINGS} ${ADDITIONAL_CXX_WARNINGS}")

    # The /Z7 option is used to build static libs without
    # *.pdb files and therefore there are no warnings of
    # the linker ("missing *.pdb file") while linking
    string(REGEX REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REGEX REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REGEX REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REGEX REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    # Build against the static C / C++ runtime
    cmake_dependent_option(ENABLE_STATIC_CRT "Build against the static C runtime" OFF "ENABLE_EXPERIMENTAL_OPTIONS" OFF)
    if (ENABLE_STATIC_CRT)
        foreach (flag_var CMAKE_CXX_FLAGS
                          CMAKE_CXX_FLAGS_DEBUG
                          CMAKE_CXX_FLAGS_DEBUG_INIT
                          CMAKE_CXX_FLAGS_INIT
                          CMAKE_CXX_FLAGS_MINSIZEREL
                          CMAKE_CXX_FLAGS_MINSIZEREL_INIT
                          CMAKE_CXX_FLAGS_RELEASE
                          CMAKE_CXX_FLAGS_RELEASE_INIT
                          CMAKE_CXX_FLAGS_RELWITHDEBINFO
                          CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT
                          CMAKE_C_FLAGS
                          CMAKE_C_FLAGS_DEBUG
                          CMAKE_C_FLAGS_DEBUG_INIT
                          CMAKE_C_FLAGS_INIT
                          CMAKE_C_FLAGS_MINSIZEREL
                          CMAKE_C_FLAGS_MINSIZEREL_INIT
                          CMAKE_C_FLAGS_RELEASE
                          CMAKE_C_FLAGS_RELEASE_INIT
                          CMAKE_C_FLAGS_RELWITHDEBINFO
                          CMAKE_C_FLAGS_RELWITHDEBINFO_INIT)
            if (${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif ()
        endforeach ()
    endif ()

    # Don't warn about usage of deprecated CRT functions
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    # disable deprecated winsock API warnings for >= VS 2015
    if (NOT ${MSVC_VERSION} LESS 1900)
        add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
    endif ()

    # always compile with Unicode support
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)

    # The /MP option can reduce the total time to compile the source files.
    # It causes the compiler to create one or more copies of itself,
    # each in a separate process.
    if (MSVC AND MSVC_IDE AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsCE")
        add_definitions(/MP)
    endif ()
endif ()

# MinGW
if (MINGW)
    # This preprocessor define is used under MinGW only
    add_definitions(-D_NATIVE_WCHAR_T_DEFINED=1)

    # MinGW does not recognize the mapped types (e.g. OpcUa_Int32 etc.)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format")

    # always compile with Unicode support
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
endif ()

# CMAKE_C_COMPILER_ID is officially supported starting with CMake V 2.8.10
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (NOT DEFINED CLANG_VERSION)
        # This is just a gcc compatible version number.
        execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE CLANG_VERSION)
        get_directory_property(TMP_PARENT_DIRECTORY PARENT_DIRECTORY)
        if (TMP_PARENT_DIRECTORY)
            # Set CLANG_VERSION in parent scope to avoid logging it with every single invocation
            # of this script. Putting it in the cache leads to it being logged only once, this
            # variant leads to it being logged once with every cmake invocation.
            set(CLANG_VERSION ${CLANG_VERSION} PARENT_SCOPE)
        endif ()
        message(STATUS "Detected Clang variant: ${CLANG_VERSION}")
    endif ()

    # use the same warnings as for GCC, except the ones not supported: -Wlogical-op, -Wpragmas, -Wsync-nand
    set(WARNINGS "-Wall -Wextra -Wformat=2 -Woverflow -Wunknown-pragmas -Wstrict-aliasing")
    set(WARNINGS "${WARNINGS} -Wpointer-arith -Wcast-align")
    set(WARNINGS "${WARNINGS} -Wmissing-noreturn -Wbuiltin-macro-redefined")
    set(WARNINGS "${WARNINGS} -Wattributes -Wshadow")
    # clang seems to know this warning, but doesn't actually warn about it.
    set(WARNINGS_C_ONLY "${WARNINGS_C_ONLY} -Wdeclaration-after-statement")
    set(WARNINGS_C_ONLY "${WARNINGS_C_ONLY} -Wpointer-to-int-cast")

    # enable all warnings
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 ${WARNINGS} ${WARNINGS_C_ONLY}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++98 ${WARNINGS}")

    # Clang uses the option -fomit-framepointer by default. This causes problems with evaluating callstacks.
    # This optimization really only makes sense on 32bit (x86) because of a too small number of registers.
    # You can safely turn this on on other systems, as the performance impact is minimal.
    option(ENABLE_CLANG_FRAME_POINTER "Set to ON to enable frame pointers to produce valid callstacks." ON)
    if (ENABLE_CLANG_FRAME_POINTER)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
    endif()

    option(ENABLE_CLANG_COVERAGE "Enables Clang's coverage analysis support. Run lcov with '--gcov-tool llmv-cov'." OFF)
    if (ENABLE_CLANG_COVERAGE)
        # optimization has to be disabled for GCOV to deliver reasonable results
        if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            message(WARNING "Warning: ENABLE_CLANG_COVERAGE is only usable when building configuration 'Debug'")
        else ()
            # this builds the executables with coverage info. Don't use ccache when enabling this.
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
            # NOTE: All following options have to be evaluated _after_ this point, otherwise
            #       the according flags get set and after that the option gets disabled.
            #disable sanitizer as these add extra code that affects branch coverage
            disable_option(ENABLE_CLANG_SANITIZER_UB)
            disable_option(ENABLE_CLANG_SANITIZER_INTEGER)
            disable_option(ENABLE_CLANG_SANITIZER_UINT_OVERFLOW)
            disable_option(ENABLE_CLANG_SANITIZER_ADDRESS)
            disable_option(ENABLE_CLANG_SANITIZER_MEMORY)
        endif ()
    endif ()

    option(ENABLE_CLANG_PEDANTIC "Set to ON to compile with -pedantic" OFF)
    if (ENABLE_CLANG_PEDANTIC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
    endif ()

    option(ENABLE_CLANG_SANITIZER_UB "Enables Clang's sanitizer for undefined behavior, may be combined with other sanitizers." OFF)
    if (ENABLE_CLANG_SANITIZER_UB)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-sanitize-recover -fsanitize=undefined")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover -fsanitize=undefined")
    endif ()

    option(ENABLE_CLANG_SANITIZER_INTEGER "Enables Clang's sanitizer for undefined or suspicious integer behavior. Also detects unsigned integer overflows." OFF)
    if (ENABLE_CLANG_SANITIZER_INTEGER)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-sanitize-recover -fsanitize=integer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover -fsanitize=integer")
    endif ()

    option(ENABLE_CLANG_SANITIZER_UINT_OVERFLOW "Enables Clang's sanitizer for unsigned integer overflows. This most likely is defined behavior." OFF)
    if (ENABLE_CLANG_SANITIZER_UINT_OVERFLOW)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-sanitize-recover -fsanitize=unsigned-integer-overflow")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover -fsanitize=unsigned-integer-overflow")
    endif ()

    # to get debug symbols in call trace of address sanitizer or memory sanitizer
    # install llvm-symbolizer and include it in your PATH variable.

    # see http://clang.llvm.org/docs/AddressSanitizer.html
    cmake_dependent_option(ENABLE_CLANG_SANITIZER_ADDRESS "Enables Clang's sanitizer for memory error detection." OFF "NOT ENABLE_CLANG_SANITIZER_THREAD AND NOT ENABLE_CLANG_SANITIZER_MEMORY" OFF)
    if (ENABLE_CLANG_SANITIZER_ADDRESS)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
    endif ()

    # see http://http://clang.llvm.org/docs/ThreadSanitizer.html
    cmake_dependent_option(ENABLE_CLANG_SANITIZER_THREAD "Enables Clang's sanitizer for data race detection." OFF "NOT ENABLE_CLANG_SANITIZER_ADDRESS AND NOT ENABLE_CLANG_SANITIZER_MEMORY" OFF)
    if (ENABLE_CLANG_SANITIZER_THREAD)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    endif ()

    # see http://clang.llvm.org/docs/MemorySanitizer.html
    cmake_dependent_option(ENABLE_CLANG_SANITIZER_MEMORY "Enables Clang's EXPERIMENTAL detector of uninitialized reads." OFF "NOT ENABLE_CLANG_SANITIZER_ADDRESS AND NOT ENABLE_CLANG_SANITIZER_THREAD" OFF)
    if (ENABLE_CLANG_SANITIZER_MEMORY)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer")
    endif ()

    find_program(CCACHE_APPLICATION ccache)
    if (CCACHE_APPLICATION)
        option(ENABLE_CCACHE_RULE "Prefix all compiler calls with 'ccache ', making use of ccache also for crosscompiling" OFF)
        if (ENABLE_CCACHE_RULE)
            set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
        endif ()
    endif ()
endif ()

cmake_policy(POP)
