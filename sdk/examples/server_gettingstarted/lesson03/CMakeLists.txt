project(server_lesson03)
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake ${CMAKE_MODULE_PATH})

find_package(UaOpenSSL)
find_package(UaLibXml2)

include(MessageUtils)
include(InstallPDBFiles)
include(InstallIfNewer)
include(ConfigureCompiler)
include(ConfigureUaStack)
include(ConfigureCppSdk)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

display_project_header()

####################################### Different Configurations ########################################

if (NOT UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)
    message(FATAL_ERROR "It is not allowed to to build the application without UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS")
endif ()

# Build with shared stack
if (BUILD_SHARED_STACK OR BUILD_SHARED_LIBS)
    add_definitions(-D_UA_STACK_USE_DLL)
endif ()

# configure Dll/Lib
if (BUILD_SHARED_LIBS)
    add_definitions(-D_UA_BASE_USE_DLL)
    if (UASDK_WITH_XMLPARSER)
        add_definitions(-D_XML_PARSER_USE_DLL)
    endif ()
    add_definitions(-D_UA_PKI_USE_DLL)
    add_definitions(-D_SERVER_CORE_USE_DLL)
    add_definitions(-D_UA_MODULE_USE_DLL)
    add_definitions(-D_UAMODELS_USE_DLL)
endif ()

if (UASDK_WITH_XMLPARSER)
    set(CONFIG_FILE ServerConfig.xml)
else ()
    set(CONFIG_FILE ServerConfig.ini)
endif ()

file(GLOB LESSON_03_H    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set(LESSON_03_H ${LESSON_03_H} ../../utilities/shutdown.h ../../utilities/opcserver.h ../../simulation_buildingautomation/bacommunicationinterface.h ../../simulation_buildingautomation/bacontrollersimulation.h)

####################################### Include Paths ########################################

include_directories(${UASTACK_INCLUDE})
include_directories(${UABASE_INCLUDE})
include_directories(${UAPKI_INCLUDE})
include_directories(${UAXMLPARSER_INCLUDE})
include_directories(${UACOREMODULE_INCLUDE})
include_directories(${UAMODULE_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../utilities)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../simulation_buildingautomation)

# LINKER PATHS
if (NOT TARGET uastack)
    link_directories(${UA_LIB_DIR})
endif ()

####################################### Create executable ###################################

add_executable(${PROJECT_NAME} ${LESSON_03_H}
    # server application sources
    servermain.cpp
    nmbuildingautomation.cpp
    controllerobject.cpp
    airconditionercontrollerobject.cpp
    furnacecontrollerobject.cpp
    # simulation classes
    ../../simulation_buildingautomation/bacommunicationinterface.cpp
    ../../simulation_buildingautomation/bacontrollersimulation.cpp
    # utility classes
    ../../utilities/opcserver.cpp
    ../../utilities/shutdown.cpp)

####################################### Link Library ########################################

target_link_libraries(${PROJECT_NAME}
                      ${UAMODULE_LIBRARY}
                      ${UACOREMODULE_LIBRARY}
                      ${UAPKI_LIBRARY}
                      ${UABASE_LIBRARY}
                      ${UASTACK_LIBRARY}
                      ${SYSTEM_LIBS})
if (UASDK_WITH_XMLPARSER)
    target_link_libraries(${PROJECT_NAME} ${UAXML_LIBRARY} ${LIBXML2_LIBRARIES})
endif ()
if (UASTACK_WITH_OPENSSL)
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
endif ()

####################################### configure debug postfix #############################

# configure debug postfix
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d" FOLDER Examples/ServerGettingStarted ${WCELINKERFLAGS})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

if (UASTACK_WITH_OPENSSL)
    install_openssl_crypto_library()
    if (UASTACK_WITH_HTTPS)
        install_openssl_ssl_library()
    endif ()
endif ()
if (UASDK_WITH_XMLPARSER)
    install_libxml2_library()
endif ()

# install the configuration file of the server
install_if_newer("${CMAKE_CURRENT_SOURCE_DIR}/../../config/${CONFIG_FILE}" "${CMAKE_INSTALL_PREFIX}/bin/${CONFIG_FILE}")

install_pdb_files()

include(VSSetDebugConfig)
vs_set_debug_config(TARGET ${PROJECT_NAME}
                    INSTALL_DIR_EXECUTABLE
                    WORKING_DIR "${CMAKE_INSTALL_PREFIX}/bin")
