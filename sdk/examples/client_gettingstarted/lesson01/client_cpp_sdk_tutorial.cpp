#include "uaplatformlayer.h"
#include "sampleclient.h"

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

    // Connect to OPC UA Server
    status = pMyClient->connect();

    // Connect succeeded
    if (status.isGood())
    {
        // Read values
        status = pMyClient->read();

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
