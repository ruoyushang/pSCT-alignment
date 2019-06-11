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
#include <string>
#include <vector>

#include "common/utilities/shutdown.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"
#include "client/utilities/configuration.hpp"
#include "client/utilities/paslogic.hpp"

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
int OpcMain(const char *szAppPath, const std::vector<std::string> &serverlist)
{
    int ret = 0;

#if SUPPORT_XML_PARSER
    UaXmlDocument::initParser(); // initialize the XML Parser
#endif
    ret = UaPlatformLayer::init(); // initialize the UA Stack platform layer
 
    UaStatus status;

    if ( ret == 0 )
    {
        // Set configuration file names
        UaString sServConfigFile(szAppPath);
        UaString sClientConfigFile(szAppPath);

#if SUPPORT_XML_PARSER
        sServConfigFile += "/PasServerConfig.xml";
#else
        sServConfigFile += "/PasServerConfig.ini";
#endif
        sClientConfigFile += "/PasClientConfig.ini";

        //PasLogic *pLogic = nullptr;

        auto pServer = new OpcServer();
        pServer->setServerConfig(sServConfigFile, szAppPath);

        // Load configuration.
        auto pClientConfiguration = std::make_shared<Configuration>();
        std::cout << "Loading Configuration...\n";
        status = pClientConfiguration->loadConnectionConfiguration(sClientConfigFile);
        status = pClientConfiguration->loadDeviceConfiguration(serverlist);
        std::cout << "Finished loading Configuration...\n";

        // Create and initialize communication interface.
        std::shared_ptr<PasCommunicationInterface> pCommIf = std::make_shared<PasCommunicationInterface>();
        pCommIf->setConfiguration(pClientConfiguration);
        // this initializes the communication interface, including all devices that talk
        // directly to it, like Aravis cameras
        status = pCommIf->initialize();
        UA_ASSERT(status.isGood());

        // Create instance of the node manager that invokes client calls
        auto pNodeManagerClient = new PasNodeManager();
        // set its configuration and communication interface
        pNodeManagerClient->setConfiguration(pClientConfiguration);
        pNodeManagerClient->setCommunicationInterface(pCommIf);

        // add the node manager
        pServer->addNodeManager(pNodeManagerClient);

        // Start server object
        ret = pServer->start();
        if ( ret == 0 )
        {
            std::cout << "************************************\n";
            std::cout << "  ---- SERVER/CLIENT STARTED ----\n";
            std::cout << "    Press x to shut down server\n";
            std::cout << "************************************" << std::endl;
            
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

            std::cout << "*************************************\n";
            std::cout << " Shutting down server/client\n";
            std::cout << "*************************************" << std::endl;

            // Stop the utilities thread;
            //pLogic->stop();

            // Stop the server and wait three seconds if clients are connected
            // to allow them to disconnect after they received the shutdown signal
            pServer->stop(3, UaLocalizedText("", "User shutdown"));
        } else {
            std::cout << "*******************************************\n";
            std::cout << "  ---- FAILED TO START SERVER/CLIENT ----\n";
            std::cout << "             Shutting down\n";
            std::cout << "*******************************************" << std::endl;
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
    std::string usage = "<PANEL POSITION NUMBERS> -l <LOGGER LEVEL (optional)>";

    if (argc == 1) {
        std::cout << "Usage: " << argv[0] << " " << usage << std::endl;
        return -1;
    }

    int c;
    std::string panelNumber, configFilePath, endpointUrl;
    std::string logLevel("info");

    while ((c = getopt(argc, argv, "l:")) != -1) {
        switch (c) {
            case 'l':
                logLevel = optarg;
                break;
            case '?':
                if (optopt == 'l')
                    std::cout << "Must provide a logger level with option u (trace, debug, info, warn, error)\n";
            default:
                abort();
        }
    }

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
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("/home/user/logs/p2pasclient_logs", 1048576 * 5, 5,
                                                                            false); // 5 rotating files with max size 5 MB
    file_sink->set_level(spdlog::level::trace); // always save all logging levels

    spdlog::logger logger("p2pasclient", {console_sink, file_sink}); //register logger to both console and file
    logger.set_level(spdlog::level::trace);
    logger.flush_on(spdlog::level::info);

    logger.warn("this should appear in both console and file");
    logger.info("this message should not appear in the console, only in the file");



    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <Positions of panels to connect to>" << std::endl;
        return 1;
    }

#ifndef WIN32
    init_keyboard();
#endif

    int ret = 0;

    // Extract application path
    char* pszAppPath = getAppPath();

    // Collect all passed panel position numbers into a vector
    std::vector<std::string> positionList;
    for (int i = optind; i < argc; i++)
        positionList.emplace_back(argv[i]);

    // Run main method
    ret = OpcMain(pszAppPath, positionList);

    delete[] pszAppPath;

#ifndef WIN32
    close_keyboard();
#endif

    return ret;
}
