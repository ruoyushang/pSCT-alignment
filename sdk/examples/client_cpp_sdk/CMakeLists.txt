project(client_cpp_sdk)
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake ${CMAKE_MODULE_PATH})

find_package(UaOpenSSL)
find_package(UaLibXml2)

include(MessageUtils)
include(InstallPDBFiles)
include(InstallIfNewer)
include(ConfigureCompiler)
include(ConfigureUaStack)
include(ConfigureCppSdk)

SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

display_project_header()

####################################### Different Configurations ########################################

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
    if (UASTACK_WITH_OPENSSL)
        add_definitions(-D_UA_PKI_USE_DLL)
    endif ()
    add_definitions(-D_UA_CLIENT_USE_DLL)
endif ()

if (NOT UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS)
    message(FATAL_ERROR "It is not allowed to to build the application without UASTACK_SUPPORT_ENC_OBJ_EXTENSIONS")
endif ()

FILE(GLOB CLIENT_H    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
SET(CLIENT_H ${CLIENT_H} ../utilities/shutdown.h)

####################################### Include Paths ########################################

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${UASTACK_INCLUDE})
include_directories(${UABASE_INCLUDE})
include_directories(${UAPKI_INCLUDE})
include_directories(${UACLIENT_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../utilities)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../utilities/${UASDK_PLATFORM})

# LINKER PATHS
if (NOT TARGET uastack)
    link_directories(${UA_LIB_DIR})
endif ()

####################################### Different Platforms ########################################

IF (WIN32)
    SET(OS_SRC version.rc)
ENDIF (WIN32)
# create executable
add_executable(${PROJECT_NAME} ${CLIENT_H} ${OS_SRC}
    client_cpp_sdk.cpp
    clientconfig.cpp
    demo_datatypes.cpp
    demo_vector.cpp
    demo_uniontest.cpp
    ../utilities/shutdown.cpp
)

####################################### Link Library ########################################

target_link_libraries(${PROJECT_NAME} ${UACLIENT_LIBRARY})
if (UASTACK_WITH_OPENSSL)
    target_link_libraries(${PROJECT_NAME} ${UAPKI_LIBRARY} ${OPENSSL_LIBRARIES})
ENDIF ()
target_link_libraries(${PROJECT_NAME}
                      ${UABASE_LIBRARY}
                      ${UASTACK_LIBRARY}
                      ${SYSTEM_LIBS})
if (UASDK_WITH_XMLPARSER)
    target_link_libraries(${PROJECT_NAME} ${UAXML_LIBRARY} ${LIBXML2_LIBRARIES})
endif ()

####################################### configure debug postfix #############################

# configure debug postfix
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d" FOLDER Examples ${WCELINKERFLAGS})

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

# install the configuration file of the client
install_if_newer("${CMAKE_CURRENT_SOURCE_DIR}/../config/ClientConfig.ini" "${CMAKE_INSTALL_PREFIX}/bin/ClientConfig.ini")

install_pdb_files()

include(VSSetDebugConfig)
vs_set_debug_config(TARGET ${PROJECT_NAME}
                    INSTALL_DIR_EXECUTABLE
                    WORKING_DIR "${CMAKE_INSTALL_PREFIX}/bin")
