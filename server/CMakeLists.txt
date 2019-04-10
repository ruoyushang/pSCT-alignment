project(passerver)
cmake_minimum_required(VERSION 2.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Required to make folder targets

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") # Set all C++ compiler flags
set(CMAKE_PREFIX_PATH "/usr" ${CMAKE_PREFIX_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/cmake ${CMAKE_MODULE_PATH}) # NOTE: requires that sdk directory is on the same level as the server directory containing CMakeLists.txt
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/lib) # NOTE: requires that sdk directory is on the same level as the server directory containing CMakeLists.txt
set(COMMON_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common) # NOTE: requires that common directory is on the same level as the server directory containing CMakeLists.txt
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

include(CMakeDependentOption)
include(MessageUtils)
include(ConfigureCompiler)
include(ConfigureUaStack)
include(ConfigureCppSdk)

find_package(UaOpenSSL)
find_package(UaLibXml2)

find_package(PkgConfig REQUIRED)
find_package(ROOT REQUIRED)

# Attempt to locate and link MySQL connector libraries
pkg_search_module(MYSQLCONNECTORCPP REQUIRED mysqlcppconn)
if( MYSQLCONNECTORCPP_FOUND )
    link_directories(${MYSQLCONNECTORCPP_LIBRARY_DIRS})
else( MYSQLCONNECTORCPP_FOUND )
    message(WARNING "MySQLConnectorCPP not found. You need MySQLConnectorCPP for this program to work!")
endif( MYSQLCONNECTORCPP_FOUND )

# Attempt to locate and link OpenCV libraries
find_package(OpenCV REQUIRED)
if ( OpenCV_FOUND )
    message(STATUS "Found OpenCV: ${OpenCV_INSTALL_PATH} (version ${OpenCV_VERSION})")
endif(OpenCV_FOUND)
if( NOT(${OpenCV_VERSION} LESS 3.0))
    message(STATUS "OpenCV Version is greater than 3.0! Setting some defines for backward compatibility")
    add_definitions(-DWITH_OPENCV3)
endif ( NOT(${OpenCV_VERSION} LESS 3.0) )

# WARNING: need libusb.so -- location is hardcoded, need to select correct location for your current system
set(LIBUSB_LIBRARIES "/usr/local/lib/libusb-1.0.so.0")
#set(LIBUSB_LIBRARIES "/usr/lib64/libusb-1.0.so.0")
#SET(LIBUSB_LIBRARIES "/lib/x86_64-linux-gnu/libusb-1.0.so.0")
#SET(LIBUSB_LIBRARIES "${LD_LIBRARY_PATH}/libusb-1.0.so.0")

message(STATUS "")
message(STATUS "====================================")
message(STATUS "Creating Makefile of ${PROJECT_NAME}")
message(STATUS "====================================")
message(STATUS "")

####################################### Different Configurations ########################################
IF (NOT DEFINED ENV{MYSQL_HOST})
    MESSAGE(FATAL_ERROR "MYSQL_HOST environment variable not found")
ELSEIF (NOT DEFINED ENV{MYSQL_USER})
    MESSAGE(FATAL_ERROR "MYSQL_USER environment variable not found")
ELSEIF (NOT DEFINED ENV{MYSQL_DATABASE})
    MESSAGE(FATAL_ERROR "MYSQL_DATABASE environment variable not found")
ELSEIF (NOT DEFINED ENV{MYSQL_PASSWORD})
    MESSAGE(FATAL_ERROR "MYSQL_PASSWORD environment variable not found")
ELSEIF (NOT DEFINED ENV{MYSQL_PORT})
    MESSAGE(FATAL_ERROR "MYSQL_PORT environment variable not found")
else()
    MESSAGE(STATUS "Database variables found")
endif()

# User options (on/off)
option(USE_XMLCONFIG "Set to ON to build the XML-Parser and to use the ServerConfig.xml file for the server application." ON)
option(BUILD_SHARED_STACK "Set to ON to build the stack as shared object/dll" ON)
option(UASTACK_WITH_OPENSSL "Set to OFF to build the Stack without OpenSSL support. IF OFF, BASIC128RSA15 BASIC256 and PKI is set to OFF." ON)
option(BUILD_BASIC_CONFIG "Build SDK in Basic Configuration" OFF)

if (BUILD_BASIC_CONFIG)
    add_definitions(-DSUPPORT_Method_Server_Facet=0 -DSUPPORT_Event_Subscription_Server_Facet=0 -DSUPPORT_Historical_Access=0 -DSUPPORT_Node_Management_Server_Facet=0 -DSUPPORT_TRACE_CONFIGURATION=0)
else (BUILD_BASIC_CONFIG)
    add_definitions(-DSUPPORT_Method_Server_Facet=1 -DSUPPORT_Event_Subscription_Server_Facet=1 -DSUPPORT_Historical_Access=1 -DSUPPORT_Node_Management_Server_Facet=1 -DSUPPORT_TRACE_CONFIGURATION=1)
endif (BUILD_BASIC_CONFIG)

if (NOT UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)
    message(FATAL_ERROR "It is not allowed to to build the application without UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS")
endif (NOT UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)

if (BUILD_SHARED_STACK)
    add_definitions(-D_UA_STACK_USE_DLL)
endif (BUILD_SHARED_STACK)

# configure Dll/Lib
if (BUILD_SHARED_LIBS)
    add_definitions(-D_UA_STACK_USE_DLL)
    add_definitions(-D_UA_BASE_USE_DLL)
    add_definitions(-D_SERVER_CORE_USE_DLL)
    add_definitions(-D_UA_MODULE_USE_DLL)
    add_definitions(-D_UAMODELS_USE_DLL)
    if ( USE_XMLCONFIG )
        add_definitions(-D_XML_PARSER_USE_DLL)
    endif ( USE_XMLCONFIG )
    if ( UASTACK_WITH_OPENSSL )
        add_definitions(-D_UA_PKI_USE_DLL)
    endif ()
endif (BUILD_SHARED_LIBS)

if ( USE_XMLCONFIG )
    set(CONFIG_FILE ServerConfig.xml)
else ( USE_XMLCONFIG )
    set(CONFIG_FILE ServerConfig.ini)
endif ( USE_XMLCONFIG )

# Locate all source files to build
file(GLOB PASSERVER_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/objects/*.cpp"
    "${COMMON_CODE_DIR}/opcua/*.cpp"
    "${COMMON_CODE_DIR}/utilities/*.cpp"
    "${COMMON_CODE_DIR}/alignment/*.cpp"
    "${COMMON_CODE_DIR}/mpescode/*.cpp"
)

# Configure for simulation mode (add additional source files)
if ( SIMMODE )
    add_definitions(-D_AMD64)
    add_definitions(-DSIMMODE)
    file(GLOB PASSERVER_SRC
        ${PASSERVER_SRC}
        "${COMMON_CODE_DIR}/cbccode/dummycbc.cpp"
        )
else ( SIMMODE )
    file(GLOB PASSERVER_SRC
        ${PASSERVER_SRC}
        "${COMMON_CODE_DIR}/cbccode/*.cpp"
        "${COMMON_CODE_DIR}/globalalignment/psdclass.cpp"
        "${COMMON_CODE_DIR}/globalalignment/laserclass.cpp"
        "${COMMON_CODE_DIR}/globalalignment/libpiusb.cpp"
    )
endif ( SIMMODE )

# Configure for 64-bit compilation (add additional source files)
if ( AMD64 )
    add_definitions(-D_AMD64)
    file(GLOB PASSERVER_SRC
        ${PASSERVER_SRC}
        "${COMMON_CODE_DIR}/cbccode/dummycbc.cpp"
        )
else ( AMD64 )
    file(GLOB PASSERVER_SRC
        ${PASSERVER_SRC}
        "${COMMON_CODE_DIR}/cbccode/*.cpp"
        "${COMMON_CODE_DIR}/globalalignment/psdclass.cpp"
        "${COMMON_CODE_DIR}/globalalignment/laserclass.cpp"
        "${COMMON_CODE_DIR}/globalalignment/libpiusb.cpp"
    )
endif ( AMD64 )

####################################### Include Paths ########################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${UASDK_BASE_DIR}/include)
include_directories(${UASTACK_INCLUDE})
include_directories(${UABASE_INCLUDE})
include_directories(${UASERVER_INCLUDE})
include_directories(${UACOREMODULE_INCLUDE})
include_directories(${OpenCV_INCLUDE_DIR})
MESSAGE("ROOT_INCLUDES=${ROOT_INCLUDES}")
include_directories(${ROOT_INCLUDES})
include_directories(${MYSQLCONNECTORCPP_INCLUDE_DIRS})

# Add extra include paths for OpenSSL and XML configurations
if (UASTACK_WITH_OPENSSL)
    include_directories(${OPENSSL_INCLUDE_DIR})
    include_directories(${UAPKI_INCLUDE})
endif (UASTACK_WITH_OPENSSL)
if (USE_XMLCONFIG)
    include_directories(${LIBXML2_INCLUDE_DIR})
    include_directories(${UAXMLPARSER_INCLUDE})
endif (USE_XMLCONFIG)

# Specify where linker should look for OPC UA libraries
if (NOT TARGET uastack)
    link_directories(${UA_LIB_DIR})
endif (NOT TARGET uastack)

####################################### Different Platforms ########################################

# Add define flags and set correct platform + platform-specific libraries

if (WIN32)
    set(OS win32)
    # add windows-required define flags
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -DUNICODE -D_UNICODE)

    if (WIN32 AND NOT WINDOWS_CE)
        set(PLATTFORM_LIBS Rpcrt4 ws2_32 Version oleaut32 ole32)
    else (WIN32 AND NOT WINDOWS_CE)
        # additional dependencies on windows CE
        set(PLATTFORM_LIBS ${WCECOMPAT_LIBRARIES} ws2 mmtimer)
    endif (WIN32 AND NOT WINDOWS_CE)
endif (WIN32)

if (UNIX)
    add_definitions(-Wall) # Turn on all compiler warnings
    set(PLATTFORM_LIBS pthread rt)
endif (UNIX)

if (QNXNTO)
    add_definitions(-Wall)
    set(PLATTFORM_LIBS socket z)
    if (USE_XMLCONFIG)
        set(PLATTFORM_LIBS ${PLATTFORM_LIBS} ${ICONV_LIBRARIES})
    endif (USE_XMLCONFIG)
endif (QNXNTO)

if (APPLE)
    add_definitions(-Wall)
endif (APPLE)

if (vxWorks)
    add_definitions(-Wall)
endif (vxWorks)

####################################### Create executable ###################################

add_executable(${PROJECT_NAME} ${PASSERVER_SRC})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11) # Set C++ standard to build against

if (UASTACK_WITH_OPENSSL)
    set(PLATTFORM_LIBS ${PLATTFORM_LIBS} ${OPENSSL_LIBRARIES})
    if (NOT TARGET uastack)
        set(PLATTFORM_LIBS ${PLATTFORM_LIBS} ${UAPKI_LIBRARY})
    else (NOT TARGET uastack)
        set(PLATTFORM_LIBS ${PLATTFORM_LIBS} uapki)
    endif (NOT TARGET uastack)
    if (WIN32)
        set(PLATTFORM_LIBS ${PLATTFORM_LIBS} crypt32)
    endif (WIN32)
endif (UASTACK_WITH_OPENSSL)

if (USE_XMLCONFIG)
    set(PLATTFORM_LIBS ${PLATTFORM_LIBS} ${LIBXML2_LIBRARIES})
    if (NOT TARGET uastack)
        set(PLATTFORM_LIBS ${PLATTFORM_LIBS} ${UAXML_LIBRARY})
    else (NOT TARGET uastack)
        set(PLATTFORM_LIBS ${PLATTFORM_LIBS} xmlparser)
    endif (NOT TARGET uastack)
endif (USE_XMLCONFIG)

####################################### Link Libraries ########################################

message("OPC UA library directory (UA_LIB_DIR) = ${UA_LIB_DIR}")
if (NOT TARGET uastack)
    target_link_libraries(${PROJECT_NAME} ${UAMODULE_LIBRARY} ${UAMODELS_LIBRARY} ${UACOREMODULE_LIBRARY} ${UABASE_LIBRARY} ${UASTACK_LIBRARY})
else (NOT TARGET uastack)
    target_link_libraries(${PROJECT_NAME} uamodule uamodels coremodule uabase uastack)
endif (NOT TARGET uastack)
target_link_libraries(${PROJECT_NAME}
        ${PLATTFORM_LIBS} ${OpenCV_LIBRARIES} ${ROOT_LIBRARIES} ${MYSQLCONNECTORCPP_LIBRARIES} ${LIBUSB_LIBRARIES})

####################################### configure debug postfix #############################

# Specify postfix to be attached to names of debug executables
set_target_properties(${PROJECT_NAME}
                        PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}
        DEBUG_POSTFIX d${WCELINKERFLAGS})

# Specify destinations for installing different build file types
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# installs the configuration file of the server if folder exists
if (EXISTS ${KM_FOLDER})
    INSTALL_IF_NEWER("${KM_FOLDER}/sdk/bin/${CONFIG_FILE}" "${CMAKE_INSTALL_PREFIX}/bin/${CONFIG_FILE}")
endif (EXISTS ${KM_FOLDER})

####################################### Install PDB ########################################

# When compiling for Windows, generate PDB files containing
# information required when running the program in debug mode
if (WIN32)
    if (MSVC)
        if (MSVC_IDE)
            install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/${PROJECT_NAME}${CMAKE_DEBUG_POSTFIX}.pdb
                DESTINATION bin
                CONFIGURATIONS Debug
            )
            install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/${PROJECT_NAME}.pdb
                DESTINATION bin
                CONFIGURATIONS RelWithDebInfo
            )
        ELSE (MSVC_IDE)
            install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_DEBUG_POSTFIX}.pdb
                DESTINATION bin
                CONFIGURATIONS Debug
            )
            install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pdb
                DESTINATION bin
                CONFIGURATIONS RelWithDebInfo
            )
        endif (MSVC_IDE)
    endif (MSVC)
endif (WIN32)
