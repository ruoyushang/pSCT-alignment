#include "uaplatformlayer.h"
#include "sampleclient.h"
#include "configuration.h"
#include "../../utilities/shutdown.h"

/*============================================================================
 * main
 *===========================================================================*/
#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int, char*[])
#endif
{
    UaStatus status;

    // Initialize the UA Stack platform layer
    UaPlatformLayer::init();

    // Create instance of SampleClient
    SampleClient* pMyClient = new SampleClient();

    // get current path to build configuration file name
    UaString sConfigFile(getAppPath());
    sConfigFile += "/sampleconfig.ini";

    // Create configuration object and load configuration
    Configuration* pMyConfiguration = new Configuration();
    status = pMyConfiguration->loadConfiguration(sConfigFile);

    // Loading configuration succeeded
    if (status.isGood())
    {
        // set configuration
        pMyClient->setConfiguration(pMyConfiguration);
        // Connect to OPC UA Server
        status = pMyClient->connect();
    }
    else
    {
        delete pMyConfiguration;
        pMyConfiguration = NULL;
    }

    // Connect succeeded
    if (status.isGood())
    {
        // Wait for user command.
        printf("\n*************************************************************************\n");
        printf("* Press Enter to create a subscription\n");
        printf("* Press Enter again to stop the subscription\n");
        printf("*************************************************************************\n");
        getchar();

        // Create subscription
        status = pMyClient->subscribe();

        if (status.isGood())
        {
            // call methods;
            pMyClient->callMethods();

            // Wait for user command.
            getchar();

            // Delete subscription
            status = pMyClient->unsubscribe();
        }

        // Wait for user command.
        printf("\nPress Enter to disconnect\n");
        getchar();

        // Disconnect from OPC UA Server
        status = pMyClient->disconnect();
    }

    // Close application
    printf("\nPress Enter to close\n");
    // Wait for user command.
    getchar();

    delete pMyClient;
    pMyClient = NULL;

    // Cleanup the UA Stack platform layer
    UaPlatformLayer::cleanup();

    return 0;
}
