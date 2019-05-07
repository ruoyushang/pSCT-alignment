project(passerver)
cmake_minimum_required(VERSION 2.8)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/cmake ${CMAKE_MODULE_PATH})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CMakeDependentOption)
include(MessageUtils)

find_package(UaOpenSSL)
find_package(UaLibXml2)

include(ConfigureCompiler)
include(ConfigureUaStack)
include(ConfigureCppSdk)

display_project_header("true")

FIND_PACKAGE(PkgConfig REQUIRED)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})
FIND_PACKAGE( ROOT REQUIRED )
pkg_search_module( MYSQLCONNECTORCPP REQUIRED mysqlcppconn )
if( MYSQLCONNECTORCPP_FOUND )
    link_directories(${MYSQLCONNECTORCPP_LIBRARY_DIRS})
else( MYSQLCONNECTORCPP_FOUND )
    message(WARNING "MySQLConnectorCPP not found. You need MySQLConnectorCPP for this program to work!")
endif( MYSQLCONNECTORCPP_FOUND )

SET(CMAKE_PREFIX_PATH "/usr" ${CMAKE_PREFIX_PATH} )
FIND_PACKAGE( OpenCV REQUIRED )
if ( OpenCV_FOUND )
    message(STATUS "Found OpenCV: ${OpenCV_INSTALL_PATH} (version ${OpenCV_VERSION})")
endif(OpenCV_FOUND)
if( NOT(${OpenCV_VERSION} LESS 3.0))
    message(STATUS "OpenCV Version is greater than 3.0! Setting some defines for backward compatibility")
    add_definitions(-DWITH_OPENCV3)
endif ( NOT(${OpenCV_VERSION} LESS 3.0) )

SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
if ( SIMMODE OR AMD64 )
    SET(CMAKE_INSTALL_RPATH ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/lib64)
    SET (UA_LIB_DIR ${UASDK_BASE_DIR}/lib64)
else ( SIMMODE OR AMD64 )
    SET(CMAKE_INSTALL_RPATH ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/lib)
endif ( SIMMODE OR AMD64 )
SET(COMMON_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

# need libusb.so -- might not be on the system!!
# brute force it for now
#SET(LIBUSB_LIBRARIES "/home/root/opcua/pascommon/globalalignment/GlobalAlignment/libusb-1.0.so")
#SET(LIBUSB_LIBRARIES "/usr/local/lib/libusb-1.0.so.0")
SET(LIBUSB_LIBRARIES "/usr/lib64/libusb-1.0.so.0")
#SET(LIBUSB_LIBRARIES "/lib/x86_64-linux-gnu/libusb-1.0.so.0")
#SET(LIBUSB_LIBRARIES "${LD_LIBRARY_PATH}/libusb-1.0.so.0")

MESSAGE( STATUS "" )
MESSAGE( STATUS "====================================" )
MESSAGE( STATUS "Creating Makefile of ${PROJECT_NAME}" )
MESSAGE( STATUS "====================================" )
MESSAGE( STATUS "" )

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
ELSE (BUILD_BASIC_CONFIG)
    add_definitions(-DSUPPORT_Method_Server_Facet=1 -DSUPPORT_Event_Subscription_Server_Facet=1 -DSUPPORT_Historical_Access=1 -DSUPPORT_Node_Management_Server_Facet=1 -DSUPPORT_TRACE_CONFIGURATION=1)
ENDIF (BUILD_BASIC_CONFIG)

IF ( NOT UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)
    MESSAGE(FATAL_ERROR "It is not allowed to to build the application without UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS")
ENDIF ( NOT UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)

# Build with shared stack
IF (BUILD_SHARED_STACK)
    add_definitions(-D_UA_STACK_USE_DLL)
ENDIF (BUILD_SHARED_STACK)

# configure Dll/Lib
if (BUILD_SHARED_LIBS)
    add_definitions(-D_UA_STACK_USE_DLL)
    add_definitions(-D_UA_BASE_USE_DLL)
    if ( USE_XMLCONFIG )
        add_definitions(-D_XML_PARSER_USE_DLL)
    endif ( USE_XMLCONFIG )
    if ( UASTACK_WITH_OPENSSL )
        add_definitions(-D_UA_PKI_USE_DLL)
    endif ()
    add_definitions(-D_SERVER_CORE_USE_DLL)
    add_definitions(-D_UA_MODULE_USE_DLL)
    add_definitions(-D_UAMODELS_USE_DLL)
endif (BUILD_SHARED_LIBS)

if ( USE_XMLCONFIG )
    SET(CONFIG_FILE ServerConfig.xml)
else ( USE_XMLCONFIG )
    SET(CONFIG_FILE ServerConfig.ini)
endif ( USE_XMLCONFIG )

file(GLOB PASSERVER_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/objects/*.cpp"
    "${COMMON_CODE_DIR}/opcua/*.cpp"
    "${COMMON_CODE_DIR}/utilities/*.cpp"
    "${COMMON_CODE_DIR}/alignment/*.cpp"
    "${COMMON_CODE_DIR}/mpescode/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/objects/*.hpp"
        "${COMMON_CODE_DIR}/opcua/*.hpp"
        "${COMMON_CODE_DIR}/utilities/*.hpp"
        "${COMMON_CODE_DIR}/alignment/*.hpp"
        "${COMMON_CODE_DIR}/mpescode/*.h"
)

if ( SIMMODE )
    add_definitions(-D_AMD64)
    add_definitions(-DSIMMODE)
    file(GLOB PASSERVER_SRC
        ${PASSERVER_SRC}
        "${COMMON_CODE_DIR}/cbccode/dummycbc.cpp"
            "${COMMON_CODE_DIR}/cbccode/dummycbc.hpp"
        )
else ( SIMMODE )
    file(GLOB PASSERVER_SRC
        ${PASSERVER_SRC}
        "${COMMON_CODE_DIR}/cbccode/*.cpp"
        "${COMMON_CODE_DIR}/globalalignment/psdclass.cpp"
        "${COMMON_CODE_DIR}/globalalignment/laserclass.cpp"
        "${COMMON_CODE_DIR}/globalalignment/libpiusb.cpp"
            "${COMMON_CODE_DIR}/cbccode/*.hpp"
            "${COMMON_CODE_DIR}/globalalignment/psdclass.h"
            "${COMMON_CODE_DIR}/globalalignment/laserclass.h"
            "${COMMON_CODE_DIR}/globalalignment/libpiusb.h"
    )
endif ( SIMMODE )

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
            "${COMMON_CODE_DIR}/cbccode/*.hpp"
            "${COMMON_CODE_DIR}/globalalignment/psdclass.h"
            "${COMMON_CODE_DIR}/globalalignment/laserclass.h"
            "${COMMON_CODE_DIR}/globalalignment/libpiusb.h"
    )
endif ( AMD64 )

####################################### Include Paths ########################################
IF ( UASTACK_WITH_OPENSSL )
    include_directories(${OPENSSL_INCLUDE_DIR})
ENDIF ()
IF (USE_XMLCONFIG)
    include_directories(${LIBXML2_INCLUDE_DIR})
ENDIF (USE_XMLCONFIG)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${COMMON_CODE_DIR}/alignment)
include_directories(${COMMON_CODE_DIR}/cbccode)
include_directories(${COMMON_CODE_DIR}/opcua)
include_directories(${COMMON_CODE_DIR}/simulatestewart)
include_directories(${COMMON_CODE_DIR}/utilities)
include_directories(${UASDK_BASE_DIR}/include)
include_directories(${UASTACK_INCLUDE})
include_directories(${UABASE_INCLUDE})
include_directories(${UASERVER_INCLUDE})
include_directories(${UACOREMODULE_INCLUDE})
include_directories(${COMMON_CODE_DIR}/mpescode)
include_directories(${COMMON_CODE_DIR}/cbccode)
include_directories(${COMMON_CODE_DIR}/opcua)
include_directories(${COMMON_CODE_DIR}/alignment)
include_directories(${COMMON_CODE_DIR}/globalalignment)
include_directories(${COMMON_CODE_DIR}/utilities)
include_directories(${COMMON_CODE_DIR}/utilities/linux)
include_directories(${OpenCV_INCLUDE_DIR})
include_directories(${ROOT_INCLUDE_DIRS})
include_directories(${MYSQLCONNECTORCPP_INCLUDE_DIRS})

IF (USE_XMLCONFIG)
    include_directories(${UAXMLPARSER_INCLUDE})
ENDIF (USE_XMLCONFIG)
IF ( UASTACK_WITH_OPENSSL )
    include_directories(${UAPKI_INCLUDE})
ENDIF ()

# LINKER PATHS
IF ( NOT TARGET uastack )
    link_directories(${UA_LIB_DIR})
ENDIF ( NOT TARGET uastack )

####################################### Different Platforms ########################################

IF (WIN32)
    SET(OS win32)
    # add windows definitions
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -DUNICODE -D_UNICODE)

    IF ( WIN32 AND NOT WINDOWS_CE )
        SET(PLATTFORM_LIBS Rpcrt4 ws2_32 Version oleaut32 ole32)
    ELSE ( WIN32 AND NOT WINDOWS_CE )
        # additional dependencies on windows CE
        SET(PLATTFORM_LIBS ${WCECOMPAT_LIBRARIES} ws2 mmtimer)
    ENDIF ( WIN32 AND NOT WINDOWS_CE )
ENDIF (WIN32)

IF (UNIX)
    ADD_DEFINITIONS(-Wall)
    SET(PLATTFORM_LIBS pthread rt)
ENDIF (UNIX)

IF (QNXNTO)
    ADD_DEFINITIONS(-Wall)
    SET(PLATTFORM_LIBS socket z)
    IF ( USE_XMLCONFIG )
        SET(PLATTFORM_LIBS ${PLATTFORM_LIBS} ${ICONV_LIBRARIES})
    ENDIF ( USE_XMLCONFIG )
ENDIF (QNXNTO)

IF (APPLE)
    ADD_DEFINITIONS(-Wall)
ENDIF (APPLE)

IF (vxWorks)
    ADD_DEFINITIONS(-Wall)
ENDIF (vxWorks)

####################################### Create executable ###################################

add_executable(${PROJECT_NAME} ${PASSERVER_SRC})

IF ( UASTACK_WITH_OPENSSL )
    IF ( NOT TARGET uastack )
        SET(PLATTFORM_LIBS ${PLATTFORM_LIBS} ${UAPKI_LIBRARY} ${OPENSSL_LIBRARIES})
        IF (WIN32)
            SET(PLATTFORM_LIBS ${PLATTFORM_LIBS} crypt32)
        ENDIF (WIN32)
    ELSE ()
        SET(PLATTFORM_LIBS ${PLATTFORM_LIBS} uapki ${OPENSSL_LIBRARIES})
        IF (WIN32)
            SET(PLATTFORM_LIBS ${PLATTFORM_LIBS} crypt32)
        ENDIF (WIN32)
    ENDIF ()
ENDIF ()

IF (USE_XMLCONFIG)
    IF ( NOT TARGET uastack )
        SET(PLATTFORM_LIBS ${PLATTFORM_LIBS} ${UAXML_LIBRARY} ${LIBXML2_LIBRARIES})
    ELSE ()
        SET(PLATTFORM_LIBS ${PLATTFORM_LIBS} xmlparser ${LIBXML2_LIBRARIES})
    ENDIF ()
ENDIF (USE_XMLCONFIG)

####################################### Link Library ########################################

SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )

IF ( NOT TARGET uastack )
    message(WARNING " UA_LIB_DIR = ${UA_LIB_DIR}")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}       ${UAMODULE_LIBRARY}
                          ${UAMODELS_LIBRARY}  ${UACOREMODULE_LIBRARY}
                          ${UABASE_LIBRARY}    ${UASTACK_LIBRARY}
                          ${PLATTFORM_LIBS} ${OpenCV_LIBRARIES} ${ROOT_LIBRARIES} ${MYSQLCONNECTORCPP_LIBRARIES} ${LIBUSB_LIBRARIES})

ELSE ()
    message(WARNING " OpenCV_LIBRARIES = ${OpenCV_LIBRARIES}")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} uamodule uamodels coremodule uabase
        uastack ${PLATTFORM_LIBS} ${OpenCV_LIBRARIES} ${ROOT_LIBRARIES} ${MYSQLCONNECTORCPP_LIBRARIES} ${LIBUSB_LIBRARIES})
ENDIF ()

####################################### configure debug postfix #############################

SET_TARGET_PROPERTIES(  ${PROJECT_NAME}
                        PROPERTIES
                         OUTPUT_NAME ${PROJECT_NAME}
                         DEBUG_POSTFIX d
                         ${WCELINKERFLAGS}
)

INSTALL(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

# installs also the configuration file of the server if folder exists
IF ( EXISTS ${KM_FOLDER} )
    INSTALL_IF_NEWER("${KM_FOLDER}/sdk/bin/${CONFIG_FILE}" "${CMAKE_INSTALL_PREFIX}/bin/${CONFIG_FILE}")
ENDIF ( EXISTS ${KM_FOLDER} )

####################################### Install PDB ########################################

IF (WIN32)
    IF(MSVC)
        IF(MSVC_IDE)
            INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/${PROJECT_NAME}${CMAKE_DEBUG_POSTFIX}.pdb
                DESTINATION bin
                CONFIGURATIONS Debug
            )
            INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/${PROJECT_NAME}.pdb
                DESTINATION bin
                CONFIGURATIONS RelWithDebInfo
            )
        ELSE ()
            INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_DEBUG_POSTFIX}.pdb
                DESTINATION bin
                CONFIGURATIONS Debug
            )
            INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pdb
                DESTINATION bin
                CONFIGURATIONS RelWithDebInfo
            )
        ENDIF()
    ENDIF()
ENDIF (WIN32)
