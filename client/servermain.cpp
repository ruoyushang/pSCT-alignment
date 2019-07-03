/******************************************************************************
** servermain.cpp
**
** Description: Main entry for the application
**
******************************************************************************/
#include "common/utilities/opcserver.hpp"
#include "common/utilities/shutdown.hpp"
#include "uaplatformlayer.h"
#include "uathread.h"
#if SUPPORT_XML_PARSER
  #include "xmldocument.h"
#endif

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common/utilities/shutdown.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"
#include "client/utilities/configuration.hpp"
#include "client/utilities/paslogic.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/sinks/basic_file_sink.h"
#include "common/utilities/spdlog/sinks/stdout_color_sinks.h"
#include "common/utilities/spdlog/sinks/rotating_file_sink.h"
#include "common/utilities/spdlog/logger.h"

#define CLIENT_CPP_SDK_ACTIVATE_MEMCHECK 0

#ifdef WIN32
#if CLIENT_CPP_SDK_ACTIVATE_MEMCHECK
    #include <crtdbg.h>
#endif /* CLIENT_CPP_SDK_ACTIVATE_MEMCHECK */
    #define kbhit _kbhit
    #define getch _getch
#else
#if defined (VXWORKS)
    void Sleep(int ms)
    {
        OpcUa_Thread_Sleep(ms);
    }
#else
    #include <unistd.h>
    void Sleep(int ms)
    {
        usleep(1000 * ms);
    }
#endif
    #include "kbhit.c"
    #include <signal.h>
    #include <limits.h>
    #define getch readch
#endif

/*============================================================================
 * WaitForKeypress -- helper
 *===========================================================================*/
bool WaitForKeypress(int& action)
{
    action = -1;

    std::map<int, int> mapActions;
    // map keys 0-9 to values 0-9
    for (int i = 0; i <= 9; i++)
    {
        mapActions['0' + i]         = i;
    }
    // map keys a-k to values 10-20
    for (int i = 0; i <= 'k'-'a'; i++)
    {
        mapActions['a' + i] = i + 10;
        mapActions['A' + i] = i + 10;
    }

#ifndef _WIN32_WCE
    if (kbhit())
    {
        int iChar = getch();
        if (iChar == 'x')
        {
            return true;
        }
        else if (mapActions.find(iChar) != mapActions.end())
        {
            action = mapActions[iChar];
        }
    }
#else
    if (GetAsyncKeyState('X'))
    {
        return true;
    }
    else
    {
        std::map<int, int>::const_iterator it = mapActions.begin();
        while (it != mapActions.end())
        {
            if (GetAsyncKeyState(it->first))
            {
                action = it->second;
                break;
            }
            ++it;
        }
    }
#endif
    return false;
}

//===================================================================
int OpcMain(const char *szAppPath, const std::vector<std::string> &serverList, const std::string &configFilePath,
            const std::string &mode)
{
    int ret = 0;

#if SUPPORT_XML_PARSER
    UaXmlDocument::initParser(); // initialize the XML Parser
#endif
    ret = UaPlatformLayer::init(); // initialize the UA Stack platform layer
 
    UaStatus status;

    std::string sClientConfigFilename = std::string(szAppPath) + "/PasClientConfig.xml";

    if ( ret == 0 )
    {
        // Set configuration file name
        UaString sClientConfigFile(sClientConfigFilename.c_str());
        UaString sServerConfigFile(configFilePath.c_str());

        //PasLogic *pLogic = nullptr;

        auto pServer = new OpcServer();
        pServer->setServerConfig(sServerConfigFile, szAppPath);
        //pServer->setCallback(pServer->pCallback);

        // Load configuration.
        auto pClientConfiguration = std::make_shared<Configuration>(mode);
        spdlog::info("Loading Configuration...");
        status = pClientConfiguration->loadConnectionConfiguration(sClientConfigFile);
        if (mode == "subclient") {
            status = pClientConfiguration->loadDeviceConfiguration(serverList);
        } else if (mode == "client") {
            status = pClientConfiguration->loadDeviceConfiguration();
            pClientConfiguration->loadSubclientConfiguration(serverList);
        }

        spdlog::info("Finished loading Configuration...");

        // Create and initialize communication interface.
        std::shared_ptr<PasCommunicationInterface> pCommIf = std::make_shared<PasCommunicationInterface>();
        pCommIf->setConfiguration(pClientConfiguration);
        // this initializes the CCD controllers (which only exist in the top-level client)
        if (mode == "client") {
            status = pCommIf->initializeCCDs();
            UA_ASSERT(status.isGood());
        }

        // Create instance of the node manager that invokes client calls
        auto pNodeManagerClient = new PasNodeManager(pClientConfiguration, mode);
        pNodeManagerClient->setCommunicationInterface(pCommIf);

        // add the node manager
        spdlog::info("Adding node manager...");
        pServer->addNodeManager(pNodeManagerClient);

        // Start server object
        spdlog::info("Starting server...");
        ret = pServer->start();
        if ( ret == 0 )
        {
            spdlog::info("************************************\n"
                         "  ---- SERVER/CLIENT STARTED ----\n"
                         "    Press x to shut down server\n"
                         "************************************");
            
            //pLogic = new PasLogic(pCommIf);
            //pLogic->start();

            // Wait for user command to terminate the server thread.
            int action;
            while (!WaitForKeypress(action))
            {
                if ( action == -1 )
                {
                    UaThread::msleep(100);
                    continue;
                }
                if (action == 0) {
                    printf("!! Panicking !!\n");
                    ret = pNodeManagerClient->Panic();
                } else {
                    continue;
                }
            }

            spdlog::info("*************************************\n"
                         " Shutting down server/client\n"
                         "*************************************");

            // Stop the utilities thread;
            //pLogic->stop();

            // Stop the server and wait three seconds if clients are connected
            // to allow them to disconnect after they received the shutdown signal
            pServer->stop(3, UaLocalizedText("", "User shutdown"));
        } else {
            spdlog::error("*******************************************\n"
                          "  ---- FAILED TO START SERVER/CLIENT ----\n"
                          "             Shutting down\n"
                          "*******************************************");
        }

        delete pServer;
    }

    UaPlatformLayer::cleanup();
#if SUPPORT_XML_PARSER
    UaXmlDocument::cleanupParser(); // Clean up the XML Parser
#endif

    return ret;
}

#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int argc, char* argv[])
#endif
{
    std::string usage = "<PANEL POSITION NUMBERS OR SUBCLIENT NAMES> -c <CONFIGURATION FILE PATH (optional)> -l <LOGGER LEVEL (optional)> -m <MODE, 'subclient' or 'client' (optional)>";

    if (argc == 1) {
        std::cout << "Usage: " << argv[0] << " " << usage << std::endl;
        return -1;
    }

    int c;
    std::string panelNumber, configFilePath;
    std::string mode("subclient");
    std::string logLevel("info");

    while ((c = getopt(argc, argv, "l:c:m:")) != -1) {
        switch (c) {
            case 'c':
                configFilePath = optarg;
                break;
            case 'l':
                logLevel = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case '?':
                if (optopt == 'l')
                    std::cout << "Must provide a logger level with option u (trace, debug, info, warn, error)\n";
            default:
                abort();
        }
    }

    std::cout << "Running in " << mode << " mode...\n";

    // Setup logging
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(); // log to console
    if (logLevel == "trace") {
        console_sink->set_level(spdlog::level::trace);
    } else if (logLevel == "debug") {
        console_sink->set_level(spdlog::level::debug);
    } else if (logLevel == "info") {
        console_sink->set_level(spdlog::level::info);
    } else if (logLevel == "warn") {
        console_sink->set_level(spdlog::level::warn);
    } else if (logLevel == "error") {
        console_sink->set_level(spdlog::level::err);
    } else if (logLevel == "critical") {
        console_sink->set_level(spdlog::level::critical);
    } else {
        std::cout << "Invalid logger level selected: " << logLevel
                  << ". Valid choices are 'trace', 'debug', 'info', 'warn', 'error', 'critical'.";
        return -1;
    }

    // Note that log directory must have been created beforehand
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(std::string(getenv("HOME")) + std::string("/logs/p2pasclient_logs"), 1048576 * 5, 5,
                                                                       false); // 5 rotating files with max size 5 MB
    file_sink->set_level(spdlog::level::trace); // always save all logging levels
    file_sink->set_pattern("[%c] [%n] [%l] [%s:%!:%#] %v");

    std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
    auto logger = std::make_shared<spdlog::logger>("p2pasclient", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::info);
    spdlog::set_default_logger(logger);

#ifndef WIN32
    init_keyboard();
#endif

    int ret = 0;

    // NOTE: This method returns a pointer to heap-allocated memory, must be freed manually
    char *pszAppPath = getAppPath(); // Extract application path

#if SUPPORT_XML_PARSER
    std::string sServConfigFilename = "/PasServerConfig.xml";
#else
    std::string sServConfigFilename = "/PasServerConfig.ini";
#endif

    if (configFilePath.empty()) {
        configFilePath = std::string(pszAppPath) + sServConfigFilename;
        logger->warn("No config file path passed, using default file {}.", configFilePath);
    }

    // Collect all passed servers (panel position numbers or subclient names) into a vector
    std::vector<std::string> serverList;
    for (int i = optind; i < argc; i++)
        serverList.emplace_back(argv[i]);

    // Run main method
    ret = OpcMain(pszAppPath, serverList, configFilePath, mode);

    delete[] pszAppPath;

#ifndef WIN32
    close_keyboard();
#endif

    return ret;
}
