/******************************************************************************
** servermain.cpp
**
** Description: Main entry for the application
**
******************************************************************************/
#include "opcserver.h"
#include "shutdown.h"
#include "uaplatformlayer.h"
#include "uathread.h"
#if SUPPORT_XML_PARSER
  #include "xmldocument.h"
#endif
#include "pasnodemanager.h"
#include "configuration.h"
#include "pascommunicationinterface.h"
#include "paslogic.h"
#include "shutdown.h"

#include <vector>
#include <string>
#include <iostream>

#define CLIENT_CPP_SDK_ACTIVATE_TRACE    0
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
int OpcMain(const char* szAppPath, std::vector<std::string> serverlist)
{
    int ret = 0;

#if SUPPORT_XML_PARSER
    UaXmlDocument::initParser(); // Initialize the XML Parser
#endif
    ret = UaPlatformLayer::init(); // Initialize the UA Stack platform layer
 
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
               
        OpcServer* pServer = new OpcServer();
        pServer->setServerConfig(sServConfigFile, szAppPath);

        // Load configuration.
        Configuration *pClientConfiguration = new Configuration();
        std::cout << "  ---- Loading Configuration ----\n";
        status = pClientConfiguration->loadConnectionConfiguration(sClientConfigFile);
        status = pClientConfiguration->loadDeviceConfiguration(serverlist);
        std::cout << "  -- Done Loading Configuration --\n";

        // Create and initialize communication interface.
        PasCommunicationInterface *pCommIf = new PasCommunicationInterface();
        pCommIf->setConfiguration(pClientConfiguration);
        // this initializes the communication interface, including all devices that talk
        // directly to it, like Aravis cameras
        UaStatus ret = pCommIf->Initialize();
        UA_ASSERT(ret.isGood());

        // Create instance of the node manager that invokes client calls
        PasNodeManager *pNodeManagerClient = new PasNodeManager();
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
            std::cout << "************************************\n";
            
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
                switch ( action )
                {
                    case 0:
                        printf("!! Panicking !!\n");
                        ret = pNodeManagerClient->Panic();
                        break;
                    default:
                        continue;
                }
            }

            std::cout << "*************************************\n";
            std::cout << " Shutting down server/client\n";
            std::cout << "*************************************\n";

            // Stop the logic thread;
            //pLogic->stop();

            // Stop the server and wait three seconds if clients are connected
            // to allow them to disconnect after they received the shutdown signal
            pServer->stop(3, UaLocalizedText("", "User shutdown"));
        } else {
            std::cout << "*******************************************\n";
            std::cout << "  ---- FAILED TO START SERVER/CLIENT ----\n";
            std::cout << "             Shutting down\n";
            std::cout << "*******************************************\n";
        }

        if (pClientConfiguration) {
            delete pClientConfiguration;
            pClientConfiguration = nullptr;
        }

        if (pServer) {
            delete pServer;
            pServer = nullptr;
        }

        /**
        if (pLogic) {
            delete pLogic;
            pLogic = nullptr;
        }
        */
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
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <Positions of panels to connect to>\n";
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
    for (int i = 1; i < argc; i++)
        positionList.push_back(argv[i]);

    // Run main method
    ret = OpcMain(pszAppPath, positionList);
    
    if ( pszAppPath ) delete [] pszAppPath;

#ifndef WIN32
    close_keyboard();
#endif

    return ret;
}
