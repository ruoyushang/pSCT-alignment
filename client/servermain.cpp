/**
 * @file servermain.cpp
 * @brief Main entry point for OPC UA alignment client
 */

#include "opcserver.h"
#include "shutdown.h"
#include "uaplatformlayer.h"
#include "uathread.h"
#if SUPPORT_XML_PARSER // Whether to use XML config files
  #include "xmldocument.h"
#endif
#include "pasnodemanager.h"
#include "configuration.h"
#include "pascommunicationinterface.h"
#include "paslogic.h"
#include "shutdown.h"
#include <vector>
#include <string>


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

/// @brief Helper function to wait for key press and map the result to an int
/// action code.
/// @details Maps keys 0-9 to actions 0-9 and maps keys a-k to actions 10-20. If
/// the received key press is none of the above, action is set to -1. If the 'x'
/// key is pressed, the function returns True, indicating that the server
/// should be shut down. If not, it returns False.
/// @param action A reference to an int variable storing the read action code.
/// @return Boolean value indicating if an 'x' key was pressed
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

/// @brief Primary entry point function to start OPC UA alignment client
/// @details Locates configuration files based on szAppPath, initializes
/// OpcServer object, and parses configuration files to get server settings
/// and to initialize Configuration object. Reads list of panel numbers
/// (servers) to initialize devices. Initializes PasCommunicationInterface using
/// Configuration object. Initializes PasNodeManager using Configuration and
/// PasCommunicationInterface objects. Finally, starts server and waits for
/// key press to shut down. When shutdown command is received, cleans up by
/// disconnecting clients and deleting server, Configuration,
/// PasCommunicationInterface.
/// @param szAppPath Application path string (path to main directory of
/// application) 
/// @param serverlist Vector of panel position numbers as strings.
int OpcMain(const char* szAppPath, std::vector<std::string> serverlist)
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

    UaStatus status;

    if ( ret == 0 )
    {
        // Create configuration file name
        UaString sServConfigFile(szAppPath);
        UaString sClientConfigFile(szAppPath);

#if SUPPORT_XML_PARSER
        sServConfigFile += "/PasServerConfig.xml";
#else
        sServConfigFile += "/PasServerConfig.ini";
#endif
        sClientConfigFile += "/PasClientConfig.ini";


        Configuration *pClientConfiguration = nullptr;
        PasCommunicationInterface *pCommIf = nullptr;
        PasLogic *pLogic = nullptr;
        //- Start up OPC server ---------------------
        // This code can be integrated into a startup
        // sequence of the application where the
        // OPC server should be integrated
        //-------------------------------------------
        // Create and initialize server object
        OpcServer* pServer = new OpcServer();
        pServer->setServerConfig(sServConfigFile, szAppPath);

        // load configuration. This is a big object -- allocate on the heap
        pClientConfiguration = new Configuration();
        printf("  ---- Loading Configuration ----\n");
        status = pClientConfiguration->loadConnectionConfiguration(sClientConfigFile);
        status = pClientConfiguration->loadDeviceConfiguration(serverlist);
        printf("  -- Done Loading Configuration --\n");

        // Create and initialize communication interface.
        // Again, this is a big object -- allocate on the heap
        pCommIf = new PasCommunicationInterface();
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

        printf("\nPasServer: starting\n");

        // Start server object
        ret = pServer->start();
        if ( ret != 0 )
        {
            printf("*******************************************\n");
            printf("  ---- FAILED TO START SERVER/CLIENT ----\n");
            printf("             Shutting down\n");
            printf("*******************************************\n");
            delete pServer;
            pServer = 0;
        }


        //-------------------------------------------
        if ( ret == 0 )
        {
            printf("************************************\n");
            printf("  ---- SERVER/CLIENT STARTED ----\n");
            printf("    Press x to shut down server\n");
            printf("************************************\n");
            // Wait for user command to terminate the server thread.

            //pLogic = new PasLogic(pCommIf);
            //pLogic->start();

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

            printf("*************************************\n");
            printf(" Shutting down server/client\n");
            printf("*************************************\n");

            //- Stop OPC server -------------------------
            // This code can be integrated into a shutdown
            // sequence of the application where the
            // OPC server should be integrated
            //-------------------------------------------
            // Stop the logic thread;
            //pLogic->stop();

            // Stop the server and wait three seconds if clients are connected
            // to allow them to disconnect after they received the shutdown signal
            pServer->stop(3, UaLocalizedText("", "User shutdown"));

            delete pServer;
            pServer = nullptr;
        }
        // clean up
        if (pClientConfiguration) {
            delete pClientConfiguration;
            pClientConfiguration = nullptr;
        }
        if (pCommIf) {
            delete pCommIf;
            pCommIf = nullptr;
        }
        if (pLogic) {
            delete pLogic;
            pLogic = nullptr;
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

/// @brief Main function.
/// @details Receives a list of panel position numbers as command line args,
/// gets the application path, and calls OpcMain().
/// @param argv List of panel position numbers (servers) for the client to
/// connect to.
/// @return int status code, 0 for success.
int main(int argc, char* argv[])
#endif
{
    if (argc < 2) {
        printf("Usage: %s <Positions of panels to connect to>\n", argv[0]);
        return 1;
    }

#ifndef WIN32
    init_keyboard();
#endif

    int ret = 0;

//    RegisterSignalHandler();

    // Extract application path
    char* pszAppPath = getAppPath();
    std::vector<std::string> positionlist;
    for (int i = 1; i < argc; i++)
        positionlist.push_back(argv[i]);

    //-------------------------------------------
    // Call the OPC server main method
    ret = OpcMain(pszAppPath, positionlist);
    //-------------------------------------------

    if ( pszAppPath ) delete [] pszAppPath;

#ifndef WIN32
    close_keyboard();
#endif

    return ret;
}
