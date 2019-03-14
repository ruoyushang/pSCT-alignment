/**
 * @file servermain.cpp
 * @brief Entry point for passerver.
 */

#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>
#include <sys/statvfs.h>

#include "uabase/uadatetime.h"
#include "uabase/uaplatformlayer.h"
#include "uabase/uathread.h"

#include "uaserver/serverconfig.h"
#include "uaserver/serverconfigdata.h"
#include "uaserver/serverconfigxml.h"
#include "uaserver/uaserverapplication.h"

#if SUPPORT_XML_PARSER
  #include "xmlparser/xmldocument.h"
#endif

#include "common/utilities/opcserver.h"
#include "common/utilities/shutdown.h"

#include "server/pascommunicationinterface.hpp"
#include "server/pasnodemanager.hpp"


/// @brief Check the available disk space on the system.
/// @param size Reference to a double to store the retrieved file system size (in GB).
/// @return Returns 0 on success and -1 on failure.
int CheckSystem(double &size)
{
    struct statvfs sysBuf;
    if (statvfs("/", &sysBuf) == 0)
        size = ((double)sysBuf.f_bsize*sysBuf.f_bfree)/(1024*1024*1024);
    else return -1;

    return 0;
}

/// @brief Initialize, configure, and start the OPC UA server.
/// @param szAppPath String application filepath.
/// @param configFilePath String path to configuration file for server.
/// @param serverIP Url for the server.
/// @return Returns 0 on success and -1 on failure.
int OpcServerMain(std::string szAppPath, std::string configFilePath, std::string serverIP)
{
    int ret;

#if SUPPORT_XML_PARSER
    UaXmlDocument::initParser(); // Initialize the XML Parser
#endif

    ret = UaPlatformLayer::init(); // Initialize the UA Stack platform layer

    if ( ret == 0 )
    {
        UaString sAppPath(szAppPath.c_str()); // Set server app path
        UaString sConfigFileName(configFilePath.c_str()); // Set config filename

        std::unique_ptr<OpcServer> pServer = std::unique_ptr<OpcServer>(new OpcServer()); // Initialize and configure server object
        pServer->setServerConfig(sConfigFileName, sAppPath);

        /**
        Get config and overwrite endpoint URL
        UaEndpointArray endpoints;
        OpcUa_UInt32 rejected_certificates_count = 1;
        ServerConfig *pConfig = pServer->getServerConfig();
        pConfig->loadConfiguration();
        pConfig->getEndpointConfiguration(sAppPath, rejected_certificates_count, endpoints);
        printf("Number of endpoints: %d\n", endpoints.length());
        endpoints[0]->setEndpointUrl(sEndpointUrl, false);
        */

        std::unique_ptr<PasCommunicationInterface> pCommIf = std::unique_ptr<PasCommunicationInterface>(new PasCommunicationInterface()); // Initialize communication interface
        pCommIf->setserverIP(serverIP.c_str());
        UaStatus ret = pCommIf->Initialize();
        UA_ASSERT(ret.isGood());

        std::unique_ptr<PasNodeManager> pNodeManager = std::unique_ptr<PasNodeManager>(new PasNodeManager()); // Create Node Manager for the server
        pNodeManager->setCommunicationInterface(pCommIf); // set its communication interface

        pServer->addNodeManager(pNodeManager.release()); // Add node manager to server

        ret = pServer->start();
        if ( ret == 0 )
        {
            std::cout << "***************************************************\n";
            std::cout << " Press " << SHUTDOWN_SEQUENCE << " to shut down server\n";
            std::cout << "***************************************************\n";

            // Wait for user command to terminate the server.
            while ( ShutDownFlag() == 0 )
            {
                UaThread::msleep(1000);
            }

            std::cout << "***************************************************\n";
            std::cout << " Shutting down server\n";
            std::cout << "***************************************************\n";

            // Stop the server and wait three seconds if clients are connected
            // to allow them to disconnect after they received the shutdown signal
            pServer->stop(3, UaLocalizedText("", "User shutdown"));
        }
    }

    UaPlatformLayer::cleanup(); // Clean up the UA Stack platform layer
#if SUPPORT_XML_PARSER
    UaXmlDocument::cleanupParser(); // Clean up the XML Parser
#endif

    return ret;
}

/// @brief Start the server.
/// @param argc Number of args.
/// @param argv Array of args.
/// @return Returns 0 on success and -1 on failure.
#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int argc, char* argv[])
#endif
{
    if (argc == 1) {
        printf("Usage: %s <CBC IP ADDRESS> -c <CONFIG FILE PATH>\n", argv[0]);
        return -1;
    }

    int c;
    std::string cbcIPAddress = "", configFilePath = "";

    while ((c = getopt (argc, argv, "c:")) != -1) {
        switch(c)
        {
            case 'c':
                configFilePath = optarg;
                break;
            case '?':
                if (optopt == 'c')
                    std::cout << "Must provide a config file path with option c\n";
         }
    }
    cbcIPAddress = argv[optind];

    // NOTE: It is unclear whether this method allocates on the heap
    // It is assumed so, which is why we later call delete manually.
    char* pszAppPath = getAppPath(); // Extract application path

    #if SUPPORT_XML_PARSER
        std::string sConfigFileName = "/ServerConfig.xml";
    #else
        std::string sConfigFileName = "/ServerConfig.ini";
    #endif

    if(configFilePath.empty()) {
        configFilePath = std::string(pszAppPath) + sConfigFileName;
    }


    double sysFree;
    std::cout << "********************SYSTEM INFO********************\n";
    if (!CheckSystem(sysFree)) // check available disk space
        std::cout << " Free disk space: " << sysFree << "GB\n";
    else {
        std::cout << " Failed to stat \"/\": terminating\n";
        return -1;
    }
    std::cout << "***************************************************\n";
    RegisterSignalHandler();

    int ret = OpcServerMain(pszAppPath, configFilePath, cbcIPAddress);

    if ( pszAppPath ) delete [] pszAppPath; // release pszAppPath memory manually

    return ret;
}
