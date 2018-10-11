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
#include <stdio.h>
#include <string.h>

#include "opcserver.h"
#include "shutdown.h"
#include "uaplatformlayer.h"
#include "uathread.h"
#if SUPPORT_XML_PARSER
  #include "xmldocument.h"
#endif
#include "uadatavariablecache.h"
#include "uadatavariablecache.h"
#include "opcuadi_nodemanagerdevices.h"
#include "opcuaplc_nodemanagerplcopen.h"
#include "nodemanagerboilerdemo.h"

int OpcServerMain(const char* szAppPath)
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
        UaString sConfigFileName(szAppPath);

#if SUPPORT_XML_PARSER
        sConfigFileName += "/ServerConfig.xml";
#else
        sConfigFileName += "/ServerConfig.ini";
#endif

        //- Start up OPC server ---------------------
        // This code can be integrated into a start up
        // sequence of the application where the
        // OPC server should be integrated
        //-------------------------------------------
        // Create and initialize server object
        OpcServer* pServer = new OpcServer;
        pServer->setServerConfig(sConfigFileName, szAppPath);

        OpcUaDi::NodeManagerDevices* pNmDI = new OpcUaDi::NodeManagerDevices;
        pServer->addNodeManager(pNmDI);

        OpcUaPlc::NodeManagerPLCopen* pNmPlc = new OpcUaPlc::NodeManagerPLCopen;
        pServer->addNodeManager(pNmPlc);

        NodeManagerBoilerDemo* pNM = new NodeManagerBoilerDemo;
        pServer->addNodeManager(pNM);

        // Start server object
        ret = pServer->start();
        if ( ret != 0 )
        {
            delete pServer;
            return ret;
        }
        //-------------------------------------------

        printf("***************************************************\n");
        printf(" Press %s to shutdown server\n", SHUTDOWN_SEQUENCE);
        printf("***************************************************\n");
        // Wait for user command to terminate the server thread.
        // Simulate data
        UaVariant newValue;
        UaDataValue dataValue;
        while(ShutDownFlag() == 0)
        {
            UaThread::msleep(100);
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
        delete pServer;
        pServer = NULL;
        //-------------------------------------------
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

#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int, char*[])
#endif
{
    int ret = 0;

    RegisterSignalHandler();

    // Extract application path
    char* pszAppPath = getAppPath();

    //-------------------------------------------
    // Call the OPC server main method
    ret = OpcServerMain(pszAppPath);
    //-------------------------------------------

    if ( pszAppPath ) delete [] pszAppPath;

    return ret;
}


