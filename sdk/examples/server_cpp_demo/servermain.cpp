/******************************************************************************
** servermain.cpp
**
** Copyright (c) 2006-2015 Unified Automation GmbH All rights reserved.
**
** Software License Agreement ("SLA") Version 2.5
**
** Unless explicitly acquired and licensed from Licensor under another
** license, the contents of this file are subject to the Software License
** Agreement ("SLA") Version 2.5, or subsequent versions
** as allowed by the SLA, and You may not copy or use this file in either
** source code or executable form, except in compliance with the terms and
** conditions of the SLA.
**
** All software distributed under the SLA is provided strictly on an
** "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
** AND LICENSOR HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT
** LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
** PURPOSE, QUIET ENJOYMENT, OR NON-INFRINGEMENT. See the SLA for specific
** language governing rights and limitations under the SLA.
**
** The complete license agreement can be found here:
** http://unifiedautomation.com/License/SLA/2.5/
**
** Project: C++ OPC Server SDK sample code
**
** Description: Main entry for the application
**
******************************************************************************/
#include "opcserver.h"
#include "uaplatformlayer.h"
#include "uathread.h"
#if SUPPORT_XML_PARSER
  #include "xmldocument.h"
  #include "mynodemanagernodesetxmlcreator.h"
  #include "serverconfigxml.h"
#endif
#include "shutdown.h"
#include "nmbuildingautomation.h"
#include "demo_nodemanagerdemo.h"
#include "access_nodemanageraccess.h"
#include "myservercallback.h"
#include "instancefactory.h"
#include "srvtrace.h"
#include "version_coremodule.h"
#include "uaunistring.h"
#include "uaunistringlist.h"
#include "serverconfigini.h"

int OpcServerMain(const char* szAppPath, const char* szConfigFileName)
{
    int ret = 0;

    //- Initialize the environment --------------
#if SUPPORT_XML_PARSER
    // Initialize the XML Parser
    UaXmlDocument::initParser();
#endif
    // Initialize the UA Stack platform layer
    ret = UaPlatformLayer::init();
    //-------------------------------------------

    if ( ret == 0 )
    {
        // Create configuration file name
        UaString sConfigFileName;

        if (szConfigFileName == 0)
        {
            sConfigFileName = UaString(szAppPath);

#if SUPPORT_XML_PARSER
            sConfigFileName += "/ServerConfig.xml";
#else
            sConfigFileName += "/ServerConfig.ini";
#endif
        }
        else
        {
            sConfigFileName = UaString(szConfigFileName);
        }

        //- Start up OPC server ---------------------
        // This code can be integrated into a start up
        // sequence of the application where the
        // OPC server should be integrated
        //-------------------------------------------
        // Create and initialize server object
        OpcServer* pServer = new OpcServer;
        pServer->setServerConfig(sConfigFileName, szAppPath);

        // Set the callback for user authentication
        MyServerCallback serverCallback;
        pServer->setCallback(&serverCallback);

        // Add NodeManager for Unified Automation Demo namespace
        Demo::NodeManagerDemo *pDemoNodeManager = new Demo::NodeManagerDemo(OpcUa_True, szAppPath);
        pServer->addNodeManager(pDemoNodeManager);

        // Add NodeManager for the building automation specific nodes
        NmBuildingAutomation *pMyNodeManager = new NmBuildingAutomation();
        pServer->addNodeManager(pMyNodeManager);

        Access::NodeManagerAccess *pAccessNodeManager = new Access::NodeManagerAccess(OpcUa_True);
        pServer->addNodeManager(pAccessNodeManager);

#if SUPPORT_XML_PARSER
        // XML UANodeSet file to load
        UaString sNodesetFile(UaString("%1/uanodesetimport.xml").arg(szAppPath));
        // We create our own BaseNode factory to create the user data from XML
        MyBaseNodeFactory* pBaseNodeFactory = new MyBaseNodeFactory;
        // We create our own NodeManager creator to instantiate our own NodeManager
        MyNodeManagerNodeSetXmlCreator* pNodeManagerCreator = new MyNodeManagerNodeSetXmlCreator;
        UaNodeSetXmlParserUaNode* pXmlParser = new UaNodeSetXmlParserUaNode(sNodesetFile, pNodeManagerCreator, pBaseNodeFactory, NULL);
        // Add UANodeSet XML parser as module
        pServer->addModule(pXmlParser);
#endif

        // Start server object
        ret = pServer->start();
        if ( ret == 0 )
        {
            printf("***************************************************\n");
            printf(" Press %s to shutdown server\n", SHUTDOWN_SEQUENCE);
            printf("***************************************************\n");
            // Wait for user command to terminate the server thread.
            while ( ShutDownFlag() == 0 )
            {
                UaThread::msleep(1000);
            }
            printf("***************************************************\n");
            printf(" Shutting down server\n");
            printf("***************************************************\n");


            //- Stop OPC server -------------------------
            // This code can be integrated into a shut down
            // sequence of the application where the
            // OPC server should be integrated
            //-------------------------------------------
            // Stop the server and wait three seconds if clients are connected
            // to allow them to disconnect after they received the shutdown signal
            pServer->stop(3, UaLocalizedText("", "User shut down"));
            //-------------------------------------------
        }
        else
        {
            printf("***************************************************\n");
            printf(" Server start failed\n");
            printf("***************************************************\n");
            // Print errors
            std::list<UaString> errorList = SrvT::getPreFileTraces();
            if ( errorList.size() > 0 )
            {
                std::list<UaString>::iterator it;
                for ( it = errorList.begin(); it != errorList.end(); it++ )
                {
                    printf("%s", it->toUtf8());
                }
            }
            else
            {
                printf(" Check log file for details\n");
            }

            printf("***************************************************\n");
            printf(" Press %s to close window\n", SHUTDOWN_SEQUENCE);
            // Wait for user command to terminate the window.
            while ( ShutDownFlag() == 0 )
            {
                UaThread::msleep(1000);
            }
        }

        delete pServer;
        pServer = NULL;
    }

    //- Cleanup the environment --------------
    // Cleanup the UA Stack platform layer
    UaPlatformLayer::cleanup();
#if SUPPORT_XML_PARSER
    // Cleanup the XML Parser
    UaXmlDocument::cleanupParser();
#endif
    //-------------------------------------------

    return ret;
}

void printHelp();
void printVersion();

#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int argc, char* argv[])
#endif
{
#if defined(WIN32) && defined(_MSC_VER) && _MSC_VER >= 1400 && !defined(_WIN32_WCE)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(590);
#endif // defined(WIN32) && _MSC_VER >= 1400

    int ret = 0;

    RegisterSignalHandler();

    // Extract application path
    char* pszAppPath = getAppPath();

    char* pszConfigFilePath = NULL;

#ifndef _WIN32_WCE
    // check for command line options
    if ( argc > 1 )
    {
        for ( int i=1; i<argc; i++)
        {
            UaString sArg(argv[i]);
            // '-c configFile' Path and name of configuration file
            if ( sArg == "-c" && i < argc )
            {
                i++;
                // copy filepath
                size_t len = UA_strnlen(argv[i], 1024) + 1;
                if (len > 0)
                {
                    pszConfigFilePath = new char [len+1];
                    pszConfigFilePath[len] = 0;
                    strncpy(pszConfigFilePath, argv[2], len);
                }
            }
            // '?' '-h' Show usage message and exit
            else if ( ((sArg == "?") || (sArg == "-h") || (sArg == "--help")) && i < argc )
            {
                printHelp();
                return 0;
            }
            // '-v' Show version information and exit
            else if ( ((sArg == "-v") || (sArg == "--version")) && i < argc )
            {
                printVersion();
                return 0;
            }
            else
            {
                printf("\n");
                printf("Invalid console parameter %s\n", sArg.toUtf8());
                printHelp();
                return 0;
            }
        }
    }
#endif // #ifndef _WIN32_WCE

    //-------------------------------------------
    // Call the OPC server main method
    ret = OpcServerMain(pszAppPath, pszConfigFilePath);
    //-------------------------------------------

    if ( pszAppPath ) delete [] pszAppPath;

    if ( pszConfigFilePath ) delete pszConfigFilePath;

    return ret;
}

void printHelp()
{
    printf("\n");
    printf("**************************************************************************\n");
    printf("* The C++ SDK demo server can be started with the following parameters:  *\n");
    printf("*  ? -h                Show usage message and exit                       *\n");
    printf("*  -v                  Show version information and exit                 *\n");
    printf("*  -c configFile       Path and name of configuration file               *\n");
    printf("**************************************************************************\n");
    printf(" Press %s to close window\n", SHUTDOWN_SEQUENCE);
    printf("\n");
    // Wait for user command to terminate the window.
    while ( ShutDownFlag() == 0 )
    {
        UaThread::msleep(1000);
    }
}

void printVersion()
{
    unsigned int i;
    UaUniString usVersionInfo;
    UaUniStringList usVersionInfoList;

    printf("\n");
    printf("**************************************************************************\n");

    printf("Version information UA Server SDK:\n");
    usVersionInfo = VersionInfoCoreModule::getCoreModuleVersionInfo().toUtf16();
    usVersionInfoList = usVersionInfo.split(UaUniString("\\"));
    for ( i=0; i<usVersionInfoList.size(); i++ )
    {
        UaString sTemp(usVersionInfoList[i].toUtf16());
        printf("    %s\n", sTemp.toUtf8());
    }
    usVersionInfoList.clear();

    printf("Version information UA Stack:\n");
    usVersionInfo = VersionInfoCoreModule::getUaStackVersionInfo().toUtf16();
    usVersionInfoList = usVersionInfo.split(UaUniString("\\"));
    for ( i=0; i<usVersionInfoList.size(); i++ )
    {
        UaString sTemp(usVersionInfoList[i].toUtf16());
        printf("    %s\n", sTemp.toUtf8());
    }
    usVersionInfoList.clear();

    printf("Static configuration information UA Stack:\n");
    usVersionInfo = VersionInfoCoreModule::getUaStackStaticConfigInfo().toUtf16();
    usVersionInfoList = usVersionInfo.split(UaUniString("\\"));
    for ( i=0; i<usVersionInfoList.size(); i++ )
    {
        UaString sTemp(usVersionInfoList[i].toUtf16());
        printf("    %s\n", sTemp.toUtf8());
    }
    usVersionInfoList.clear();

    printf("Version information UA Stack Platform Layer:\n");
    usVersionInfo = VersionInfoCoreModule::getUaStackPlatformLayerVersionInfo().toUtf16();
    usVersionInfoList = usVersionInfo.split(UaUniString("\\"));
    for ( i=0; i<usVersionInfoList.size(); i++ )
    {
        UaString sTemp(usVersionInfoList[i].toUtf16());
        printf("    %s\n", sTemp.toUtf8());
    }
    usVersionInfoList.clear();

    printf("**************************************************************************\n");
    printf(" Press %s to close window\n", SHUTDOWN_SEQUENCE);
    printf("\n");
    // Wait for user command to terminate the window.
    while ( ShutDownFlag() == 0 )
    {
        UaThread::msleep(1000);
    }
}
