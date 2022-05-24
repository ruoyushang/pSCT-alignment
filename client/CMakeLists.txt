PROJECT (p2pasclient)
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
FIND_PACKAGE( Eigen3 REQUIRED )
FIND_PACKAGE( ROOT REQUIRED COMPONENTS Minuit)

if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
#SET(CMAKE_INSTALL_RPATH ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/lib64 ${CMAKE_CURRENT_SOURCE_DIR}/../../lib ${CMAKE_CURRENT_SOURCE_DIR}/../../lib64)
SET(UA_LIB_DIR ${UASDK_BASE_DIR}/lib64)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

SET(COMMON_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common)

pkg_search_module( MYSQLCONNECTORCPP REQUIRED mysqlcppconn )
if( MYSQLCONNECTORCPP_FOUND )
    link_directories(${MYSQLCONNECTORCPP_LIBRARY_DIRS})
else( MYSQLCONNECTORCPP_FOUND )
    message(WARNING "MySQLConnectorCPP not found. You need MySQLConnectorCPP for this program to work!")
endif( MYSQLCONNECTORCPP_FOUND )

pkg_search_module(ARAVIS REQUIRED aravis-0.6 aravis-0.4)
if(ARAVIS_FOUND)
    link_directories(${ARAVIS_LIBRARY_DIRS})
else(NOT ARAVIS_FOUND)
    message(WARNING "Aravis not found. You need Aravis for this program to work!")
endif(ARAVIS_FOUND)

pkg_search_module(ROBAST REQUIRED ROBAST)
if(ROBAST_FOUND)
    link_directories(${ROBAST_LIBRARY_DIRS})
else(NOT ROBAST_FOUND)
    message(WARNING "ROBAST not found. You need ROBAST for this program to work!")
endif(ROBAST_FOUND)

pkg_search_module(CCFITS REQUIRED CCfits)
if(CCFITS_FOUND)
    link_directories(${CCFITS_LIBRARY_DIRS})
else(NOT CCFITS_FOUND)
    message(WARNING "CCFits not found. You need CCFits for this program to work!")
endif(CCFITS_FOUND)

pkg_search_module(NLOPT REQUIRED nlopt)
if(NLOPT_FOUND)
    link_directories(${NLOPT_LIBRARY_DIRS})
else(NOT NLOPT_FOUND)
    message(WARNING "NLopt not found. You need NLopt for this program to work!")
endif(NLOPT_FOUND)

find_package(Boost 1.50.0 REQUIRED COMPONENTS
        filesystem regex)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
else(Boost_FOUND)
    message(WARNING "Boost not found. You need Boost for this program to work!")
endif(Boost_FOUND)

MESSAGE( STATUS "" )
MESSAGE( STATUS "====================================" )
MESSAGE( STATUS "Creating Makefile of ${PROJECT_NAME}" )
MESSAGE( STATUS "====================================" )
MESSAGE( STATUS "" )

####################################### Different Configurations ########################################

OPTION( USE_XMLCONFIG           "Set to ON to build the XML-Parser and to use the ServerConfig.xml file for the server application." ON )
OPTION( BUILD_SHARED_STACK      "Set to ON to build the stack as shared object/dll" ON )
OPTION( UASTACK_WITH_OPENSSL    "Set to OFF to build the Stack without OpenSSL support. IF OFF, BASIC128RSA15 BASIC256 and PKI is set to OFF." ON )

# Build with shared stack
IF (BUILD_SHARED_STACK)
    add_definitions(-D_UA_STACK_USE_DLL)
ENDIF (BUILD_SHARED_STACK)

IF ( NOT UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)
    MESSAGE(FATAL_ERROR "It is not allowed to to build the application without UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS")
ENDIF ( NOT UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)

# configure Dll/Lib
if (BUILD_SHARED_LIBS)
    add_definitions(-D_UA_STACK_USE_DLL)
    add_definitions(-D_UA_BASE_USE_DLL)
    if ( UASTACK_WITH_OPENSSL )
        add_definitions(-D_UA_PKI_USE_DLL)
    endif ()
    if ( USE_XMLCONFIG )
        add_definitions(-D_XML_PARSER_USE_DLL)
    endif ( USE_XMLCONFIG )
    add_definitions(-D_UA_CLIENT_USE_DLL)
endif (BUILD_SHARED_LIBS)

FILE(GLOB P2PAS_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/pascommunicationinterface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pasnodemanager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pasobjectfactory.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/clienthelper.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/objects/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utilities/*.cpp"
    "${COMMON_CODE_DIR}/opcua/*.cpp"
    "${COMMON_CODE_DIR}/simulatestewart/stewartplatform.cpp"
    "${COMMON_CODE_DIR}/simulatestewart/mathtools.cpp"
    "${COMMON_CODE_DIR}/globalalignment/ccdclass.cpp"
    "${COMMON_CODE_DIR}/globalalignment/ccd/*.cpp"
    "${COMMON_CODE_DIR}/utilities/*.cpp"
    "${COMMON_CODE_DIR}/alignment/focalplane.cpp"
    "${COMMON_CODE_DIR}/alignment/device.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/objects/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utilities/*.hpp"
    "${COMMON_CODE_DIR}/opcua/*.hpp"
    "${COMMON_CODE_DIR}/simulatestewart/stewartplatform.h"
    "${COMMON_CODE_DIR}/simulatestewart/mathtools.h"
    "${COMMON_CODE_DIR}/globalalignment/ccdclass.h"
    "${COMMON_CODE_DIR}/globalalignment/ccd/*.h"
    "${COMMON_CODE_DIR}/globalalignment/ccd/*.hpp"
    "${COMMON_CODE_DIR}/utilities/*.cpp"
    "${COMMON_CODE_DIR}/alignment/device.hpp"
    "${COMMON_CODE_DIR}/alignment/focalplane.hpp"
    )

if ( SIMMODE )
    message("SIMMODE!")
    add_definitions(-DSIMMODE)
endif ( SIMMODE )

####################################### Include Paths ########################################
IF ( UASTACK_WITH_OPENSSL )
    include_directories(${OPENSSL_INCLUDE_DIR})
ENDIF ( UASTACK_WITH_OPENSSL )
IF ( USE_XMLCONFIG )
    include_directories(${LIBXML2_INCLUDE_DIR})
ENDIF ( USE_XMLCONFIG )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/controllers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/objects)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utilities)
include_directories(${UASDK_BASE_DIR}/include)
include_directories(${COMMON_CODE_DIR}/opcua)
include_directories(${COMMON_CODE_DIR}/simulatestewart)
include_directories(${COMMON_CODE_DIR}/globalalignment)
include_directories(${COMMON_CODE_DIR}/utilities)
include_directories(${COMMON_CODE_DIR}/utilities/spdlog)
include_directories(${COMMON_CODE_DIR}/utilities/spdlog/sinks)
include_directories(${COMMON_CODE_DIR}/utilities/spdlog/fmt)
include_directories(${COMMON_CODE_DIR}/utilities/spdlog/details)
include_directories(${COMMON_CODE_DIR}/utilities/linux)
#include_directories(${COMMON_CODE_DIR}/globalalignment/ccd)
include_directories(${ARAVIS_INCLUDE_DIRS})
include_directories(${ROBAST_INCLUDE_DIRS})
include_directories(${CCFITS_INCLUDE_DIRS})
include_directories(${UASTACK_INCLUDE})
include_directories(${UABASE_INCLUDE})
include_directories(${ROOT_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIRS})
include_directories(${MYSQLCONNECTORCPP_INCLUDE_DIRS})
IF ( UASTACK_WITH_OPENSSL )
    include_directories(${UAPKI_INCLUDE})
ENDIF ()
IF (USE_XMLCONFIG)
    include_directories(${UAXMLPARSER_INCLUDE})
ENDIF (USE_XMLCONFIG)

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
        SET(PLATTFORM_LIBS ${WCELIBCEX_LIBRARIES} ws2 mmtimer)
    ENDIF ( WIN32 AND NOT WINDOWS_CE )
ENDIF (WIN32)

IF (UNIX)
    ADD_DEFINITIONS(-Wall -DSPDLOG_COMPILED_LIB)
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

ADD_EXECUTABLE(${PROJECT_NAME} servermain.cpp ${P2PAS_SRC})
ADD_EXECUTABLE(convertActToPhysical ${COMMON_CODE_DIR}/simulatestewart/ConvertActToPhysicalCoords.cpp ${P2PAS_SRC})
ADD_EXECUTABLE(convertActToTelRF ${COMMON_CODE_DIR}/simulatestewart/ConvertActToTelRFCoords.cpp ${P2PAS_SRC})

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
SET_PROPERTY(TARGET convertActToPhysical PROPERTY CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )

IF ( NOT TARGET uastack )
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${UAMODULE_LIBRARY}
        ${UAMODELS_LIBRARY} ${UACOREMODULE_LIBRARY} ${UACLIENT_LIBRARY} ${UABASE_LIBRARY} ${UASTACK_LIBRARY} ${PLATTFORM_LIBS} ${MYSQLCONNECTORCPP_LIBRARIES} ${ROOT_LIBRARIES}
            ${ARAVIS_LIBRARIES} ${ROBAST_LIBRARIES} ${CCFITS_LIBRARIES} ${NLOPT_LIBRARIES})
ELSE ()
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} uamodelue uamodels codemodule uaclient uabase uastack ${PLATTFORM_LIBS} ${MYSQLCONNECTORCPP_LIBRARIES}
            ${ARAVIS_LIBRARIES} ${ROBAST_LIBRARIES} ${CCFITS_LIBRARIES} ${NLOPT_LIBRARIES} ${ROOT_LIBRARIES})
ENDIF ()

IF ( NOT TARGET uastack )
    TARGET_LINK_LIBRARIES(convertActToPhysical ${UAMODULE_LIBRARY}
            ${UAMODELS_LIBRARY} ${UACOREMODULE_LIBRARY} ${UACLIENT_LIBRARY} ${UABASE_LIBRARY} ${UASTACK_LIBRARY} ${PLATTFORM_LIBS} ${MYSQLCONNECTORCPP_LIBRARIES} ${ROOT_LIBRARIES}
            ${ARAVIS_LIBRARIES} ${ROBAST_LIBRARIES} ${CCFITS_LIBRARIES} ${NLOPT_LIBRARIES})
    TARGET_LINK_LIBRARIES(convertActToTelRF ${UAMODULE_LIBRARY}
            ${UAMODELS_LIBRARY} ${UACOREMODULE_LIBRARY} ${UACLIENT_LIBRARY} ${UABASE_LIBRARY} ${UASTACK_LIBRARY} ${PLATTFORM_LIBS} ${MYSQLCONNECTORCPP_LIBRARIES} ${ROOT_LIBRARIES}
            ${ARAVIS_LIBRARIES} ${ROBAST_LIBRARIES} ${CCFITS_LIBRARIES} ${NLOPT_LIBRARIES})
ELSE ()
    TARGET_LINK_LIBRARIES(convertActToPhysical uamodelue uamodels codemodule uaclient uabase uastack ${PLATTFORM_LIBS} ${MYSQLCONNECTORCPP_LIBRARIES}
            ${ARAVIS_LIBRARIES} ${ROBAST_LIBRARIES} ${CCFITS_LIBRARIES} ${NLOPT_LIBRARIES} ${ROOT_LIBRARIES})
    TARGET_LINK_LIBRARIES(convertActToTelRF uamodelue uamodels codemodule uaclient uabase uastack ${PLATTFORM_LIBS} ${MYSQLCONNECTORCPP_LIBRARIES}
            ${ARAVIS_LIBRARIES} ${ROBAST_LIBRARIES} ${CCFITS_LIBRARIES} ${NLOPT_LIBRARIES} ${ROOT_LIBRARIES})
ENDIF ()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE spdlog::spdlog)
if(Boost_FOUND)
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif()
TARGET_LINK_LIBRARIES(convertActToPhysical PRIVATE spdlog::spdlog)
TARGET_LINK_LIBRARIES(convertActToTelRF PRIVATE spdlog::spdlog)
if(Boost_FOUND)
    target_link_libraries(convertActToPhysical ${Boost_LIBRARIES})
    target_link_libraries(convertActToTelRF ${Boost_LIBRARIES})
endif()

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

# installs also the configuration file sampleconfig.ini
#INSTALL_IF_NEWER("${CMAKE_CURRENT_SOURCE_DIR}/../sampleconfig.ini" "${CMAKE_INSTALL_PREFIX}/bin/sampleconfig.ini")

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
