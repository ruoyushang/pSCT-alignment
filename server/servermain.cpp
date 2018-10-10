/******************************************************************************
** servermain.cpp
**
** Copyright (c) 2006-2014 Unified Automation GmbH All rights reserved.
**
** Software License Agreement ("SLA") Version 2.3
**
** Unless explicitly acquired and licensed from Licensor under another
** license, the contents of this file are subject to the Software License
** Agreement ("SLA") Version 2.3, or subsequent versions
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
** http://unifiedautomation.com/License/SLA/2.3/
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
#endif
#include "shutdown.h"
#include "pasnodemanager.h"
#include "pascommunicationinterface.h"
#include <sys/statvfs.h>

// check the system is reasonable -- enough disk space, enabled network connection
int CheckSystem(double *size)
{
    // check disk space
    struct statvfs sysBuf;
    if (statvfs("/", &sysBuf) == 0) 
        *size = ((double)sysBuf.f_bsize*sysBuf.f_bfree)/(1024*1024*1024);
    else return 1;

    return 0;
}


int OpcServerMain(const char* szAppPath, const char* serverUrl)
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
        // This code can be integrated into a startup
        // sequence of the application where the
        // OPC server should be integrated
        //-------------------------------------------
        // Create and initialize server object
        OpcServer* pServer = new OpcServer;
        pServer->setServerConfig(sConfigFileName, szAppPath);

        // Create and initialize communication interface
        // this is a large object -- allocate on the heap
        PasCommunicationInterface *pCommIf = new PasCommunicationInterface();
        // this initializes the communication interface, including all devices that talk
        // directly to it, like Aravis cameras
        //serverUrl = "10.0.1.20"; //Ruo
        pCommIf->setServerUrl(serverUrl);
        UaStatus ret = pCommIf->Initialize();
        UA_ASSERT(ret.isGood());

        // Add NodeManager for the server specific nodes
        PasNodeManager *pNodeManager = new PasNodeManager();
        // set its communication interface
        pNodeManager->setCommunicationInterface(pCommIf);
        pServer->addNodeManager(pNodeManager);

        // Start server object
        ret = pServer->start();
        if ( ret != 0 )
        {
            delete pServer;
            pServer = 0;
        }
        //-------------------------------------------

        if ( ret == 0 )
        {
            printf("***************************************************\n");
            printf(" Press %s to shut down server\n", SHUTDOWN_SEQUENCE);
            printf("***************************************************\n");
            fflush(stdout);
            // Wait for user command to terminate the server thread.
            
            while ( ShutDownFlag() == 0 )
            {
                UaThread::msleep(1000);
            }
            printf("***************************************************\n");
            printf(" Shutting down server\n");
            printf("***************************************************\n");


            //- Stop OPC server -------------------------
            // This code can be integrated into a shutdown
            // sequence of the application where the
            // OPC server should be integrated
            //-------------------------------------------
            // Stop the server and wait three seconds if clients are connected
            // to allow them to disconnect after they received the shutdown signal
            pServer->stop(3, UaLocalizedText("", "User shutdown"));
            delete pServer;
            pServer = nullptr;
            //-------------------------------------------
        }
        // clean up
        if (pCommIf) {
            delete pCommIf;
            pCommIf = nullptr;
        }
    }

    //- Clean up the environment --------------
    // Clean up the UA Stack platform layer
    UaPlatformLayer::cleanup();
#if SUPPORT_XML_PARSER
    // Clean up the XML Parser
    UaXmlDocument::cleanupParser();
#endif
    //-------------------------------------------

    return ret;
}

#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int argc, char* argv[])
#endif
{
    if (argc != 2) {
        printf("Usage: %s <CBC IP ADDRESS>\n", argv[0]);
        return 1;
    }

    int ret = 0;

    // check and print system info
    double sysFree;
    printf("********************SYSTEM INFO********************\n");
    if (!CheckSystem(&sysFree))
        printf(" Free disk space: %4.2fGB\n",sysFree);
    else {
        printf(" Failed to stat \"/\": terminating\n");
        return -1;
    } 
    printf("***************************************************\n"); 


    RegisterSignalHandler();

    // Extract application path
    char* pszAppPath = getAppPath();

    //-------------------------------------------
    // Call the OPC server main method
    ret = OpcServerMain(pszAppPath, argv[1]);
    //-------------------------------------------

    if ( pszAppPath ) delete [] pszAppPath;

    return ret;
}
