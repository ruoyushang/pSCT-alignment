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

    // Loading configuration succeeded - use discovery to get information about the server
    if (status.isGood())
    {
        // set configuration
        pMyClient->setConfiguration(pMyConfiguration);

        // Discover
        status = pMyClient->discover();
    }
    else
    {
        delete pMyConfiguration;
        pMyConfiguration = NULL;
    }

    // Discovery succeeded - now we connect
    if (status.isGood())
    {
        // Wait for user command.
        printf("\nPress Enter to connect with security\n");
        getchar();

        // Connect to OPC UA Server using a secure connection
        status = pMyClient->connectSecure();
    }

    // Connect succeeded
    if (status.isGood())
    {
        // Wait for user command.
        printf("\nPress Enter to read values\n");
        getchar();

        // Read values
        status = pMyClient->read();

        // Wait for user command.
        printf("\nPress Enter to write values\n");
        getchar();

        // Write values
        status = pMyClient->write();

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
